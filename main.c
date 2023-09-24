#include <stdio.h>
#include <stdlib.h>
#include "csdlctx.h"
#include "cmidi.h"
#include "cmidiprograms.h"
#include "cmidi_sdlext.h"

struct cmidi_Song* song;
struct cmidi_Song* sfx;

struct cmidi_Device* sfxDevice;
struct cmidi_Patch sfxPatch;

void audioCallback(void* userdata, Uint8* buffer, int len) {

    int flen = len/(sizeof(float)*2); //len is length in raw bytes... we need length in stereo floats
    float* buf = buffer;
    for(int i = 0; i < flen; i++) { //I think this is wrong: needs to be flen*2?? since we're iterating of samples and not paired left-right samples.
        buf[i] = 0.0f;
    }
    cmidi_read(song, buf, flen); //fill buffer with 32-bit stereo float PCM data

    if(sfx) {
        cmidi_read(sfx, buf, flen);
    }

    sdlctx_drawWave(buf, flen);
}

#define _CASE(X) \
                case SDLK_ ## X: { \
                    printf("%d\n", X); \
                    sdlctx_lockAudioDevice(); \
                    cmidi_playTrack(sfx, X, 0, 0, 0); \
                    sdlctx_unlockAudioDevice(); \
                    break; \
                }

//Play some SFX when the keys 1, 2, 3, 4, & 5 are pressed.
//Key 0 will play track 1 of the sfx which will be silent meta data.
void eventHandler(SDL_Event* e) {
    switch(e->type) {
        case SDL_KEYDOWN: {
            switch(e->key.keysym.sym) {
                _CASE(0)
                _CASE(1)
                _CASE(2)
                _CASE(3)
                _CASE(4)
                _CASE(5)
            }
            break;
        }
    }
}

int main(int argc, char* argv[]) {

    if(argc != 2) {
        puts("Bad argument count.\n\nUsage: CMidi.exe [filename]\n\n\tfilename - Relative path to midi file.\n");
        return 1;
    }

    printf("Playing %s...\n", argv[1]);

    //init SDL
    sdlctx_init();
    sdlctx_createWindow("CMidi", 300, 300);
    sdlctx_initAudioDevice(audioCallback);

    //init cmidi
    cmidi_init(CMIDI_INIT_ALL);

    //setup the programs (instruments)
    //this is a pretty good setup for GM midi for what is provided by CMidi
    struct cmidi_Program programA = cmidi_Square;
    struct cmidi_Program programB = cmidi_Saw;
    struct cmidi_Program programC = cmidi_Square2;
    struct cmidi_Program programD = cmidi_Triangle;
    struct cmidi_Program programE = cmidi_Test;
    struct cmidi_Program programF = cmidi_Sin2;
    struct cmidi_Program programG = cmidi_Saw;
    struct cmidi_Program programH = cmidi_Triangle;
    struct cmidi_Program programI = cmidi_Perc;
    cmidi_setProgram(69, &programA);
    cmidi_setProgram(60, &programB);
    cmidi_setProgram(61, &programC);
    cmidi_setProgram(47, &programD);
    cmidi_setProgram(80, &programE);
    cmidi_setProgram(45, &programF);
    cmidi_setProgram(48, &programG);
    cmidi_setProgram(46, &programH);
    cmidi_setProgram(128+48, &programI);

    for(int i = 0; i < 128; i++) {
        if(!cmidi_patch_default.program[i]) {
            cmidi_setProgram(i, &cmidi_Test);
        }
    }
    for(int i = 128; i < 256; i++) {
        if(!cmidi_patch_default.program[i]) {
            cmidi_setProgram(i, &cmidi_Perc);
        }
    }

    //load up the SFX and assign it it's own device so that instruments and channels don't collide.
    sfx = cmidi_load("midi/sfx.mid");
    for(int i = 0; i < 128; i++) {
        cmidi_setPatchProgram(&sfxPatch, i, &cmidi_Square2);
    }
    sfxDevice = cmidi_openDevice("SFX Device", &sfxPatch);
    cmidi_setDevice(sfx, sfxDevice);

    cmidi_playTrack(sfx, 0, 0, 0, 0); //tell cmidi to only play the first track; which is tempo and time signature stuff.

    song = cmidi_load(argv[1]);
    cmidi_setVolume(0.3f);
    sdlctx_setEventHandler(eventHandler);

    //Tell SDL to start playing the audio
    sdlctx_playAudioDevice();

    //SDL event loop
    while(sdlctx_running() && !cmidi_eot(song)) {
        sdlctx_handleEvents();
    }

    //Free memory and close up shop
    sdlctx_closeAudioDevice();  //close audio device
    cmidi_free(song);           //free song data (audio device must be paused or closed before doing this)
    cmidi_free(sfx);
    cmidi_closeDevice(sfxDevice);
    sdlctx_quit();
    cmidi_quit();

    puts("Good-bye!");

    return 0;
}

