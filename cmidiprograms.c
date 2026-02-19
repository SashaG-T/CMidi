#include "cmidiprograms.h"
#include "bell.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "cmidiperc.h"

#define PI (3.14159)
#define TAU (2.0*PI)

//built in programs
void cmidi_program_sin(struct cmidi_Voice* v, float* out, int len);
void cmidi_program_sin2(struct cmidi_Voice* v, float* out, int len);
void cmidi_program_triangle(struct cmidi_Voice* v, float* out, int len);
void cmidi_program_saw(struct cmidi_Voice* v, float* out, int len);
void cmidi_program_square(struct cmidi_Voice* v, float* out, int len);
void cmidi_program_square2(struct cmidi_Voice* v, float* out, int len);
void cmidi_program_string(struct cmidi_Voice* v, float* out, int len);
void cmidi_program_test(struct cmidi_Voice* v, float* out, int len);
void cmidi_program_snare(struct cmidi_Voice* v, float* out, int len);
void cmidi_program_snare2(struct cmidi_Voice* v, float* out, int len);
void cmidi_program_kick(struct cmidi_Voice* v, float* out, int len);
void cmidi_program_perc(struct cmidi_Voice* v, float* out, int len);
void cmidi_program_sampler(struct cmidi_Voice* v, float* out, int len);

//built in controllers
void cmidi_program_controller(struct cmidi_Program* p, unsigned char controller, unsigned char value);

//built in pre-constructed cmidi_Programs
struct cmidi_Program cmidi_Sin      = {cmidi_program_sin,      cmidi_program_controller, cmidi_filter_adsr,        0.707106f, 0.707106f, 1.0f, 1.0f, 0};
struct cmidi_Program cmidi_Sin2     = {cmidi_program_sin2,     cmidi_program_controller, cmidi_filter_adsr_nohold_norelease, 0.707106f, 0.707106f, 1.0f, 1.0f, 0};
struct cmidi_Program cmidi_Triangle = {cmidi_program_triangle, cmidi_program_controller, cmidi_filter_adsr,        0.707106f, 0.707106f, 1.0f, 1.0f, 0};
struct cmidi_Program cmidi_Saw      = {cmidi_program_saw,      cmidi_program_controller, cmidi_filter_adsr,        0.707106f, 0.707106f, 1.0f, 1.0f, 0};
struct cmidi_Program cmidi_Square   = {cmidi_program_square,   cmidi_program_controller, cmidi_filter_adsr,        0.707106f, 0.707106f, 1.0f, 1.0f, 0};
struct cmidi_Program cmidi_Square2  = {cmidi_program_square2,  cmidi_program_controller, cmidi_filter_adsr,        0.707106f, 0.707106f, 1.0f, 1.0f, 0};
struct cmidi_Program cmidi_Test     = {cmidi_program_test,     cmidi_program_controller, cmidi_filter_adsr,        0.707106f, 0.707106f, 1.0f, 1.0f, 0};
struct cmidi_Program cmidi_Snare    = {cmidi_program_snare,    cmidi_program_controller, cmidi_filter_adsr_nohold_norelease, 0.707106f, 0.707106f, 1.0f, 1.0f, 0};
struct cmidi_Program cmidi_Snare2   = {cmidi_program_snare2,   cmidi_program_controller, cmidi_filter_adsr_nohold_norelease, 0.707106f, 0.707106f, 1.0f, 1.0f, 0};
struct cmidi_Program cmidi_Kick     = {cmidi_program_kick,     cmidi_program_controller, cmidi_filter_adsr_nohold_norelease, 0.707106f, 0.707106f, 1.0f, 1.0f, 0};
struct cmidi_Program cmidi_Perc     = {cmidi_program_perc,     cmidi_program_controller, cmidi_filter_perc,        0.707106f, 0.707106f, 1.0f, 1.0f, 0};
struct cmidi_Program cmidi_Sampler  = {cmidi_program_sampler,  cmidi_program_controller, cmidi_filter_adsr,        0.707106f, 0.707106f, 1.0f, 1.0f, 0};

struct cmidi_ADSR cmidi_adsr_default = {480, 480, 0.5f, 10000};

