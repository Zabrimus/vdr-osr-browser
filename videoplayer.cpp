#include "logger.h"
#include "videoplayer.h"
#include "keycodes.h"
#include "sendvdr.h"

// Fix some compile problems
#undef av_err2str
#define av_err2str(errnum) av_make_error_string((char*)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)

#undef av_ts2str
#define av_ts2str(ts) av_ts_make_string((char*)__builtin_alloca(AV_TS_MAX_STRING_SIZE), ts)

#undef av_ts2timestr
#define av_ts2timestr(ts, tb) av_ts_make_time_string((char*)__builtin_alloca(AV_TS_MAX_STRING_SIZE), ts, tb)

VideoPlayer::VideoPlayer(bool fullscreen) {
    audioQueue = ReaderWriterQueue<AVFrame*>(5);
    imageQueue = ReaderWriterQueue<AVFrame*>(5);

    swsCtx = sws_getContext(1280, 720,
                            AV_PIX_FMT_BGRA,
                            1280, 720,
                            AV_PIX_FMT_YUV420P,
                            SWS_BICUBIC, nullptr, nullptr, nullptr);

    swrCtx = swr_alloc();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        CONSOLE_ERROR("Failed to init SDL: {}",  SDL_GetError());
        return;
    }

    window = nullptr;
    renderer = nullptr;
    texture = nullptr;

    lastAudioPts = 0;
    lastVideoPts = 0;

    initialized = false;
    isFullScreen = fullscreen;

    init_mutex = SDL_CreateMutex();

    quit = false;

    threads.emplace_back(&VideoPlayer::playAudio, this);
    threads.emplace_back(&VideoPlayer::playVideo, this);
}

VideoPlayer::~VideoPlayer() {
    if (swsCtx != nullptr) {
        sws_freeContext(swsCtx);
    }

    swsCtx = nullptr;

    if (swrCtx != nullptr) {
        swr_free(&swrCtx);
    }

    swrCtx = nullptr;

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
}

bool VideoPlayer::initSDLVideo() {
    CONSOLE_TRACE("initSDLVideo called...");

    initialized = true;

    int width = 1280;
    int height = 720;

    /*
    SDL_WINDOW_FULLSCREEN - fullscreen window
    SDL_WINDOW_OPENGL - the window usable with OpenGL context
    SDL_WINDOW_SHOWN - the window is visible (ignored by SDL_CreateWindow)
    SDL_WINDOW_HIDDEN - the window is not visible
    SDL_WINDOW_BORDERLESS - the window have no decoration
    SDL_WINDOW_RESIZABLE - the window can be resized
    SDL_WINDOW_MINIMIZED - the window is minimized
    SDL_WINDOW_MAXIMIZED - the window is maximized
    SDL_WINDOW_INPUT_GRABBED - the window has grabbed input focus
    SDL_WINDOW_INPUT_FOCUS - the window has input focus
    SDL_WINDOW_MOUSE_FOCUS - the window has mouse focus
    SDL_WINDOW_FULLSCREEN_DESKTOP - ( SDL_WINDOW_FULLSCREEN | 0x00001000 )
    SDL_WINDOW_FOREIGN - the window not created by SDL
    SDL_WINDOW_ALLOW_HIGHDPI - the window created in high dpi mode
    SDL_WINDOW_MOUSE_CAPTURE -the window has mouse captured (unrelated to INPUT_GRABBED)
    SDL_WINDOW_ALWAYS_ON_TOP - the window should always be above others
    SDL_WINDOW_SKIP_TASKBAR - the window should not be added to the taskbar
    SDL_WINDOW_UTILITY - the window is a utility window
    SDL_WINDOW_TOOLTIP - the window is a tooltip
    SDL_WINDOW_POPUP_MENU - the window is a popup menu
    SDL_WINDOW_VULKAN - the window usable for Vulkan surface
    */

    if (window != nullptr) {
        CONSOLE_CRITICAL("WINDOW ALREADY EXISTS...");
        return true;
    }

    window = SDL_CreateWindow("videoplayer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_ShowCursor(SDL_ENABLE);

    setFullScreen(isFullScreen);

    /*
    SDL_RENDERER_SOFTWARE - The renderer is a software fallback
    SDL_RENDERER_ACCELERATED - The renderer uses hardware acceleration
    SDL_RENDERER_PRESENTVSYNC - Present is synchronized with the refresh rate
    SDL_RENDERER_TARGETTEXTURE - The renderer supports rendering to texture
    */

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        CONSOLE_ERROR("Unable to create renderer: {}", SDL_GetError());
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (texture == nullptr) {
        CONSOLE_ERROR("Unable to create texture: {}", SDL_GetError());
    }

    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear")) {
        CONSOLE_ERROR("SDL_SetHint: {}", SDL_GetError());
    }

    SDL_RenderSetLogicalSize(renderer, width, height);

    if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) != 0) {
        CONSOLE_ERROR("SDL_SetRenderDrawColor: {}", SDL_GetError());
    }

    if (SDL_RenderClear(renderer) != 0) {
        CONSOLE_ERROR("SDL_RenderClear: {}", SDL_GetError());
    }

    SDL_RenderPresent(renderer);

    CONSOLE_TRACE("initSDLVideo finished...");

    return true;
}

