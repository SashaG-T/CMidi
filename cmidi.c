#include "cmidi.h"
#include <stdio.h>
#include <stdlib.h>
#include "cmidiprograms.h"
#include "hash.h"
#include <string.h>
#include <assert.h>
#include <math.h>

static float masterVolume = 0.75f;

inline int cmidi_audioFreq() {
    return 48000;
}

struct HashTable cmidi_markerDictionary = {0}; //there should be a table for each song
struct HashTable cmidi_funcDictionary = {0};

struct cmidi_Voice voice[256] = {0};
unsigned char nextVoice = 0;
unsigned char voiceQueueBegin = 0;
unsigned char voiceQueueEnd = 0;
struct cmidi_Voice* voiceQueue[256] = {0};
struct cmidi_Channel cmidi_channel_default;
struct cmidi_Patch cmidi_patch_default = {{0}};
//struct cmidi_Patch cmidi_patch_gm; //maybe one day...
struct cmidi_Device cmidi_device_default = {"Default Device", 0, &cmidi_patch_default, {}};
struct cmidi_Device* portDevice[256] = {0};

struct cmidi_Voice* cmidi_getVoiceTable() {
    return voice;
};

struct cmidi_Device* cmidi_openDevice(const char* name, struct cmidi_Patch* patch) {
    struct cmidi_Device* device = calloc(1, sizeof(struct cmidi_Device));
    if(device) {
        const char* _name = name == 0 ? "Unnamed Device" : name;
        struct cmidi_Patch* _patch = patch == 0 ? &cmidi_patch_default : patch;
        int i = 0;
        for(; i < 256 && (i < 1 || _name[i-1] != 0); i++) {
            device->name[i] = _name[i];
        }
        device->name[255] = 0;
        device->patch = _patch;
        device->refCount = 1;
        for(int channel = 0; channel < 16; channel++) {
            device->channel[channel].pitch = 1.0f;
        }
    }
    #ifdef CMIDI_DEBUG
    else {
        puts("cmidi: failed to allocate device memory.");
    }
    #endif
    return device;
}

void cmidi_useDevice(struct cmidi_Track* track, struct cmidi_Device* device) {
    if(track) {
        if(device) {
            if(device->refCount > 0) {
                device->refCount++;
                track->device = device;
    #ifdef CMIDI_DEBUG
            } else {
                printf("cmidi: device->refCount too low: %d\n", device->refCount);
            }
        } else {
            puts("cmidi: can't use null device.");
        }
    } else {
        puts("cmidi: can't use null track.");
    }
    #else
            }
        }
    }
    #endif
}

void cmidi_closeDevice(struct cmidi_Device* device) {
    if(device) {
        if(device->refCount > 0) {
            //object is a allocated on the heap
            device->refCount--;
            if(device->refCount == 0) {
                //object is no longer used by anything
                free(device);
            }
        }
    #ifdef CMIDI_DEBUG
    } else {
        puts("cmidi: can't close null device.");
    #endif
    }

}

void cmidi_setPort(unsigned char port, struct cmidi_Device* device) {
    cmidi_closeDevice(portDevice[port]);
    portDevice[port] = device;
}

struct cmidi_Voice* cmidi_getNextVoice() {
    struct cmidi_Voice* v;
    if(voiceQueueBegin == voiceQueueEnd) {
        v = &voice[nextVoice++];
        nextVoice %= 256; //should overflow but I think it's best to be explicit.
    } else {
        v = voiceQueue[voiceQueueBegin++];
        voiceQueueBegin %= 256; //should overflow but I think it's best to be explicit.
    }
    v->count = 0;
    return v;
}

static float tick2Sample(struct cmidi_Song* song) {
    float microPerTick = song->tempo / (float)song->division;
    float tickPerSecond = 1000000 / (float)microPerTick;
    float samplesPerTick = song->rate / (float)tickPerSecond;
    #ifdef CMIDI_DEBUG
    printf("samplesPerTick: %f\n", samplesPerTick);
    #endif
    return samplesPerTick;
}