void cmidi_openSampler(struct cmidi_Program* program, float* monoSampleData, int numSamples, int loop, float freq, cmidi_Program_Filter filter, struct cmidi_ADSR* adsr) {
    if(program) {
        *program = cmidi_Sampler;
        struct cmidi_SampleData* user = malloc(sizeof(struct cmidi_SampleData));
        if(user) {
            user->monoSampleData = monoSampleData;
            user->numSamples = numSamples;
            user->loop = loop;
            program->freq = freq;
            user->adsr = adsr ? *adsr : cmidi_adsr_default;
            program->user = user;
            if(filter) {
                program->filter = filter;
            }
        } else {
            puts("cmidi: sampler memory allocation failed.");
        }
    } else {
        puts("cmidi: could not open sampler; program parameter shouldn't be null!");
    }
}

void cmidi_closeSampler(struct cmidi_Program* program) {
    free(program->user);
}

void cmidi_program_controller(struct cmidi_Program* p, unsigned char controller, unsigned char value) {
    switch(controller) {
        case 0x07: {
            p->vol = (float)value / (float)0x7F;
            break;
        }
        case 0x0A: {
            float pan = (float)value / (float)0x7F;
            p->lPan = sqrtf(1.0f-pan);
            p->rPan = sqrtf(pan);
            break;
        }
    }
}

static double noteToFreq(double step) {
    return (pow(2, step / 12.0) * 27.5) / 2.0;
}

static float sinGenerator(struct cmidi_Voice* v, float x) {
    return sin(x*TAU);
}

static float trianglePulse = 0.5f;

static float triangleGenerator(struct cmidi_Voice* v, float x) {
    x = fmod(x, 2.0f);
    if(x < trianglePulse) {
        x = x/trianglePulse;
    } else if(x < 2.0f-trianglePulse) {
        float t = (x - trianglePulse) / (2.0f - 2.0f * trianglePulse);
        x = 1.0f - 2*t;
    } else {
        x = -(1.0f - ((x-(2.0f-trianglePulse))/trianglePulse));
    }
    return x;
}

static float squarePulse = 0.5f;

static float testGenerator(struct cmidi_Voice* v, float x) {
    x += squarePulse;
    x = fmod(x, 2.0f);

    if(x < squarePulse*2.0f) {
        x = (x/squarePulse) - 1.0f;
    } else if(x < 1.0f) {
        x = 1.0f;
    } else if(x < 1.0f + squarePulse*2.0f) {
        x = ((x-1.0f)/-squarePulse) + 1.0f;
    } else {
        x = -1.0f;
    }
    return x;
}

static float squareGenerator(struct cmidi_Voice* v, float x) {
    x = fmod(x, 2.0f);
    return x < squarePulse ? -1.0f : 1.0f;
}

static float snareGenerator(struct cmidi_Voice* v, float x) {
    static unsigned long long seed = 14642743464ull;
    seed *= 723744263;
    return (float)((seed % 1000) / 1000.0f) * 2.0f - 1.0f;
}

static float snare2Generator(struct cmidi_Voice* v, float x) {
    x/=8;
    x *= 192.0f/v->timer;
    return (squareGenerator(v, x) + snareGenerator(v, x));
}

static float kickGenerator(struct cmidi_Voice* v, float x) {
    float t = (v->timer / 2000.0f);
    if(t > 1.0f) {
        t = 1.0f;
    }
    float k = x*t*0.1f;
    return sin((x*TAU)+x*t);
}

static float percGenerator(struct cmidi_Voice* v, float x) {
    float* sample = v->program->user;
    float value = 0.0f;
    if(v->timer < CMIDI_PERC_SAMPLE_SIZE) {
        value = sample[v->timer];
    }
    return value;
}

static float sampleGenerator(struct cmidi_Voice* v, float x) {
    struct cmidi_SampleData* user = v->program->user;
    if(x > user->numSamples) {
        x = fmod(x - user->loop, user->numSamples - user->loop) + user->loop;
    }
    float value = 0.0f;
    unsigned _x = ((x - (unsigned)x) * 256);
    unsigned bellIdx[] = {255 - _x, 511 - _x, 256 + _x, _x};
    for(int j = 0; j < 4; j++) {
        unsigned o = (((unsigned)x + j));
        if(o >= user->numSamples) {
            o = user->loop + (o % user->numSamples);
        }
        float bell = (float)bellCurve[bellIdx[j]];
        value += user->monoSampleData[o] * bell;
    }
    return value;
}

