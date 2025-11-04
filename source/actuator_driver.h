#pragma once
#include <stdint.h>

typedef enum {
    MUSIC_OFF = 0,
    MUSIC_HAPPY,
    MUSIC_SAD,
    MUSIC_ALERT
} MusicType_t;

typedef struct {
    MusicType_t buzzer_command;
    uint8_t led_intensity;
    // uint8_t alert_level; // This was in your doc, add it if you need it
} ActuatorCommand_t;


void Actuators_Init(void);
void Set_LED_Intensity(uint8_t intensity_0_255);
void Play_Music(MusicType_t music_type);