void cmidi_freeVoice(struct cmidi_Voice* v) {
    if(v->channel && v->channel->voices[v->note] == v) {
        v->channel->voices[v->note] = 0;
    }
    v->count = 0;
    v->channel = 0;
    v->program = 0;
    v->song = 0;
    voiceQueue[voiceQueueEnd++] = v;
    voiceQueueEnd %= 256; //should overflow but I think it's best to be explicit.
}

void cmidi_setPatchProgram(struct cmidi_Patch* patch, int programId, struct cmidi_Program* program) {
    if(!patch) {
        patch = &cmidi_patch_default;
    }
    patch->program[programId] = program;
}

void cmidi_setVolume(float volume) {
    masterVolume = volume;
}

void cmidi_setDevice(struct cmidi_Song* song, struct cmidi_Device* device) {
    assert(song != 0);
    device = device ? device : &cmidi_device_default;
    cmidi_panicStopSong(song);
    for(int i = 0; i < song->ntrks; i++) {
        struct cmidi_Track* track = &song->track[i];
        if(track) {
            cmidi_useDevice(track, device);
        }
    }
}

//we can do this because the byte order is specified by the MIDI spec
static unsigned readUint(char* data) {
    unsigned char* d = (unsigned char*)data;
    return ((d[0]) << 24) | (d[1] << 16) | (d[2] << 8) | (d[3]);
}

//we can do this because the byte order is specified by the MIDI spec
static short readUshort(char* data) {
    unsigned char* d = (unsigned char*)data;
    return (d[0] << 8) | (d[1]);
}

static inline unsigned char cmidi_next(unsigned char** event) {
    unsigned char n = **event;
    (*event)++;
    return n;
}

static inline unsigned char cmidi_Track_next(struct cmidi_Track* track) {
    assert(track->next < track->end);
    return cmidi_next(&track->next);
}

static unsigned cmidi_getLen(unsigned char** event) {
    register unsigned long value = cmidi_next(event);
    register unsigned char c;
    if(value & 0x80) {
        value &= 0x7F;
        do {
            c = cmidi_next(event);
            value = (value << 7) + (c & 0x7F);
        } while(c & 0x80);
    }
    return value;
}

static unsigned cmidi_Track_getLen(struct cmidi_Track* track) {
    return cmidi_getLen(&track->next);
}

static void cmidi_Channel_noteOn(struct cmidi_Song* song, struct cmidi_Track* track, struct cmidi_Channel* channel, unsigned char note, unsigned char vel) {
    #ifdef CMIDI_DEBUG
    printf("note on: %d\t\t(%s)\n", note, track->name);
    #endif
    struct cmidi_Voice* v = channel->voices[note];
    if(!v) {
        v = cmidi_getNextVoice(); //only get a new voice if the channel isn't already using one for this specific note!
    }
    v->song = song;
    v->count++;
    v->channel = channel;
    v->program = track->device->patch->program[channel->program];
    v->note = note;
    v->timer = 0;
    v->offset = 0;
    v->vel = (float)vel / (float)0x7F;
    v->channel->voices[note] = v;
}

static void cmidi_Channel_noteOff(struct cmidi_Channel* channel, unsigned char note, unsigned char vel) {
    #ifdef CMIDI_DEBUG
    printf("note off: %d\n", note);
    #endif
    struct cmidi_Voice* v = channel->voices[note];
    if(v && !(--v->count)) {
        channel->voices[v->note] = 0;
        v->rel = v->timer;
    }
}

static int cmidi_cmpMarker(void* m1, void* m2) {
    return strcmp(m1, m2);
}

static unsigned int cmidi_hashMarker(void* key) {
    char* k = key;
    int hashval;
    for (hashval = 0; *k != '\0'; k++) {
        hashval = *k + 31 * hashval;
    }
    return hashval;
}

static int cmidi_onRemoveMarker(struct HashPair* pair) {
    free(pair->key);
    free(pair->element);
}

