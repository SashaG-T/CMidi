#include "csdlctx.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct SDLContext {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    int running;
    int audioOnly;

    const char* title;
    unsigned width;
    unsigned height;
};

static struct SDLContext* sdlctx = 0;
static void (*_eventHandler)(SDL_Event* event);

void sdlctx_setEventHandler(void (*eventHandler)(SDL_Event* event)) {
    _eventHandler = eventHandler;
}

SDL_Renderer* sdlctx_getRenderer() {
    return sdlctx->renderer;
}

void ErrorMessage(char const * msg) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                         "CMidi Playground",
                         msg,
                         NULL);
        fprintf(stderr, "%s\n", msg);
}

void sdlctx_init()
{
    sdlctx = malloc(sizeof(struct SDLContext));
    if(SDL_Init(SDL_INIT_EVERYTHING)) {
            printf("\n%s\nSDL could not be initialized.\n", SDL_GetError());
            ErrorMessage(SDL_GetError());
            exit(1);
    }
    atexit(SDL_Quit);
    sdlctx->running = 1;
    sdlctx->audioOnly = 0;
}

void sdlctx_audio_only()
{
    sdlctx = malloc(sizeof(struct SDLContext));
    if(SDL_Init(SDL_INIT_AUDIO)) {
            printf("\n%s\nSDL could not be initialized.\n", SDL_GetError());
            ErrorMessage(SDL_GetError());
            exit(1);
    }
    atexit(SDL_Quit);
    sdlctx->running = 1;
    sdlctx->audioOnly = 1;
}

void sdlctx_quit() {
    if(!sdlctx->audioOnly) {
        SDL_DestroyTexture(sdlctx->texture);
        SDL_DestroyRenderer(sdlctx->renderer);
        SDL_DestroyWindow(sdlctx->window);
    }
    free(sdlctx);
    SDL_Quit();
}

static void createWindow(const char* title, unsigned width, unsigned height, Uint32 flags) {
    assert(!sdlctx->audioOnly);

    sdlctx->title = title;
    sdlctx->width = width;
    sdlctx->height = height;
    sdlctx->window = SDL_CreateWindow(title,
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              width, height,
                              flags);

    sdlctx->renderer = SDL_CreateRenderer(sdlctx->window, -1,
                                  SDL_RENDERER_ACCELERATED |
                                  SDL_RENDERER_PRESENTVSYNC |
                                  SDL_RENDERER_TARGETTEXTURE);

    if(sdlctx->window == NULL) {
        exit(2);
    }
    if(sdlctx->renderer == NULL) {
        exit(3);
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    SDL_ShowWindow(sdlctx->window);

    sdlctx->texture = SDL_CreateTexture(sdlctx->renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, width, height);
    if(!sdlctx->texture) {
        exit(4);
    }
}

void sdlctx_createFullscreenWindow(const char* title) {
    SDL_Rect r;
    SDL_GetDisplayBounds(0, &r);
    createWindow(title, r.w, r.h, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIDDEN);
}

void sdlctx_createWindow(const char* title, unsigned width, unsigned height) {
    createWindow(title, width, height, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);
}

void sdlctx_setLogicalSize(unsigned width, unsigned height) {
    sdlctx->width = width;
    sdlctx->height = height;
    SDL_RenderSetLogicalSize(sdlctx->renderer, width, height);
}

static SDL_AudioDeviceID device;
static SDL_AudioSpec spec;
void sdlctx_initAudioDevice(void (*callback)(void* userdata, Uint8* out, int len)) {
    if(!spec.callback) {
        SDL_AudioSpec want;
        SDL_memset(&want, 0, sizeof(want));
        want.freq = 48000;
        want.format = AUDIO_F32SYS;
        want.channels = 2;
        want.samples = 4096;
        want.callback = callback;
        device = SDL_OpenAudioDevice(NULL, 0, &want, &spec, 0);
    }
}

void sdlctx_closeAudioDevice() {
    SDL_CloseAudioDevice(device);
}

void sdlctx_playAudioDevice() {
    SDL_PauseAudioDevice(device, 0);
}

void sdlctx_pauseAudioDevice() {
    SDL_PauseAudioDevice(device, 1);
}

void sdlctx_lockAudioDevice() {
    SDL_LockAudioDevice(device);
}

void sdlctx_unlockAudioDevice() {
    SDL_UnlockAudioDevice(device);
}

void sdlctx_handleEvents() {
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(_eventHandler) {
            _eventHandler(&e);
        }
        switch(e.type) {
            case SDL_QUIT: {
                sdlctx->running = 0;
                break;
            }
        }
    }
}

int sdlctx_running() {
    return sdlctx->running;
}

void sdlctx_drawWave(float* in, int len) {
    int l = len/2;
    int start = sdlctx->width - l;
    SDL_SetRenderTarget(sdlctx->renderer, sdlctx->texture);
    SDL_Rect dst = {-l, 0, sdlctx->width, sdlctx->height};
    SDL_RenderCopy(sdlctx->renderer, sdlctx->texture, 0, &dst);
    dst.x = start;
    dst.w = l;
    SDL_SetRenderDrawColor(sdlctx->renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderFillRect(sdlctx->renderer, &dst);
    int max = sdlctx->height / 4;
    int loffset = max;
    int roffset = max*3;
    SDL_SetRenderDrawColor(sdlctx->renderer, 0x00, 0x00, 0x00, 0xff);
    for(int i = 0; i < len; i+=2) {
        start++;
        SDL_RenderDrawPoint(sdlctx->renderer, start, loffset + in[i] * max);
        SDL_RenderDrawPoint(sdlctx->renderer, start, roffset + in[i+1] * max);
    }
    SDL_SetRenderTarget(sdlctx->renderer, 0);
    SDL_RenderClear(sdlctx->renderer);
    SDL_RenderCopy(sdlctx->renderer, sdlctx->texture, 0, 0);
    SDL_RenderPresent(sdlctx->renderer);
}
