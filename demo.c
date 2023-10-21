#include "cmidi.h"
#include "cmidiprograms.h"
#include <stdio.h>
#include <stdlib.h>

FILE* openWAV(const char* filename) {
    FILE* f = fopen(filename, "wb");
    if(f) {
        //RIFF WAVEfmt header for 2 channel, 44100Hz, signed 16-bit PCM
        char meta[] = {
            'R','I','F','F',                    // RIFF (header)
            0, 0, 0, 0,                         // RIFF content size: we write this later.
            'W','A','V','E','f','m','t',' ',    // WAVEfmt (header)
            0x10, 0, 0, 0,                      // 16?
            0x01, 0,                            // 1? (format I think)
            0x02, 0,                            // number of channels
            0x44, 0xAC, 0, 0,                   // sample rate
            0x10, 0xB1, 0x02, 0,                // number of channels * sample rate * sample size
            0x04, 0,                            // size of interleaved sample packet in bytes: number of channels * sample size
            0x10, 0,                            // size of a single sample in bits: sample size * sizeof(short)
            'd','a','t','a',                    // data (header)
            0, 0, 0, 0                          // data content size: we write this later.
        };
        fwrite(meta, 1, sizeof(meta), f);
    }
    return f;
}
void fwriteu(unsigned int value, FILE* f) {
    if(f) {
        char v[4] = {
            value,
            value >> 8,
            value >> 16,
            value >> 24
        };
        fwrite(v, 1, 4, f);
    }
}
void closeWAV(FILE* f) {
    if(f) {
        fseek(f, 0, SEEK_END);
        unsigned int filesize = ftell(f);
        fseek(f, 4, SEEK_SET);
        fwriteu(filesize - 8, f); //sub 8 for RIFF header
        fseek(f, 40, SEEK_SET);
        fwriteu(filesize - (8 + 36), f); //sub 8 for RIFF header and 36 for WAVE header.
        fclose(f);
    }
}

int main(int argc, char* argv[]) {

    FILE* f = openWAV("deb_clai.wav");  //Open and initialize a new WAV file

    cmidi_init(CMIDI_INIT_NONE);        //Initialize CMIDI.. CMIDI_INIT_NONE means we're not interpreting
                                        //MIDI text events are commands.

    cmidi_setVolume(0.2f);  //We need to lower the master volume since we're converting to 16-bit signed
                            //(It will help prevent/reduce clipping... adjust accordingly)

    struct cmidi_Program squarePluck = cmidi_Test;      //We need to setup working memory for each instrument
                                                        //MIDI calls them 'programs' so CMIDI follows suit

    cmidi_setProgram(0, &squarePluck);                  //We have to setup the program patch
                                                        //cmidi_setProgram is a macro that patches programs
                                                        //into the default "program patch" for the default "device"

    struct cmidi_Song* song = cmidi_load("midi/deb_clai.mid");  //load the MIDI file into memory.

    const char* err = 0;
    do {
        float buffer[4410];
        for(int i = 0; i < 4410; i++) {
            buffer[i] = 0.0f;
        }
        cmidi_read(song, buffer, 2205);         //processes the MIDI file advancing it and returns 32-bit float PCM data.
        char out[8820];
        for(int i = 0; i < 4410; i++) {
            short s = buffer[i] * 32767;        //conversion into 16-bit signed PCM data.
            out[i*2]    = s;
            out[i*2+1]  = s >> 8;
        }
        fwrite(out, 1, 8820, f);
        if(feof(f)) {
            err = "Reached end of file before file writting could finish.";
        }
    } while(!cmidi_eot(song) && !err);

    if(err) {
        printf("demo: %s\n", err);
    }

    cmidi_free(song);       //Free any memory help by our cmidi_Song struct
    cmidi_quit();           //This cleans up some internal hash tables

    closeWAV(f);            //Complete the WAV file and close it.

    puts("Good-bye!");

    return 0;
}