static void cmidi_processText(struct cmidi_Song* song, struct cmidi_Track* track, unsigned long len) {
    assert(track->next < track->end);
    char* textStart = track->next;
    track->next += len;
    if(HashTable_Ready(&cmidi_funcDictionary)) {
        char* params = textStart;
        char* textEnd = textStart + len;
        while(*params != 0x20 && params != textEnd) {
            params++;
        }
        char oldChar = *params;
        *params = 0;
        cmidi_Func* func = HashTable_Has(&cmidi_funcDictionary, textStart);
        *params = oldChar;
        params++;
        if(func && *func) {
            oldChar = textStart[len];
            textStart[len] = 0;
            (*func)(song, track, textEnd == params ? 0 : params);
            textStart[len] = oldChar;
        }
    } else {
        puts("funcDict not ready!");
    }
}

static void cmidi_processMarker(struct cmidi_Song* song, struct cmidi_Track* track, unsigned long len) {
    assert(track->next < track->end);
    if(!HashTable_Ready(&cmidi_markerDictionary)) {
        HashTable_Init(&cmidi_markerDictionary, cmidi_cmpMarker, cmidi_hashMarker, cmidi_onRemoveMarker);
    }
    char* text = track->next;
    char oldChar = text[len];
    text[len] = 0;
    struct cmidi_SongStash** songStash = HashTable_Has(&cmidi_markerDictionary, text);
    if(songStash) {
        //marker already exists!!! D:
        printf("Song marker \"%s\" already exists...", text);
        text[len] = oldChar;
        return;
    }
    char* key = strdup(text);
    songStash = HashTable_At(&cmidi_markerDictionary, key);
    text[len] = oldChar;
    #ifdef CMIDI_DEBUG
    printf("Marker: %s\n", key);
    //printf("%x\t%x\n", songStash, *songStash);
    #endif
    if(!(*songStash)) {
        struct cmidi_SongStash* stash = malloc(sizeof(struct cmidi_SongStash) + sizeof(struct cmidi_TrackStash) * song->ntrks);
        if(stash) {
            stash->nextTrack = song->nextTrack;
            for(int i = 0; i < song->ntrks; i++) {
                struct cmidi_Track* _track = &song->track[i];
                struct cmidi_TrackStash* s = &stash->stash[i];
                s->delta = _track->delta < track->delta ? 0 : _track->delta - track->delta;
                s->last = _track->last;
                s->next = _track == track ? track->currEvent : _track->next;
                s->runningStatus = _track == track ? 0 : _track->runningStatus;
                #ifdef CMIDI_DEBUG
                printf("track[%d] {\n\t%d\n\t%d\n\t%d\n\t%d\n}\n", i, s->delta, s->last, s->next, s->runningStatus);
                #endif
            }
        }
        #ifdef CMIDI_DEBUG
        else {
            puts("malloc failed.");
        }
        #endif // CMIDI_DEBUG
        *songStash = stash;

    #ifdef CMIDI_DEBUG
    printf("%x\t%x\n", stash, *songStash);
    #endif
    }
}

