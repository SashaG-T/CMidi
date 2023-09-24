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

    FILE* f = openWAV("clouds.wav");      //Open and initialize a new WAV file

    cmidi_init(CMIDI_INIT_PRINT);       //Initialize CMIDI.. but only init the "print" command.
                                        //If we don't want the "print" command (or any custom ones we
                                        //might want to register) than we can omit this entierly.

    cmidi_setVolume(0.2f);  //We need to lower the master volume since we're converting to 16-bit signed
                            //(It will help prevent/reduce clipping... adjust accordingly)

    struct cmidi_Program programA = cmidi_Square;       //We need to setup working memory for each instrument
    struct cmidi_Program programB = cmidi_Saw;          //MIDI calls them 'programs' so CMIDI follows suit
    struct cmidi_Program programC = cmidi_Square2;
    struct cmidi_Program programD = cmidi_Triangle;
    struct cmidi_Program programE = cmidi_Test;
    struct cmidi_Program programF = cmidi_Sin2;
    struct cmidi_Program programG = cmidi_Saw;
    struct cmidi_Program programH = cmidi_Triangle;
    struct cmidi_Program programI = cmidi_Perc;
    cmidi_setProgram(69, &programA);                    //We have to setup the program patch
    cmidi_setProgram(60, &programB);                    //cmidi_setProgram is a macro that patches programs
    cmidi_setProgram(61, &programC);                    //into the default "program patch" for the default "device"
    cmidi_setProgram(47, &programD);
    cmidi_setProgram(80, &programE);                    //It probably sound a bit confusing but it's
    cmidi_setProgram(45, &programF);                    //possible to setup different "devices" that each
    cmidi_setProgram(48, &programG);                    //have their own "program patch".. some MIDI files
    cmidi_setProgram(46, &programH);                    //interact with a set of "devices".
    cmidi_setProgram(128+48, &programI);

                                                        //I've been putting "device" and "program patch" in quotes to
                                                        //hopefully help signify that they are entierly internal and virtual
                                                        //to CMIDI. If a MIDI file uses more than one "device" or "program patch"
                                                        //CMIDI will create a device internally and patch it to the same programs.
                                                        //If you want you can setup your own "devices".
                                                        //
                                                        //See: cmidi_setPort(..); cmidi_openDevice(..); cmidi_closeDevice(..);

    struct cmidi_Song* song = cmidi_load("midi/clouds.mid"); //load the MIDI file into memory.

    /* If you don't know which programs are used then you can set them all

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

    */

    const char* err = 0;
    do {
        float buffer[4410];
        for(int i = 0; i < 4410; i++) {
            buffer[i] = 0.0f;
        }
        cmidi_read(song, buffer, 2205);     //processes the MIDI file advancing it and returns 32-bit float PCM data.
        char out[8820];
        for(int i = 0; i < 4410; i++) {
            short s = buffer[i] * 32767;    //conversion into 16-bit signed PCM data.
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
