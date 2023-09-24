# CMidi

CMidi (CUH-MEE-DEE) is C-based MIDI file synthesizer.

# Documentation

Unfortunately there is not much documentation at this point in time but I'll still try and point you in the right direction.

Currently the best way to learn how to use CMidi is to study `demo.c` and `main.c`.

### demo.c

This file shows you how to use CMidi using only the standard C library to generate WAV files from MIDI files.

### main.c

This file shows you how to integrate CMidi with SDL2 and even how to implement a rudementary SFX system.

## Usage

Here's an incomplete program that shows the more important things you should know.

```C
#include "cmidi.h"
#include "cmidiprograms.h"

int main(int argc, char* argv[]) {

  //Tell cmidi we'll only be using the "print" command.
  cmidi_init(CMIDI_INIT_PRINT); 

  //load the midi file we want to play.
  struct cmidi_Song* song = cmidi_load("song.mid");

  //lower the volume so there's no clipping
  cmidi_setVolume(0.3f);

  do {
    //create a buffer for our samples and zero it out.
    //we have to do this because cmidi_read adds to the existing values stored in the buffer.
    //see `main.c` to see why that could be useful.
    float buffer[1000];
    for(int i = 0; i < 1000; i++) {
      buffer[i] = 0.0f;
    }

    //populate the buffer with 500 samples (advancing the song)
    cmidi_read(song, buffer, 500);  //500 because the buffer will be filled
                                    //with interlaced stereo samples.
                                    //500 pairs of left and right channels samples.
                                    //(Which comes to 1000 floats to fill the buffer)

    //do something with the samples
    processBuffer(buffer, 1000);
    
  } while(!cmidi_eot(song));  //keep processing samples until the song's done!

  cmidi_quit(); //call this when we're done using CMidi!
  ...

  return 0;
}

```

## Commands

With CMidi you can use the MIDI Text event in MIDI files to trigger function calls in C. CMidi comes with 2 built in commands:

- goto [marker]
- print [text]

A command consists of an identifier and parameters: `identifier [parameters]`

You can register your own commands using the macro `cmidi_register(identifier, cmidi_Func)`.

You could imagine a custom user command that triggers the next stage in a boss fight: `set_boss_stage 2`

An `identifier` must not contain a space. The space is used to separate the `identifier` from the `[parameters]`.

## Markers

CMidi makes use of MIDI Marker events. Whenever a marker is passed when reading samples using cmidi_read any internal state needed to restore the song to that point is stashed. Use the `goto` command to implement song loops along with MIDI Marker events.