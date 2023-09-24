/*
    This file supplies the user with utility functions for CMIDI
    They are implemented using SDL.

    This file isn't require to use CMIDI and is supplied only for
    your convience.
*/
#ifndef _CMIDI_SDLEXT_H_
    #define _CMIDI_SDLEXT_H_

#include <SDL2/SDL.h>
#include <stdio.h>
#include "cmidi.h"

#define cmidi_sdlext_loadSamplerWAV(program,filename,loop,freq,filter,adsr) cmidi_sdlext_loadSamplerRW(cmidi_sdlext_WAV_RW,program,filename,loop,freq,filter,adsr)

typedef void (*cmidi_sdlext_RW)(const char* filename, float** out, int* len);

void cmidi_sdlext_loadSamplerRW(cmidi_sdlext_RW rw, struct cmidi_Program* program, const char* filename, int loop, float freq, cmidi_Program_Filter filter, struct cmidi_ADSR* asdr) {
    int len;
    float* data;
    rw(filename, &data, &len);
    cmidi_openSampler(program, data, len, loop, freq, filter, asdr);
}

void cmidi_sdlext_freeSampler(struct cmidi_Program* program) {
    struct cmidi_SampleData* user = program->user;
    free(user->monoSampleData);
    cmidi_closeSampler(program);
}

//cmidi_sdlext_WAV_RW only accepts a very narrow WAV file format.
//It was written for demo purposes. Please write your own if you want
//to support a more diverse set of WAV files.
void cmidi_sdlext_WAV_RW(const char* filename, float** out, int* len) {
    SDL_AudioSpec wav_spec;
    Uint32 wav_length;
    Uint8 *wav_buffer;

    /* Load the WAV */
    if (SDL_LoadWAV(filename, &wav_spec, &wav_buffer, &wav_length) == NULL) {
        fprintf(stderr, "Could not open %s: %s\n", filename, SDL_GetError());
    } else {
        /* Do stuff with the WAV data, and then... */
        const char* errmsg = 0;
        if(wav_spec.channels == 1) {
            if(wav_spec.format == AUDIO_S16) {
                if(wav_spec.freq != 44100) {
                    errmsg = "Frequency should be 44100... but we'll try to use it anyways.";
                }
                //Convert short 16 to float 32.
                int samples = wav_length/sizeof(short);
                float* _out = malloc(samples*sizeof(float));
                short* _in = wav_buffer;
                for(int i = 0; i < samples; i++) {
                    _out[i] = (_in[i] / 32767.f);
                }
                *out = _out;
                *len = samples;

                printf("samples: %d\n", samples);

            } else {
                errmsg = "Format must be signed 16 bit in little-endian byte order.";
            }
        } else {
            errmsg = "File must be mono.";
        }
        if(errmsg) {
            printf("cmidi_sdlext: %s\n", errmsg);
        }
        SDL_FreeWAV(wav_buffer);
    }
}


#endif // _CMIDI_SDLEXT_H_
