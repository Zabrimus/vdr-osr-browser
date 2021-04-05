#include <iostream>
#include <fstream>
#include "logger.h"
#include "videoplayer.h"

int main(int argc, char* argv[]) {
    printf("LoadImage\n");

    std::ifstream fl("testimage.rgba");
    fl.seekg(0, std::ios::end);
    size_t len = fl.tellg();
    char *image = new char[len];
    fl.seekg(0, std::ios::beg);
    fl.read(image, len);
    fl.close();

    printf("Create audio\n");
    const float* audio[2];
    audio[0] = static_cast<float *>(malloc(1024));
    audio[1] = static_cast<float *>(malloc(1024));

    logger.set_level(spdlog::level::trace);

    if (argc > 1 && strncmp(argv[1], "1", 1) == 0) {
        printf("Create VideoPlayer\n");

        VideoPlayer *videoPlayer = new VideoPlayer();

        videoPlayer->setAudioParameters(2, 48000);

        for (int i = 0; i < 1000; ++i) {
            fprintf(stderr, "Frame: %d\r", i);
            videoPlayer->addVideoFrame(1280, 720, (uint8_t *) image, i * 1000);
            videoPlayer->addAudioFrame(reinterpret_cast<const float **>(&audio[0]), 1024, i * 500);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        printf("\nWait some time...\n");
        std::this_thread::sleep_for(std::chrono::seconds(5));

        videoPlayer->stop();

        delete videoPlayer;
        delete[] image;
    } else {
        SwsContext *swsCtx = sws_getContext(1280, 720,
                                            AV_PIX_FMT_BGRA,
                                            1280, 720,
                                            AV_PIX_FMT_YUV420P,
                                            SWS_BICUBIC, nullptr, nullptr, nullptr);

        // Init SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
            printf("Failed to init SDL: %s\n", SDL_GetError());
            return -1;
        }

        SDL_Window* window = SDL_CreateWindow("videoplayer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (renderer == nullptr) {
            printf("Unable to create renderer: %s\n", SDL_GetError());
        }

        SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, 1280, 720);
        if (texture == nullptr) {
            printf("Unable to create texture: %s\n", SDL_GetError());
        }

        if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear")) {
            printf("SDL_SetHint: %s\n", SDL_GetError());
        }

        SDL_RenderSetLogicalSize(renderer, 1280, 720);
        if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) != 0) {
            printf("SDL_SetRenderDrawColor: %s\n", SDL_GetError());
        }

        if (SDL_RenderClear(renderer) != 0) {
            printf("SDL_RenderClear: %s\n", SDL_GetError());
        }

        SDL_RenderPresent(renderer);


        // convert to YUV and add to Queue
        AVFrame* frame = av_frame_alloc();

        int ret = av_image_alloc(frame->data, frame->linesize, 1280, 720, AV_PIX_FMT_YUV420P, 32);
        if (ret < 0) {
            return -1;
        }

        uint8_t *inData[1] = { (uint8_t*)image };
        int inLinesize[1] = { 4 * 1280 };

        sws_scale(swsCtx, inData, inLinesize, 0, 720, frame->data, frame->linesize);

        frame->pts = 1;

        for (int i = 0; i < 100; ++i) {
            frame->pts++;

            // show image
            if (SDL_UpdateYUVTexture(
                    texture, nullptr,
                    frame->data[0], frame->linesize[0],
                    frame->data[1], frame->linesize[1],
                    frame->data[2], frame->linesize[2]) == 0) {

                if (SDL_RenderClear(renderer) != 0) {
                    printf("PlayVideo, RenderClear failed: %s\n", SDL_GetError());
                }

                if (SDL_RenderCopy(renderer, texture, nullptr, nullptr) != 0) {
                    printf("PlayVideo, RenderCopy failed: %s\n", SDL_GetError());
                }

                SDL_RenderPresent(renderer);
            } else {
                printf("UpdateYUVTexture failed: %s\n", SDL_GetError());
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        av_frame_free(&frame);

        std::this_thread::sleep_for(std::chrono::seconds(5));

        // close everything
        if (swsCtx != nullptr) {
            sws_freeContext(swsCtx);
        }

        swsCtx = nullptr;

        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);

        SDL_Quit();
        delete[] image;
    }
}