static void cmidi_Track_processEvent(struct cmidi_Song* song, struct cmidi_Track* track) {
    assert(track->next < track->end);
    track->currEvent = track->next;
    unsigned char d = (!(*(track->next) & 0x80) && track->runningStatus) ? track->runningStatus : cmidi_Track_next(track);
    if(!track->device && ((d & 0xF0) != 0xF0)) {
        ///open a new device... there must be a better way to do this. Maybe fast forward all tracks until they all ready non-zero delta times
        ///during midi file loading?
        track->device = cmidi_openDevice(0, 0);
    }
    switch(d & 0xF0) {
        case 0x80: {
            //note off!
            track->runningStatus = d;
            unsigned char channel = (d & 0x0F);
            unsigned char note = (cmidi_Track_next(track) & 0x7F);
            unsigned char vel = (cmidi_Track_next(track) & 0x7F);
            cmidi_Channel_noteOff(&track->device->channel[channel], note, vel);
            break;
        }
        case 0x90: {
            //note on!
            track->runningStatus = d;
            unsigned char channel = (d & 0x0F);
            unsigned char note = (cmidi_Track_next(track) & 0x7F);
            unsigned char vel = (cmidi_Track_next(track) & 0x7F);
            //printf("90 %x %x %x\n", channel, note, vel);
            if(vel) {
                cmidi_Channel_noteOn(song, track, &track->device->channel[channel], note, vel);
            } else {
                cmidi_Channel_noteOff(&track->device->channel[channel], note, vel);
            }
            break;
        }
        case 0xA0: {
            //polyphonic key pressure...
            track->runningStatus = d;
            unsigned char channel = (d & 0x0F);
            unsigned char note = (cmidi_Track_next(track) & 0x7F); //note
            unsigned char presure = (cmidi_Track_next(track) & 0x7F); //presure
            break;
        }
        case 0xB0: {
            //Control change
            track->runningStatus = d;
            unsigned char channel = (d & 0x0F);
            unsigned char controller = (cmidi_Track_next(track) & 0x7F); //controller number
            unsigned char value = (cmidi_Track_next(track) & 0x7F); //new value
            struct cmidi_Program* p = track->device->patch->program[track->device->channel[channel].program];
            if(p && p->controller) {
                p->controller(p, controller, value);
            }
            break;
        }
        case 0xC0: {
            //program change
            track->runningStatus = d;
            unsigned char channel = (d & 0x0F);
            unsigned char program = (cmidi_Track_next(track) & 0x7F);
            struct cmidi_Channel* channelPtr = &track->device->channel[channel];
            channelPtr->program = channel == 9 ? program + 128 : program;
            #ifdef CMIDI_DEBUG
            printf("Program Change: %s - Channel %u - Program %u\n", track->name, channel, program);
            #endif
            break;
        }
        case 0xD0: {
            //channel pressure
            track->runningStatus = d;
            unsigned char channel = (d & 0x0F);
            unsigned char pressure = cmidi_Track_next(track); //pressure
            #ifdef CMIDI_DEBUG
            printf("pressure: %u\tchannel: %u\n", pressure, channel);
            #endif
            break;
        }
        case 0xE0: {
            //pitch wheel change
            track->runningStatus = d;
            unsigned char channel = (d & 0x0F);
            unsigned lsb = (cmidi_Track_next(track) & 0x7F);
            unsigned msb = (cmidi_Track_next(track) & 0x7F);
            unsigned pitch = ((msb << 7) | lsb);
            float _p = (((float)pitch - (float)0x2000) / (float)0x2000);
            struct cmidi_Channel* channelPtr = &track->device->channel[channel];
            channelPtr->pitch = powf(2, _p);
            #ifdef CMIDI_DEBUG
            printf("pitch: %u\tchannel: %u\n", pitch, channel);
            #endif
            break;
        }
        case 0xF0: {
            switch(d & 0x0F) {
                case 0x07:   //sysex continuation or escaped event
                case 0x00: { //system exclusive message
                    unsigned long len = cmidi_Track_getLen(track);
                    track->runningStatus = 0;
                    #ifdef CMIDI_DEBUG
                    puts("System Exclusive Event:");
                    for(int i = 0; i < len; i++) {
                        putchar(*(track->next + i));
                    }
                    putchar('\n');
                    #endif
                    track->next += len;
                    break;
                }
                case 0x0F: {
                    unsigned char metaEventType = cmidi_Track_next(track);
                    unsigned long len = cmidi_Track_getLen(track);
                    switch(metaEventType) {
                        case 0x2f: {
                            song->playing--;
                            #ifdef CMIDI_DEBUG
                            printf("Done processing track: %s\n", track->name);
                            #endif
                            break;
                        }
                        case 0x51: {
                            unsigned char c[3];
                            c[0] = cmidi_Track_next(track);
                            c[1] = cmidi_Track_next(track);
                            c[2] = cmidi_Track_next(track);
                            song->tempo = (c[0] << 16 | c[1] << 8 | c[2]);
                            song->tick2Sample = tick2Sample(song);
                            #ifdef CMIDI_DEBUG
                            printf("tempo: %u\n", song->tempo);
                            #endif
                            break;
                        }
                        case 0x58: {
                            unsigned char nn = cmidi_Track_next(track); //nn
                            unsigned char dd = cmidi_Track_next(track); //dd
                            song->clock = cmidi_Track_next(track); //cc
                            unsigned char bb = cmidi_Track_next(track); //bb;
                            #ifdef CMIDI_DEBUG
                            printf("Clock: \t%u/%u\t%u\t%u\n", nn, (int)pow(2,dd), song->clock, bb);
                            #endif
                            break;
                        }
                        case 0x59: {
                            unsigned char sf = cmidi_Track_next(track);
                            unsigned char mi = cmidi_Track_next(track);
                            #ifdef CMIDI_DEBUG
                            printf("Key Signature: \t%u\t%u\n", sf, mi);
                            #endif
                            break;
                        }
                        case 0x03: {
                            unsigned char* _next = track->next + len;
                            for(unsigned long i = 0; i < len && i < 256; i++) {
                                track->name[i] = cmidi_Track_next(track);
                            }
                            track->name[len < 256 ? len : 255] = 0;
                            track->next = _next;
                            #ifdef CMIDI_DEBUG
                            printf("Track Name: %s\n", track->name);
                            #endif
                            break;
                        }
                        case 0x21: {
                            unsigned char port = cmidi_Track_next(track);
                            cmidi_closeDevice(track->device);
                            struct cmidi_Device* device = portDevice[port];
                            if(!device) {
                                device = cmidi_openDevice(0, 0);
                                cmidi_setPort(port, device);
                            }
                            cmidi_useDevice(track, device);
                            break;
                        }
                        case 0x01: {
                            cmidi_processText(song, track, len);
                            break;
                        }
                        case 0x06: {
                            cmidi_processMarker(song, track, len);
                            track->next += len;
                            break;
                        }
                        default: {
                            //we don't care about the other meta events right now; just skip them.
                            #ifdef CMIDI_DEBUG
                            printf("Meta Event: %d\n", metaEventType);
                            for(int i = 0; i < len; i++) {
                                putchar(*(track->next + i));
                            }
                            putchar('\n');
                            #endif
                            track->next += len;
                            break;
                        }
                    }
                    break;
                }
                default: {
                    break;
                }
            }
            break;
        }
    }
}

