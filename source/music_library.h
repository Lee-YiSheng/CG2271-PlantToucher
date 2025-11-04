#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t freq;    // Hz (0 = rest)
    uint16_t dur_ms;  // duration in milliseconds
} Note;


// Ids for your tunes
typedef enum {
    HAPPY_TUNES = 0,
    SAD_TUNES   = 1,
} MusicId;

// Play a full tune by calling your tone function for each note.
// You must provide a function with signature: void play_tone(uint32_t hz, uint32_t ms);
void Music_Play(MusicId id, void (*play_tone)(uint32_t, uint32_t));

// --- Optional: access raw sequences & lengths
extern const Note TUNE_HAPPY[];
extern const uint8_t TUNE_HAPPY_LEN;
extern const Note TUNE_SAD[];
extern const uint8_t TUNE_SAD_LEN;

#ifdef __cplusplus
}
#endif
