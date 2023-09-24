#ifndef CSDLCTX_H_
    #define CSDLCTX_H_

#include <SDL2/SDL.h>

void sdlctx_init();
void sdlctx_audio_only();
void sdlctx_quit();

void sdlctx_createFullscreenWindow(const char* title);
void sdlctx_createWindow(const char* title, unsigned width, unsigned height);
void sdlctx_setLogicalSize(unsigned width, unsigned height);
void sdlctx_handleEvents();
int sdlctx_running();

void sdlctx_initAudioDevice(void (*audioCallback)(void* userdata, Uint8* out, int len));
void sdlctx_closeAudioDevice();
void sdlctx_playAudioDevice();
void sdlctx_pauseAudioDevice();
void sdlctx_lockAudioDevice();
void sdlctx_unlockAudioDevice();

void sdlctx_drawWave(float* in, int len);

void sdlctx_setEventHandler(void (*eventHandler)(SDL_Event* event));
SDL_Renderer* sdlctx_getRenderer();

#endif // CSDLCTX_H_