int cmidi_loadMem(struct cmidi_Song* song, void* mem) {
    int success = 1;
    if(song) {
        unsigned char* midi = mem;
        unsigned char* header = midi;
        midi += 14;
        if(readUint(header) == 0x4D546864) {
            song->format = readUshort(&header[8]);
            song->ntrks = readUshort(&header[10]);
            song->division = readUshort(&header[12]);
            #ifdef CMIDI_DEBUG
            printf("Format:     %u\n", song->format);
            printf("ntrks:      %u\n", song->ntrks);
            printf("Division:   %u\n", song->division);
            #endif
            song->track = calloc(song->ntrks, sizeof(struct cmidi_Track));
            if(song->format > 1) {
                puts("cmidi: Warning; only format 0 and 1 are supported. The file may be processed incorrectly.");
            }
            if(song->division & 0x8000) {
                puts("cmidi: Warning; SMPTE format isn't supported. The file will be processed incorrectly.");
            }
            if(song->track) {
                song->delta = 0xFFFFFFFF;
                for(int i = 0; i < song->ntrks && success; i++) {
                    struct cmidi_Track* track = &song->track[i];
                    header = midi;
                    midi += 8;
                    if(readUint(header) == 0x4D54726B) {
                        unsigned len = readUint(&header[4]);
                        printf("cmidi: loading track: %u bytes...\n", len);
                        track->runningStatus = 0;
                        track->data = midi;
                        track->next = midi;
                        midi += len;
                        track->end = midi;
                        track->delta = cmidi_Track_getLen(track);
                        if(track->delta < song->delta) {
                            song->delta = track->delta;
                            song->nextTrack = track;
                        }
                    } else {
                        printf("cmidi: %c%c%c%c header found; skipping.\n", header[0], header[1], header[2], header[3]);
                        unsigned len = readUint(&header[4]);
                        midi += len;
                        i--;
                    }
                }
                if(!success) {
                    free(song->track);
                }
            } else {
                puts("cmidi: memory allocation failed for tracks.");
            }
        } else {
            puts("cmidi: not a midi file.");
        }
        song->tempo = 60000000 / 120;
        song->rate = 48000;
        song->clock = 100;
        song->tick2Sample = tick2Sample(song);
        song->data = 0;
        song->position = 0;
        song->playing = song->ntrks;
        song->hasJumped = 0;
        song->soloTrack = 0;
        song->onFinishedPlaying = 0;
        song->callbackSender = 0;
        song->callbackData = 0;
    } else {
        puts("cmidi: bad cmidi_Song pointer.");
    }
    return success;
}