void VideoPlayer::setAudioParameters(int channels_, int sample_rate) {
    uint64_t channelLayout = av_get_default_channel_layout(channels_);

    av_opt_set_channel_layout(swrCtx, "in_channel_layout", channelLayout, 0);
    av_opt_set_channel_layout(swrCtx, "out_channel_layout", channelLayout, 0);
    av_opt_set_int(swrCtx, "in_sample_rate", sample_rate, 0);
    av_opt_set_int(swrCtx, "out_sample_rate", sample_rate, 0);
    av_opt_set_sample_fmt(swrCtx, "in_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);
    av_opt_set_sample_fmt(swrCtx, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);

    swr_init(swrCtx);

    this->channels = channels_;
    this->sampleRate = sample_rate;

    memset(&audioSpec, 0, sizeof(audioSpec));
    audioSpec.freq = sample_rate;
    audioSpec.format = AUDIO_F32;
    audioSpec.channels = channels;
    audioSpec.samples = 1024;

    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &audioSpec, &audioSpec, 0);

    SDL_PauseAudioDevice(audioDevice, 0);
}

bool VideoPlayer::addVideoFrame(int width, int height, uint8_t* image, uint64_t pts) {
    if (!initialized) {
        return false;
    }

    // convert to YUV and add to Queue

    AVFrame* frame = av_frame_alloc();

    int ret = av_image_alloc(frame->data, frame->linesize, width, height, AV_PIX_FMT_YUV420P, 32);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Could not allocate raw picture buffer: %d, %s\n", ret, av_err2str(ret));
        return false;
    }

    uint8_t *inData[1] = { image };
    int inLinesize[1] = { 4 * width };

    sws_scale(swsCtx, inData, inLinesize, 0, height, frame->data, frame->linesize);

    frame->pts = pts;

    imageQueue.enqueue(std::move(frame));

    return true;
}

bool VideoPlayer::addAudioFrame(const float **data, int frames, uint64_t pts) {
    if (!initialized) {
        return false;
    }

    AVFrame* frame = av_frame_alloc();

    if (av_samples_alloc(&frame->data[0], nullptr, channels, 1024, AV_SAMPLE_FMT_FLT, 0) >= 0) {
        swr_convert(swrCtx, &frame->data[0], 1024, reinterpret_cast<const uint8_t **>(data), 1024);
        frame->pts = pts;

        audioQueue.enqueue(frame);
    }

    return true;
}

