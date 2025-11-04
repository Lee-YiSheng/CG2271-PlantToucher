#include "board.h"
#include "pin_mux.h"
#include "fsl_common.h"
#include "fsl_clock.h"
#include "fsl_device_registers.h"

#include "music_library.h"
#include "actuator_driver.h"

#include "board.h"
#include "pin_mux.h"
#include "fsl_common.h"
#include "fsl_clock.h"
#include "fsl_device_registers.h"

#include "actuator_driver.h"
#include "music_library.h"

#define LED_PIN_PTC   1u    // external LED on PTC1
#define BUZ_PIN_PTC   2u    // buzzer S on PTC2

static inline void delay_us(uint32_t us) {
    SDK_DelayAtLeastUs(us, SystemCoreClock);
}

/* -------------------- BUZZER (GPIO) -------------------- */
static void buzzer_init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;
    PORTC->PCR[BUZ_PIN_PTC] =
        (PORTC->PCR[BUZ_PIN_PTC] & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(1); // GPIO
    GPIOC->PDDR |= (1u << BUZ_PIN_PTC);
    GPIOC->PCOR   = (1u << BUZ_PIN_PTC);
}

static void buzzer_play_gpio(uint32_t freq_hz, uint32_t ms) {
    if (!ms) return;

    if (!freq_hz) {          // rest
        delay_us(ms * 1000u);
        GPIOC->PCOR = (1u << BUZ_PIN_PTC);
        return;
    }

    uint32_t half_us = 500000u / freq_hz;
    if (!half_us) half_us = 1;
    uint32_t toggles = (ms * 1000u) / half_us;
    if (toggles & 1u) toggles--;

    for (uint32_t i = 0; i < toggles; ++i) {
        GPIOC->PTOR = (1u << BUZ_PIN_PTC);
        delay_us(half_us);
    }
    GPIOC->PCOR = (1u << BUZ_PIN_PTC);
}

/* -------------------- LED (TPM0 on PTC1) -------------------- */
static void led_init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;
    PORTC->PCR[1] = (PORTC->PCR[1] & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(4); // ALT4 = TPM0_CH0
    SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK;
    SIM->SOPT2 = (SIM->SOPT2 & ~SIM_SOPT2_TPMSRC_MASK) | SIM_SOPT2_TPMSRC(1);

    TPM0->SC  = 0;
    TPM0->CNT = 0;
    TPM0->SC |= TPM_SC_PS(7);        // prescaler 128
    TPM0->SC |= TPM_SC_CPWMS_MASK;   // center-aligned
    TPM0->MOD = 125;                 // ~250 Hz

    // high-true PWM  ➜  use ELSA if your LED is active-low
    TPM0->CONTROLS[0].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSB_MASK;
    TPM0->CONTROLS[0].CnV  = 0;

    TPM0->SC = (TPM0->SC & ~TPM_SC_CMOD_MASK) | TPM_SC_CMOD(1); // start
}

void Set_LED_Intensity(uint8_t intensity) {
    // map 0..255 -> 0..MOD
    uint32_t mod = TPM0->MOD;
    uint32_t cv  = (uint32_t)((intensity / 255.0f) * (float)mod);
    TPM0->CONTROLS[0].CnV = cv;
}

/* -------------------- PUBLIC API -------------------- */
void Actuators_Init(void) {
    led_init();
    buzzer_init();
    Set_LED_Intensity(0);    // start off
}


void Play_Music(MusicType_t music_type) {
    switch (music_type) {
    case MUSIC_OFF:
    	GPIOC->PCOR = (1u << BUZ_PIN_PTC);
        break;
    case MUSIC_HAPPY:
        Music_Play(HAPPY_TUNES, buzzer_play_gpio);
        break;
    case MUSIC_SAD:
        Music_Play(SAD_TUNES, buzzer_play_gpio);
        break;
    case MUSIC_ALERT:
        // simple built-in alert
        for (int i = 0; i < 3; i++) {
            buzzer_play_gpio(2000, 150);
            delay_us(80000u);
        }
        break;
    default:
        break;
    }
}
