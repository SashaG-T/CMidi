#include <stdio.h>
#include <stdlib.h>
#include "csdlctx.h"
#include "cmidi.h"
#include "cmidiprograms.h"
#include "cmidi_sdlext.h"

/// Compile PCH build target and then run.
//  This will generate cmidiperc.pch which
//  is wrapped by cmidiperc.h for use by
//  cmidiprograms.c to supply a means of
//  basic percussion.

//  Ideally, pchgen.c would contain
//  GM standard percussion for common
//  program mapping.. enable-able via
//  define macros.

//  pchgen.c is a tool to be used while
//  compiling cmidi pch files. It's not
//  intented to be used by user programs.

//  pchgen.c converts the wav files into
//  raw single-point precision C-style
//  arrays.

/// Possible improvement:
//  Have pchgen.c parse the wav filenames
//  from a text file. This would enable
//  cmidiperc.pch to be regnerated in a
//  pre-build script for other build targets.
//
//  The text file could also contain
//  multiple percussion mappings.

//  p 0 Standard
//  s 40 Snare.wav
//  s 38 Snare2.wav
//  p 48 Orchestral
//  s 40 OrchestralSnare.wav
//  s 38 OrchestralSnare2.wav

//  a line would be formatted as follows:
//  [line identifier] [index] [value]
//
//  line identifier tells us what sort of information the particular line has:
//  p for program patch; closes previous patch and starts a new one. index is the program patch slot. value is name of the patch.
//  s for sample; adds a sample to the current patch. index is the instrument slot. value is the path to the WAV file.
//
//  all other lines that don't start with a p of s will be ignored... so comments and such can be added.

#define CMIDI_PERC_SAMPLE_SIZE 44100

char* percfiles[128] = {0};

void initPercfiles() {
    //All files must be 16-bit signed mono-channel 44100Hz WAVEfmt format.
    percfiles[35] = "samples/707 Kick.wav";
    percfiles[38] = "samples/808 Snare.wav";
    percfiles[40] = "samples/808 Snare.wav";
    percfiles[44] = "samples/808 CH.wav";
    percfiles[46] = "samples/808 OH.wav";
    percfiles[49] = "samples/TimpaniC#.wav";
    percfiles[54] = "samples/FPC Tamb.wav";
}

void printfloats(FILE* f, char* filename) {
    float* buf;
    int len;
    cmidi_sdlext_WAV_RW(filename, &buf, &len);
    for(int i = 0; i < CMIDI_PERC_SAMPLE_SIZE; i++) {
        float value = 0.0f;
        if(i < len) {
            value = buf[i];
        }
        fprintf(f, "\t%ff,\n", value);
    }
    free(buf);
}

int main(int argc, char* argv[]) {

    initPercfiles();

    FILE* f = fopen("cmidiperc.pch", "w");
    if(f) {
        fprintf(f, "#define CMIDI_PERC_SAMPLE_SIZE %d\n", CMIDI_PERC_SAMPLE_SIZE);
        for(int i = 0; i < 128; i++) {
            char* filename = percfiles[i];
            if(filename) {
                fprintf(f, "float cmidi_perc_%d[CMIDI_PERC_SAMPLE_SIZE] = {\n", i);
                printfloats(f, filename);
                fprintf(f, "}; //cmidi_perc_%d\n", i);
            }
        }
        fprintf(f, "float cmidi_perc_silence[CMIDI_PERC_SAMPLE_SIZE] = {0};\n");
        fprintf(f, "float* cmidi_perc_table[128] = {\n");
        for(int i = 0; i < 128; i++) {
            char* filename = percfiles[i];
            if(filename) {
                fprintf(f, "\tcmidi_perc_%d,\t\t\t//%d\n", i, i);
            } else {
                fprintf(f, "\tcmidi_perc_silence,\t\t//%d\n", i);
            }
        }
        fprintf(f, "};\n");
        fclose(f);
    }

    return 0;
}
