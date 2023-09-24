#ifndef CMIDI_H_
    #define CMIDI_H_

//#define CMIDI_DEBUG
#define cmidi_setProgram(A,B) cmidi_setPatchProgram(0,A,B)
#define cmidi_register(KEY,VALUE) (*((cmidi_Func*)HashTable_At(&cmidi_funcDictionary, KEY)) = VALUE)
#define CMIDI_INIT_NONE     0x00
#define CMIDI_INIT_GOTO     0x01
#define CMIDI_INIT_PRINT    0x02
#define CMIDI_INIT_ALL      0xFF
struct cmidi_Voice;
struct cmidi_Program;
struct cmidi_ADSR;
struct cmidi_Song;
struct cmidi_Track;

typedef void (*cmidi_Program_Generator)(struct cmidi_Voice* v, float* out, int len);
typedef void (*cmidi_Program_Controller)(struct cmidi_Program* p, unsigned char controller, unsigned char value);
typedef void (*cmidi_Program_Filter)(struct cmidi_Voice* v, float* out, int len, float (*generator)(struct cmidi_Voice*, float), struct cmidi_ADSR* adsr);
typedef void (*cmidi_Func)(struct cmidi_Song* song, struct cmidi_Track* track, char* params);

struct cmidi_ADSR {
    unsigned a;
    unsigned d;
    float s;
    unsigned r;
};

struct cmidi_Program {
    cmidi_Program_Generator generator;
    cmidi_Program_Controller controller;
    cmidi_Program_Filter filter;
    float lPan;
    float rPan;
    float vol;
    float freq; //frequency multiplier; default 1.
    void* user;
};

struct cmidi_Channel {
    unsigned char program;
    float pitch;
    struct cmidi_Voice* voices[256];
};

struct cmidi_Patch {
    struct cmidi_Program* program[256];
};

struct cmidi_Device {
    char name[256];
    int refCount;   //if positive non-zero than this device is alocated on the heap. Otherwise it's on the stack.
    struct cmidi_Patch* patch;
    struct cmidi_Channel channel[16];
};

struct cmidi_Voice {
    struct cmidi_Song* song;
    struct cmidi_Channel* channel;
    struct cmidi_Program* program;
    int count;
    int timer;
    float offset;
    int rel;
    unsigned char note;
    float vel;
};

struct cmidi_Track {
    char name[256];
    struct cmidi_Device* device;
    unsigned long delta;
    unsigned long last;
    unsigned char runningStatus;
    unsigned char* data;
    unsigned char* next;
    unsigned char* end;
    unsigned char* currEvent; //used mainly when next doesn't point to midi event
};

struct cmidi_TrackStash {
    unsigned long delta;
    unsigned long last;
    unsigned char runningStatus;
    unsigned char* next;
};

struct cmidi_SongStash {
    struct cmidi_Track* nextTrack;
    struct cmidi_TrackStash stash[];
};

struct cmidi_Song {
    const char * filename;
    unsigned char * data;
    unsigned format;
    unsigned ntrks;
    unsigned rate;
    unsigned tempo;
    unsigned division;
    unsigned clock;
    float tick2Sample;
    struct cmidi_Track* track; //array of tracks allocated continuously
    struct cmidi_Track* nextTrack; //next track to play
    struct cmidi_Track* soloTrack; //if set then only this track will be processed.
    unsigned delta; //how many samples we're to advance by next time we're sampling the voices.
    unsigned position;  //unused; mean't to be song position in midi ticks
    unsigned playing; //number of tracks currently playing.
    unsigned hasJumped; //goto was called so we need to restore some state
    void (*onFinishedPlaying)(void*, void*);
    void* callbackSender;
    void* callbackData;
};

int cmidi_eot(struct cmidi_Song* song); //End of Track (have all tracks completed playing)
int cmidi_loadMem(struct cmidi_Song* song, void* mem);
void cmidi_freeMem(struct cmidi_Song* song);
struct cmidi_Song* cmidi_load(const char* filename);
void cmidi_free(struct cmidi_Song* song);

void cmidi_setPort(unsigned char port, struct cmidi_Device* device);
void cmidi_setPatchProgram(struct cmidi_Patch* patch, int programId, struct cmidi_Program* program);
struct cmidi_Voice* cmidi_getNextVoice();
void cmidi_freeVoice(struct cmidi_Voice* v);
void cmidi_setVolume(float volume);
void cmidi_setDevice(struct cmidi_Song* song, struct cmidi_Device* device);

struct cmidi_Device* cmidi_openDevice(const char* name, struct cmidi_Patch* patch);
void cmidi_closeDevice(struct cmidi_Device* device);

void cmidi_noteOffAll(struct cmidi_Song* song);
void cmidi_panicStopAll();
void cmidi_panicStopSong(struct cmidi_Song* song);
void cmidi_panicStopDevice(struct cmidi_Device* device);
void cmidi_resetSong(struct cmidi_Song* song);

void cmidi_playTrack(struct cmidi_Song* song, int track, void (*callback)(void* sender, void* data), void* sender, void* data);

void cmidi_init();
void cmidi_quit();

int cmidi_audioFreq();

void cmidi_read(struct cmidi_Song* song, float* out, int len); //float is len*2 since is number of samples and out had 2 channels.. so twice the amount of samples.

extern struct cmidi_Patch cmidi_patch_default;
extern struct cmidi_Device cmidi_device_default;

#endif // CMIDI_H_