void cmidi_filter_perc(struct cmidi_Voice* v, float* out, int len, float (*generator)(struct cmidi_Voice*, float), struct cmidi_ADSR* adsr) {
    for(int i = 0; i < len*2; i+=2) {
        float sample = generator(v, 0.0f);
        out[i] += sample * v->program->lPan * v->program->vol * v->vel;
        out[i+1] += sample * v->program->rPan * v->program->vol * v->vel;
        v->timer++;
        if(v->timer >= CMIDI_PERC_SAMPLE_SIZE) {
            cmidi_freeVoice(v);
            break;
        }
    }
}

void cmidi_filter_adsr(struct cmidi_Voice* v, float* out, int len, float (*generator)(struct cmidi_Voice*, float), struct cmidi_ADSR* adsr) {
    struct cmidi_ADSR _adsr = adsr ? *adsr : cmidi_adsr_default;
    unsigned a = _adsr.a;
    unsigned d = _adsr.d;
    float s = _adsr.s;
    unsigned r = _adsr.r;
    float shift = ((noteToFreq((float)v->note) * v->program->freq * v->channel->pitch) / (double)cmidi_audioFreq());
    if(v->count) {
        for(int i = 0; i < len*2; i+=2) {
            float k;
            int t = v->timer + (i/2);
            if(t > a + d) {
                k = s;
            } else if(t > a) {
                float _t = (t - a) / (float)d;
                k = (1.0f - _t) + s * _t;
            } else {
                k = t / (float)a;
            }
            float sample = k * generator(v, v->offset + (i/2)*shift);
            out[i] += sample * v->program->lPan * v->program->vol * v->vel;
            out[i+1] += sample * v->program->rPan * v->program->vol * v->vel;
        }
        v->timer += len;
    } else {
        for(int i = 0; i < len*2; i+=2) {
            int t = v->timer - v->rel + (i/2);
            float k = t / (float)r;
            if(k > 1.0f) {
                k = 1.0f;
            }
            float sample = (s * (1.0f - k)) * generator(v, v->offset + (i/2)*shift);
            out[i] += sample * v->program->lPan * v->program->vol * v->vel;
            out[i+1] += sample * v->program->rPan * v->program->vol * v->vel;
        }
        v->timer += len;
        if(v->timer - v->rel >= r) {
            cmidi_freeVoice(v);
        }
    }
    v->offset += len * shift;
}

void cmidi_filter_adsr_nohold(struct cmidi_Voice* v, float* out, int len, float (*generator)(struct cmidi_Voice*, float), struct cmidi_ADSR* adsr) {
    struct cmidi_ADSR _adsr = adsr ? *adsr : cmidi_adsr_default;
    unsigned a = _adsr.a;
    unsigned d = _adsr.d;
    float s = _adsr.s;
    unsigned r = _adsr.r;
    float shift = ((noteToFreq((float)v->note) * v->program->freq * v->channel->pitch) / (double)cmidi_audioFreq());
    for(int i = 0; i < len*2; i+=2) {
        float k;
        int t = v->timer + (i/2);
        if(t > a + d) {
            k = (t - (a + d)) / (float)r;
            if(k > 1.0f) {
                k = 1.0f;
            }
            k = s * (1.0f - k);
        } else if(t > a) {
            k = (t - a) / (float)d;
            k = (1.0f - k) + s * k;
        } else {
            k = t / (float)a;
        }
        float sample = k * generator(v, v->offset + (i/2)*shift);
        out[i] += sample * v->program->lPan * v->program->vol * v->vel;
        out[i+1] += sample * v->program->rPan * v->program->vol * v->vel;
    }
    v->timer += len;
    v->offset += len * shift;
    if(v->timer - v->rel >= r) {
        cmidi_freeVoice(v);
    }
}