void VideoPlayer::playAudio() {
    int bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT);
    size_t linesize = 1024 * bytesPerSample;

    while (!quit) {
        AVFrame *frame;
        if (audioQueue.try_dequeue(frame)) {
            SDL_QueueAudio(audioDevice, frame->data[0], linesize * channels);

            lastAudioPts = frame->pts;

            av_freep(&frame->data[0]);
            av_frame_free(&frame);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void VideoPlayer::playVideo() {
    SDL_LockMutex(init_mutex);

    if (!initSDLVideo()) {
        CONSOLE_ERROR("InitSDLVideo failed: {}", SDL_GetError());
        initialized = false;
        quit = true;
        return;
    }

    SDL_UnlockMutex(init_mutex);

    AVFrame *frame;

    while(!quit) {
        input();

        if (imageQueue.try_dequeue(frame)) {
            // fprintf(stderr, "Video PTS: %10ld,  Audio PTS: %10ld, Difference: %10ld\n", lastVideoPts, lastAudioPts, (int64_t)(lastVideoPts - lastAudioPts));
            // fprintf(stderr, "Video PTS Diff: %10ld\n", frame->pts-lastVideoPts);
            // fprintf(stderr, "Video PTS: %10ld,  Audio PTS: %10ld, Difference: %10ld\n", lastVideoPts / 1000, lastAudioPts / 1000 , (int64_t)(lastVideoPts - lastAudioPts) / 1000);

            // FIXME: Ich habe nicht unbedingt das Gefühl, daß dies so richtig echt richtig ist.
            /*
            if (lastAudioPts != 0 && (int64_t)(lastVideoPts - lastAudioPts) < 0) {
                std::this_thread::sleep_for(std::chrono::nanoseconds((int64_t) (-lastVideoPts + lastAudioPts)));
            }
            */

            if (SDL_UpdateYUVTexture(
                    texture, nullptr,
                    frame->data[0], frame->linesize[0],
                    frame->data[1], frame->linesize[1],
                    frame->data[2], frame->linesize[2]) == 0) {

                if (SDL_RenderClear(renderer) != 0) {
                    CONSOLE_TRACE("PlayVideo, RenderClear failed: {}", SDL_GetError());
                }

                if (SDL_RenderCopy(renderer, texture, nullptr, nullptr) != 0) {
                    CONSOLE_TRACE("PlayVideo, RenderCopy failed: {}", SDL_GetError());
                }

                SDL_RenderPresent(renderer);
            } else {
                CONSOLE_TRACE("PlayVideo, UpdateYUVTexture failed: {}", SDL_GetError());
            }

            lastVideoPts = frame->pts;

            av_freep(&frame->data[0]);
            av_frame_free(&frame);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
}

void VideoPlayer::stop() {
    quit = true;

    for (auto &th : threads) {
        th.join();
    }
}

void VideoPlayer::input() {
    // FIXME: Events... Tastatur, Fernbedienung,...

    if (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                break;

            case SDL_KEYUP:
                if (keyCodesSDL.count(event.key.keysym.sym) > 0) {
                    std::string xsymcode = keyCodesSDL[event.key.keysym.sym];

                    char *buffer = nullptr;
                    asprintf(&buffer, "KEY: %s", xsymcode.c_str());
                    SendToVdrString(CMD_STATUS, buffer);
                    free(buffer);
                } else {
                    // ignore this key value
                }

                break;

            case SDL_MOUSEBUTTONDOWN:
                toggleFullScreen();
                break;
            case SDL_QUIT:
                stop();
                break;

            default:
                break;
        }
    }
}

void VideoPlayer::toggleFullScreen() {
    isFullScreen = !isFullScreen;
    setFullScreen(isFullScreen);
}

void VideoPlayer::setFullScreen(bool full) {
    if (full) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_ShowCursor(SDL_DISABLE);
    } else {
        SDL_SetWindowFullscreen(window, 0);
        SDL_ShowCursor(SDL_ENABLE);
    }
}