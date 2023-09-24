#ifndef _CMIDIPERC_H_
    #define _CMIDIPERC_H_

#include "cmidiprograms.h"

///cmidiperc.pch is generated via the PCH build target!
///See pchgen.c
#include "cmidiperc.pch"

#ifdef cmidiperc_pch_is_missing

#define CMIDI_PERC_SAMPLE_SIZE 44100
float cmidi_perc_silence[CMIDI_PERC_SAMPLE_SIZE] = {0};
float* cmidi_perc_table[128] = {
    cmidi_perc_silence,             //0
    cmidi_perc_silence,             //1
    cmidi_perc_silence,             //2
    cmidi_perc_silence,             //3
    cmidi_perc_silence,             //4
    cmidi_perc_silence,             //5
    cmidi_perc_silence,             //6
    cmidi_perc_silence,             //7

    cmidi_perc_silence,             //8
    cmidi_perc_silence,             //9
    cmidi_perc_silence,             //10
    cmidi_perc_silence,             //11
    cmidi_perc_silence,             //12
    cmidi_perc_silence,             //13
    cmidi_perc_silence,             //14
    cmidi_perc_silence,             //15

    cmidi_perc_silence,             //16
    cmidi_perc_silence,             //17
    cmidi_perc_silence,             //18
    cmidi_perc_silence,             //19
    cmidi_perc_silence,             //20
    cmidi_perc_silence,             //21
    cmidi_perc_silence,             //22
    cmidi_perc_silence,             //23

    cmidi_perc_silence,             //24
    cmidi_perc_silence,             //25
    cmidi_perc_silence,             //26
    cmidi_perc_silence,             //27
    cmidi_perc_silence,             //28
    cmidi_perc_silence,             //29
    cmidi_perc_silence,             //30
    cmidi_perc_silence,             //31

    cmidi_perc_silence,             //32
    cmidi_perc_silence,             //33
    cmidi_perc_silence,             //34
    cmidi_perc_silence,             //35
    cmidi_perc_silence,             //36
    cmidi_perc_silence,             //37
    cmidi_perc_silence,             //38
    cmidi_perc_silence,             //39

    cmidi_perc_silence,             //40
    cmidi_perc_silence,             //41
    cmidi_perc_silence,             //42
    cmidi_perc_silence,             //43
    cmidi_perc_silence,             //44
    cmidi_perc_silence,             //45
    cmidi_perc_silence,             //46
    cmidi_perc_silence,             //47

    cmidi_perc_silence,             //48
    cmidi_perc_silence,             //49
    cmidi_perc_silence,             //50
    cmidi_perc_silence,             //51
    cmidi_perc_silence,             //52
    cmidi_perc_silence,             //53
    cmidi_perc_silence,             //54
    cmidi_perc_silence,             //55

    cmidi_perc_silence,             //56
    cmidi_perc_silence,             //57
    cmidi_perc_silence,             //58
    cmidi_perc_silence,             //59
    cmidi_perc_silence,             //60
    cmidi_perc_silence,             //61
    cmidi_perc_silence,             //62
    cmidi_perc_silence,             //63

    cmidi_perc_silence,             //64
    cmidi_perc_silence,             //65
    cmidi_perc_silence,             //66
    cmidi_perc_silence,             //67
    cmidi_perc_silence,             //68
    cmidi_perc_silence,             //69
    cmidi_perc_silence,             //70
    cmidi_perc_silence,             //71

    cmidi_perc_silence,             //72
    cmidi_perc_silence,             //73
    cmidi_perc_silence,             //74
    cmidi_perc_silence,             //75
    cmidi_perc_silence,             //76
    cmidi_perc_silence,             //77
    cmidi_perc_silence,             //78
    cmidi_perc_silence,             //79

    cmidi_perc_silence,             //80
    cmidi_perc_silence,             //81
    cmidi_perc_silence,             //82
    cmidi_perc_silence,             //83
    cmidi_perc_silence,             //84
    cmidi_perc_silence,             //85
    cmidi_perc_silence,             //86
    cmidi_perc_silence,             //87

    cmidi_perc_silence,             //88
    cmidi_perc_silence,             //89
    cmidi_perc_silence,             //90
    cmidi_perc_silence,             //91
    cmidi_perc_silence,             //92
    cmidi_perc_silence,             //93
    cmidi_perc_silence,             //94
    cmidi_perc_silence,             //95

    cmidi_perc_silence,             //96
    cmidi_perc_silence,             //97
    cmidi_perc_silence,             //98
    cmidi_perc_silence,             //99
    cmidi_perc_silence,             //100
    cmidi_perc_silence,             //101
    cmidi_perc_silence,             //102
    cmidi_perc_silence,             //103

    cmidi_perc_silence,             //104
    cmidi_perc_silence,             //105
    cmidi_perc_silence,             //106
    cmidi_perc_silence,             //107
    cmidi_perc_silence,             //108
    cmidi_perc_silence,             //109
    cmidi_perc_silence,             //110
    cmidi_perc_silence,             //111

    cmidi_perc_silence,             //112
    cmidi_perc_silence,             //113
    cmidi_perc_silence,             //114
    cmidi_perc_silence,             //115
    cmidi_perc_silence,             //116
    cmidi_perc_silence,             //117
    cmidi_perc_silence,             //118
    cmidi_perc_silence,             //119

    cmidi_perc_silence,             //120
    cmidi_perc_silence,             //121
    cmidi_perc_silence,             //122
    cmidi_perc_silence,             //123
    cmidi_perc_silence,             //124
    cmidi_perc_silence,             //125
    cmidi_perc_silence,             //126
    cmidi_perc_silence,             //127
};
#endif

#endif // _CMIDIPERC_H_