void cmidi_filter_adsr_nohold_norelease(struct cmidi_Voice* v, float* out, int len, float (*generator)(struct cmidi_Voice*, float), struct cmidi_ADSR* adsr) {
    struct cmidi_ADSR _adsr = adsr ? *adsr : cmidi_adsr_default;
    unsigned a = _adsr.a;
    unsigned d = _adsr.d;
    float s = _adsr.s;
    unsigned r = _adsr.r;
    float shift = ((noteToFreq((float)v->note) * v->program->freq * v->channel->pitch) / (double)cmidi_audioFreq());
    for(int i = 0; i < len*2; i+=2) {
        float k;
        int t = v->timer + (i/2);
        if(t > a + d) {
            k = (t - (a + d)) / (float)r;
            if(k > 1.0f) {
                k = 1.0f;
            }
            k = s * (1.0f - k);
        } else if(t > a) {
            k = (t - a) / (float)d;
            k = (1.0f - k) + s * k;
        } else {
            k = t / (float)a;
        }
        float sample = k * generator(v, v->offset + (i/2)*shift);
        out[i] += sample * v->program->lPan * v->program->vol * v->vel;
        out[i+1] += sample * v->program->rPan * v->program->vol * v->vel;
    }
    v->timer += len;
    v->offset += len * shift;
    if(v->timer >= r) {
        cmidi_freeVoice(v);
    }
}

void cmidi_program_sin(struct cmidi_Voice* v, float* out, int len) {
    struct cmidi_ADSR adsr = {480, 1000, 0.3f, 2000};
    v->program->filter(v, out, len, sinGenerator, &adsr);
}

void cmidi_program_sin2(struct cmidi_Voice* v, float* out, int len) {
    struct cmidi_ADSR adsr = {960, 4800, 0.1f, 40000};
    v->program->filter(v, out, len, sinGenerator, &adsr);
}

void cmidi_program_triangle(struct cmidi_Voice* v, float* out, int len) {
    struct cmidi_ADSR adsr = {200, 2000, 0.3f, 2000};
    trianglePulse = 0.5f;
    v->program->filter(v, out, len, triangleGenerator, &adsr);
}

void cmidi_program_saw(struct cmidi_Voice* v, float* out, int len) {
    struct cmidi_ADSR adsr = {3400, 4800, 0.5f, 4800};
    trianglePulse = 0.1f;
    v->program->filter(v, out, len, triangleGenerator, &adsr);
}

void cmidi_program_square(struct cmidi_Voice* v, float* out, int len) {
    struct cmidi_ADSR adsr = {960, 480, 0.3f, 1000};
    squarePulse = 1.0f;
    v->program->filter(v, out, len, squareGenerator, &adsr);
}

void cmidi_program_square2(struct cmidi_Voice* v, float* out, int len) {
    struct cmidi_ADSR adsr = {240, 2000, 0.3f, 4800};
    squarePulse = 0.3f;
    v->program->filter(v, out, len, squareGenerator, &adsr);
}

void cmidi_program_test(struct cmidi_Voice* v, float* out, int len) {
    struct cmidi_ADSR adsr = {240, 1000, 0.4f, 7800};
    squarePulse = sin(v->timer/100000.0f);
    v->program->filter(v, out, len, testGenerator, &adsr);
}

void cmidi_program_snare(struct cmidi_Voice* v, float* out, int len) {
    struct cmidi_ADSR adsr = {100, 200, 0.5f, 1000.0f};
    v->program->filter(v, out, len, snareGenerator, &adsr);
}

void cmidi_program_snare2(struct cmidi_Voice* v, float* out, int len) {
    struct cmidi_ADSR adsr = {100, 200, 0.5f, 1000.0f};
    squarePulse = 0.4f;
    v->program->filter(v, out, len, snare2Generator, &adsr);
}

void cmidi_program_kick(struct cmidi_Voice* v, float* out, int len) {
    struct cmidi_ADSR adsr = {100, 100, 0.8f, 10000.0f};
    v->note = 50;
    v->program->filter(v, out, len, kickGenerator, &adsr);
}

void cmidi_program_perc(struct cmidi_Voice* v, float* out, int len) {
    if(!v->timer) {
        v->program->user = cmidi_perc_table[v->note];
    }
    v->program->filter(v, out, len, percGenerator, 0);
}

void cmidi_program_sampler(struct cmidi_Voice* v, float* out, int len) {
    struct cmidi_SampleData* user = v->program->user;
    v->program->filter(v, out, len, sampleGenerator, &user->adsr);
}
