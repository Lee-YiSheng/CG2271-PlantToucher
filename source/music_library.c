#include "music_library.h"

// convenience macros (equal-tempered, A4 = 440 Hz). Use what you need.
#define _C4  262
#define _D4  294
#define _E4  330
#define _F4  349
#define _G4  392
#define _A4  440
#define _Bb4 466
#define _B4  494
#define _C5  523
#define _D5  587
#define _E5  659
#define _F5  698
#define _G5  784
#define _A5  880
#define REST 0

// --- “Happy” (cheery) short fanfare-style riff (public-domain friendly) ---
const Note TUNE_HAPPY[] = {
    {_C5,150},{_E5,150},{_G5,200},{REST,60},
    {_G5,120},{_A5,120},{_G5,220},{REST,80},
    {_E5,160},{_C5,220}
};
const uint8_t TUNE_HAPPY_LEN = sizeof(TUNE_HAPPY)/sizeof(TUNE_HAPPY[0]);

// --- “Sad” (downward) motif (a.k.a. the classic “sad trombone” contour) ---
const Note TUNE_SAD[] = {
    {_G4,220},{REST,40},
    {_E4,220},{REST,40},
    {_C4,320},{REST,60},
    {_D4,420}
};
const uint8_t TUNE_SAD_LEN = sizeof(TUNE_SAD)/sizeof(TUNE_SAD[0]);

static void play_sequence(const Note *seq, uint8_t n, void (*play_tone)(uint32_t, uint32_t)) {
    for (uint8_t i = 0; i < n; i++) {
        if (seq[i].freq == REST) {
            // rest: just delay the time by playing 0 Hz with duration
            play_tone(0, seq[i].dur_ms);
        } else {
            play_tone(seq[i].freq, seq[i].dur_ms);
        }
    }
}

void Music_Play(MusicId id, void (*play_tone)(uint32_t, uint32_t)) {
    if (!play_tone) return;
    switch (id) {
        case HAPPY_TUNES: play_sequence(TUNE_HAPPY, TUNE_HAPPY_LEN, play_tone); break;
        case SAD_TUNES:   play_sequence(TUNE_SAD,   TUNE_SAD_LEN,   play_tone); break;
        default: break;
    }
}
