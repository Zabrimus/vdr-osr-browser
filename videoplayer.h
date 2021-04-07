#ifndef VDR_OSR_BROWSER_VIDEOPLAYER_H
#define VDR_OSR_BROWSER_VIDEOPLAYER_H

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#ifdef __cplusplus
}
#endif

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include <thread>
#include <vector>
#include "readerwriterqueue.h"

using namespace moodycamel;

class VideoPlayer {
    public:
        VideoPlayer();
        ~VideoPlayer();

        void setAudioParameters(int channels_, int sample_rate);
        bool addVideoFrame(int width, int height, uint8_t* image, uint64_t pts);
        bool addAudioFrame(const float **data, int frames, uint64_t pts);

        void stop();

        bool isInitialized() { return initialized; };

    private:
        bool quit;
        bool initialized;
        bool isFullScreen;

        int sampleRate;
        int channels;

        uint64_t lastVideoPts;
        uint64_t lastAudioPts;

        std::vector<std::thread> threads;

        ReaderWriterQueue<AVFrame*> audioQueue;
        ReaderWriterQueue<AVFrame*> imageQueue;

        SwsContext* swsCtx;
        SwrContext* swrCtx;

        SDL_Window *window;
        SDL_Renderer *renderer;
        SDL_Texture *texture;
        SDL_Event event;

        SDL_AudioSpec audioSpec;
        SDL_AudioDeviceID audioDevice;

        SDL_mutex* init_mutex;

        bool initSDLVideo();
        void playVideo();
        void playAudio();

        void input();
};

#endif //VDR_OSR_BROWSER_VIDEOPLAYER_H