void cmidi_freeMem(struct cmidi_Song* song) {
    if(song) {
        for(int i = 0; i < song->ntrks; i++) {
            struct cmidi_Track* track = &song->track[i];
            cmidi_closeDevice(track->device);
        }
        free(song->track);
    }
    HashTable_Destroy(&cmidi_markerDictionary);
}

struct cmidi_Song* cmidi_load(const char* filename) {
    unsigned char* midi;
    struct cmidi_Song* song = malloc(sizeof(struct cmidi_Song));
    if(song) {
        FILE* f = fopen(filename, "rb");
        if(f) {
            fseek(f, 0, SEEK_END);
            unsigned long sz = ftell(f);
            rewind(f);
            midi = malloc(sz);
            if(midi) {
                fread(midi, 1, sz, f);
                cmidi_loadMem(song, midi);
                song->data = midi;
            } else {
                free(song);
                song = 0;
                puts("cmidi: could not allocate memory for midi data.");
            }
        } else {
            free(song);
            song = 0;
            puts("cmidi: could not open file.");
        }
    } else {
        puts("cmidi: could not allocate memory for struct.");
    }
    return song;
}

void cmidi_free(struct cmidi_Song* song) {
    if(song) {
        cmidi_freeMem(song);
        if(song->data) {
            free(song->data);
        }
        free(song);
    }
}

void cmidi_noteOffAll(struct cmidi_Song* song) {
    for(int i = 0; i < 256; i++) {
        struct cmidi_Voice* v = &voice[i];
        if(v && v->song == song) {
            v->count = 0;
            v->channel->voices[v->note] = 0;
            v->rel = v->timer;
        }
    }
}

void cmidi_panicStopAll() {
    for(int i = 0; i < 256; i++) {
        struct cmidi_Voice* v = &voice[i];
        if(v->channel && v->channel->voices[v->note] == v) {
            v->channel->voices[v->note] = 0;
        }
        v->count = 0;
        v->channel = 0;
        v->program = 0;
    }
    memset(voiceQueue, 0, sizeof(voiceQueue));
    nextVoice = 0;
    voiceQueueBegin = 0;
    voiceQueueEnd = 0;
}

void cmidi_panicStopSong(struct cmidi_Song* song) {
    assert(song != 0);
    for(int i = 0; i < song->ntrks; i++) {
        struct cmidi_Device* device = song->track[i].device;
        if(device) {
            cmidi_panicStopDevice(device);
        }
    }
}

void cmidi_panicStopDevice(struct cmidi_Device* device) {
    assert(device != 0);
    for(int ch = 0; ch < 16; ch++) {
        struct cmidi_Channel* channel = &device->channel[ch];
        for(int i = 0; i < 256; i++) {
            struct cmidi_Voice* v = channel->voices[i];
            if(v) {
                cmidi_freeVoice(v);
            }
        }
    }
}

