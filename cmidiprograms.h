#ifndef CMIDIPROGRAMS_H_
    #define CMIDIPROGRAMS_H_

#include "cmidi.h"

extern struct cmidi_Program cmidi_Sin;
extern struct cmidi_Program cmidi_Sin2;
extern struct cmidi_Program cmidi_Triangle;
extern struct cmidi_Program cmidi_Saw;
extern struct cmidi_Program cmidi_Square;
extern struct cmidi_Program cmidi_Square2;

extern struct cmidi_Program cmidi_Test;

extern struct cmidi_Program cmidi_Snare;
extern struct cmidi_Program cmidi_Snare2;

extern struct cmidi_Program cmidi_Kick;

extern struct cmidi_Program cmidi_Perc;

struct cmidi_SampleData {
    float* monoSampleData;
    int numSamples;
    int loop;
    struct cmidi_ADSR adsr;
};

void cmidi_openSampler(struct cmidi_Program* program, float* monoSampleData, int numSamples, int loop, float freq, cmidi_Program_Filter filter, struct cmidi_ADSR* adsr);
void cmidi_closeSampler(struct cmidi_Program* program);

void cmidi_filter_adsr(struct cmidi_Voice* v, float* out, int len, float (*generator)(struct cmidi_Voice*, float), struct cmidi_ADSR* adsr);
void cmidi_filter_adsr_nohold(struct cmidi_Voice* v, float* out, int len, float (*generator)(struct cmidi_Voice*, float), struct cmidi_ADSR* adsr);
void cmidi_filter_adsr_nohold_norelease(struct cmidi_Voice* v, float* out, int len, float (*generator)(struct cmidi_Voice*, float), struct cmidi_ADSR* adsr);
void cmidi_filter_perc(struct cmidi_Voice* v, float* out, int len, float (*generator)(struct cmidi_Voice*, float), struct cmidi_ADSR* adsr);

#endif // CMIDIPROGRAMS_H_
