#pragma once
#include <stdint.h>

typedef enum {
    MUSIC_OFF = 0,
    MUSIC_HAPPY,
    MUSIC_SAD,
    MUSIC_ALERT
} MusicType_t;


void Actuators_Init(void);
void Set_LED_Intensity(uint8_t intensity_0_255);
void Play_Music(MusicType_t music_type);