void cmidi_resetSong(struct cmidi_Song* song) {
    if(song) {
        //puts("cmidi: Warning; may result in stuck notes (still need to implement all note panic stopping).");
        cmidi_panicStopSong(song);
        song->delta = 0xFFFFFFFF;
        for(int i = 0; i < song->ntrks; i++) {
            struct cmidi_Track* track = &song->track[i];
            track->next = track->data;
            track->delta = cmidi_Track_getLen(track);
            if(track->delta < song->delta) {
                song->delta = track->delta;
                song->nextTrack = track;
            }
        }
        song->playing = song->ntrks;
        song->hasJumped = 0;
        song->onFinishedPlaying = 0;
        song->callbackSender = 0;
        song->callbackData = 0;
    }
}

void cmidi_playTrack(struct cmidi_Song* song, int track, void (*callback)(void*, void*), void* sender, void* data) {
    if(song) {
        struct cmidi_Track* soloTrack = (track >= 0 && track < song->ntrks) ? &(song->track[track]) : 0;
        cmidi_resetSong(song);
        song->soloTrack = soloTrack;
        song->onFinishedPlaying = callback;
        song->callbackSender = sender;
        song->callbackData = data;
        printf("%x\n", data);
    }
}

static void cmidi_read_helper(struct cmidi_Song* song, float* out, int len) {
    if(len > 0) {
        for(int i = 0; i < 256; i++) {
            struct cmidi_Voice* v = &voice[i];
            if(v->song == song) {
                struct cmidi_Program* program = v->program;
                if(program && program->generator) {
                    program->generator(v, out, len);
                }
            }
        }
    }
}

void cmidi_read(struct cmidi_Song* song, float* out, int len) {
    if(song && !cmidi_eot(song)) {
        unsigned sampleCount = 0;
        while(sampleCount < len && !cmidi_eot(song)) {
            if(!cmidi_eot(song)) {
                unsigned songDelta = song->nextTrack->delta;
                unsigned sampleDelta = songDelta * (unsigned)song->tick2Sample;
                if(sampleCount + sampleDelta >= len) {
                    sampleDelta = len - sampleCount;
                }
                cmidi_read_helper(song, &out[sampleCount*2], sampleDelta);
                sampleCount += sampleDelta;
                unsigned ticks = sampleDelta / (unsigned)song->tick2Sample;
                if(ticks >= songDelta) {
                    do {
                        struct cmidi_Track* track = song->nextTrack;
                        ticks -= songDelta;
                        do {
                            if(track->next < track->end) {
                                cmidi_Track_processEvent(song, track);
                                if(song->hasJumped) {
                                    song->hasJumped = 0;
                                    songDelta = 0;
                                } else if(track->next != track->end) {
                                    track->delta = cmidi_Track_getLen(track);
                                } else {
                                    track->delta = 0xFFFFFFFF;
                                }
                            }
                        } while(track->delta == 0 && track->next < track->end);
                        struct cmidi_Track* nextTrack = song->soloTrack;
                        if(!nextTrack) {
                            for(int i = 0; i < song->ntrks; i++) {
                                struct cmidi_Track* t = &song->track[i];
                                if(t->next < t->end) {
                                    if(t != track) {
                                        t->delta -= songDelta;
                                    }
                                    if(!nextTrack || t->delta < nextTrack->delta) {
                                        nextTrack = t;
                                    }
                                }
                            }
                        }
                        song->nextTrack = nextTrack;
                        songDelta = song->nextTrack ? song->nextTrack->delta : 0;
                    } while(ticks >= songDelta && song->nextTrack);
                } else {
                    for(int i = 0; i < song->ntrks; i++) {
                        struct cmidi_Track* t = &song->track[i];
                        if(t->next < t->end) {
                             t->delta -= ticks;
                        }
                    }
                }
            } else {
                cmidi_read_helper(song, &out[sampleCount*2], len - sampleCount);
            }
        }
        if(cmidi_eot(song)) {
            if(song->onFinishedPlaying) {
                song->onFinishedPlaying(song->callbackSender, song->callbackData);
                song->onFinishedPlaying = 0;
            }
        }
    }

    for(int i = 0; i < len*2; i++) {
        out[i] *= masterVolume;
    }
}
/*
            for(int i = 0; i < song->ntrks; i++) {                  //an ordered loop like this can skip events when looping.
                                                                    //if two events are close enough together (specifically if
                                                                    //both tracks's delta's are less than song->delta..) they won't
                                                                    //necesarily happen in order. e.i. track 1 might trigger a loop
                                                                    //while track 2 might still have a note off event that's supposed
                                                                    //to have happen technically before track 1's loop event.

                                                                    //the for loop needs to be replaced so that process the next track
                                                                    //delta-timewise... if they have the same time stamp than they should
                                                                    //be ordered by track number. This may require a sort of priority queue
                                                                    //so that we're not doing a O(N) search for the next smallest delta time.
                                                                    //
                struct cmidi_Track* track = &song->track[i];
                if(track->next != track->end) {
                    if(ticks >= track->delta) {
                        do {
                            cmidi_Track_processEvent(song, track);
                            if(song->hasJumped) {
                                song->hasJumped = 0;
                                i = -1;
                                ticks -= track->delta;
                                if(track->delta < song->delta) {
                                    song->delta = track->delta;
                                }
                                continue;
                            }
                            if(track->next != track->end) {
                                track->delta = cmidi_Track_getLen(track);
                            }
                        } while(track->delta == 0);
                    } else {
                        track->delta -= ticks;
                    }
                    if(track->delta < song->delta) {
                        song->delta = track->delta;
                    }
                }
            }
        }
    }

    for(int i = 0; i < len*2; i++) {
        out[i] *= masterVolume;
    }
}
*/

static void cmidi_gotoFunc(struct cmidi_Song* song, struct cmidi_Track* track, char* params) {
    if(!params) {
        return;
    }

    struct cmidi_SongStash** songStash = HashTable_At(&cmidi_markerDictionary, params);
    if(*songStash) {
        song->nextTrack = (*songStash)->nextTrack;
        for(int i = 0; i < song->ntrks; i++) {
            struct cmidi_Track* _track = &song->track[i];
            struct cmidi_TrackStash* s = &((*songStash)->stash)[i];
            _track->delta = s->delta;
            _track->last = s->last;
            _track->next = s->next;
            _track->runningStatus = s->runningStatus;
            #ifdef CMIDI_DEBUG
            printf("track[%d] {\n\t%d\n\t%d\n\t%d\n\t%d\n}\n", i, s->delta, s->last, s->next, s->runningStatus);
            #endif
        }
        song->delta = 0xFFFFFFFF;
        song->hasJumped = 1;

        cmidi_noteOffAll(song);
    }
}

static void cmidi_printFunc(struct cmidi_Song* song, struct cmidi_Track* track, char* params) {
    printf("%s\n", params);
}

static int cmidi_onRemoveFunc(struct HashPair* pair) {
    free(pair->key);
}

void cmidi_init(int flags) {
    HashTable_Init(&cmidi_funcDictionary, cmidi_cmpMarker, cmidi_hashMarker, cmidi_onRemoveFunc);
    if(flags & CMIDI_INIT_GOTO)     cmidi_register("goto", cmidi_gotoFunc);
    if(flags & CMIDI_INIT_PRINT)    cmidi_register("print", cmidi_printFunc);
}

void cmidi_quit() {
    HashTable_Destroy(&cmidi_funcDictionary);
}

int cmidi_eot(struct cmidi_Song* song) {
    assert(song != 0);
    return (!song->soloTrack && song->nextTrack == 0) || (song->soloTrack && song->soloTrack->next >= song->soloTrack->end);
}
