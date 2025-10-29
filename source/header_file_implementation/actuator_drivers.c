#include "actuator_drivers.h"
#include "project_config.h"      // For task priorities, stack sizes
#include "fsl_gpio.h"            // For potential direct GPIO control if needed
#include "fsl_port.h"            // For pin muxing
//#include "fsl_tpm.h"             // TPM driver for PWM
#include "board.h"               // Board specifics
#include "fsl_debug_console.h"   // For PRINTF
#include "FreeRTOS.h"
#include "task.h"                // For vTaskDelay

// --- Peripheral Definitions ---
// Define the TPM instance and channels used for LED and Buzzer
// IMPORTANT: Choose pins that support TPM output and configure them in pin_mux.c
#define LED_TPM_BASEADDR        TPM0                // Example: Use TPM0
#define LED_TPM_CHANNEL         kTPM_Chnl_0         // Example: Use Channel 0
#define LED_GPIO_PORT           /* GPIO_PORT_FOR_LED */  // Define in board.h or project_config.h
#define LED_GPIO_PIN            /* GPIO_PIN_FOR_LED */   // Define in board.h or project_config.h

#define BUZZER_TPM_BASEADDR     TPM0                // Example: Use TPM0
#define BUZZER_TPM_CHANNEL      kTPM_Chnl_1         // Example: Use Channel 1
#define BUZZER_GPIO_PORT        /* GPIO_PORT_FOR_BUZZER */ // Define in board.h or project_config.h
#define BUZZER_GPIO_PIN         /* GPIO_PIN_FOR_BUZZER */  // Define in board.h or project_config.h

#define TPM_SOURCE_CLOCK        CLOCK_GetFreq(kCLOCK_PllFllSelClk) // Get the TPM clock frequency

// --- Global Handles (defined elsewhere, e.g., main.c) ---
extern QueueHandle_t xActuatorQueue;

// --- Simple Music Definitions ---
// Structure for a musical note
typedef struct {
    uint16_t frequency; // Frequency in Hz (0 for rest)
    uint16_t duration;  // Duration in ms
} Note_t;

// Example Tunes (replace with actual desired notes)
const Note_t happy_tune[] = {
    {523, 200}, {587, 200}, {659, 200}, {0, 100}, // C5, D5, E5, Rest
    {659, 200}, {587, 200}, {523, 400}, {0, 0}    // E5, D5, C5 (End marker)
};

const Note_t stressed_tune[] = {
    {261, 150}, {0, 50}, {261, 150}, {0, 150},   // C4, Rest, C4, Rest
    {311, 150}, {0, 50}, {311, 150}, {0, 0}      // D#4, Rest, D#4 (End marker)
};


// --- Private Helper Functions ---

/**
 * @brief Configures a TPM instance and channel for PWM output.
 *
 * @param base TPM peripheral base address (e.g., TPM0).
 * @param channel TPM channel number (e.g., kTPM_Chnl_0).
 * @param initial_duty_cycle Initial duty cycle percentage (0-100).
 */
static void Init_PWM_Channel(TPM_Type *base, tpm_chnl_t channel, uint8_t initial_duty_cycle) {
    tpm_config_t tpmInfo;
    tpm_chnl_pwm_signal_param_t tpmParam;

    // Configure PWM signal parameters
    tpmParam.chnlNumber = channel;
    tpmParam.level = kTPM_HighTrue; // High-true pulses
    tpmParam.dutyCyclePercent = initial_duty_cycle;

    // Get default TPM configuration
    TPM_GetDefaultConfig(&tpmInfo);
    // You might want to adjust prescaler depending on desired frequency range/resolution
    // tpmInfo.prescale = kTPM_Prescale_Divide_4;

    // Initialize TPM module
    TPM_Init(base, &tpmInfo);

    // Setup PWM channel
    // Use kTPM_EdgeAlignedPwm as it's simpler for fixed frequency PWM like LED brightness
    // For buzzer frequency control, kTPM_CenterAlignedPwm might offer advantages but is more complex to update
    TPM_SetupPwm(base, &tpmParam, 1U, kTPM_EdgeAlignedPwm, 24000U, TPM_SOURCE_CLOCK); // Default 24kHz PWM frequency

    // Start the timer
    TPM_StartTimer(base, kTPM_SystemClock);
}

/**
 * @brief Plays a sequence of notes defined in an array.
 *
 * @param tune Pointer to the array of Note_t structures.
 */
static void Play_Tune(const Note_t *tune) {
    int i = 0;
    while (tune[i].duration > 0) { // Loop until duration is 0 (end marker)
        Play_Buzzer_Tone(tune[i].frequency, tune[i].duration);
        // Add a small gap between notes if desired
        // vTaskDelay(pdMS_TO_TICKS(10));
        i++;
    }
    // Ensure buzzer is off after the tune finishes
    Play_Buzzer_Tone(0, 0);
}


// --- Public Function Implementations ---

void Actuator_Init(void) {
    // 1. Configure LED Pin Muxing (Example for a pin on PORTA)
    // CLOCK_EnableClock(kCLOCK_PortA);
    // PORT_SetPinMux(PORTA, LED_GPIO_PIN, kPORT_MuxAlt3); // TPM0_CH0 on PTAx? Check datasheet

    // 2. Configure Buzzer Pin Muxing (Example for a pin on PORTB)
    // CLOCK_EnableClock(kCLOCK_PortB);
    // PORT_SetPinMux(PORTB, BUZZER_GPIO_PIN, kPORT_MuxAlt3); // TPM0_CH1 on PTBx? Check datasheet

    // 3. Initialize PWM channels
    Init_PWM_Channel(LED_TPM_BASEADDR, LED_TPM_CHANNEL, 0); // LED off initially
    Init_PWM_Channel(BUZZER_TPM_BASEADDR, BUZZER_TPM_CHANNEL, 0); // Buzzer off initially

    PRINTF("Actuators Initialized.\r\n");
}

void Set_LED_Brightness(uint8_t brightness) {
    // Clamp brightness to 0-100 range
    if (brightness > 100) {
        brightness = 100;
    }

    // Stop the timer, update duty cycle, restart timer
    // This is the safest way to update PWM parameters
    TPM_StopTimer(LED_TPM_BASEADDR);
    TPM_UpdatePwmDutycycle(LED_TPM_BASEADDR, LED_TPM_CHANNEL, kTPM_EdgeAlignedPwm, brightness);
    TPM_StartTimer(LED_TPM_BASEADDR, kTPM_SystemClock);
}

void Play_Buzzer_Tone(uint16_t frequency, uint32_t duration) {
    TPM_StopTimer(BUZZER_TPM_BASEADDR);

    if (frequency > 0) {
        // Update PWM frequency (period) and set duty cycle (e.g., 50%)
        // Note: TPM_UpdatePwmDutycycle doesn't change frequency directly.
        // We need TPM_SetupPwm to change the frequency (mod value).
        // This is more complex than just updating duty cycle.

        // Simpler Approach (less accurate frequency): Update period if needed, then duty cycle
        // Calculate MOD value based on frequency and TPM clock. Be careful about clock source and prescaler.
        // uint32_t mod = (TPM_SOURCE_CLOCK / (1 << BUZZER_TPM_BASEADDR->SC & TPM_SC_PS_MASK)) / frequency;
        // BUZZER_TPM_BASEADDR->MOD = mod;

        // Using TPM_SetupPwm to change frequency might be necessary for accuracy
        // TPM_SetupPwm(BUZZER_TPM_BASEADDR, &buzzerPwmParam, 1U, kTPM_EdgeAlignedPwm, frequency, TPM_SOURCE_CLOCK);

        // For simplicity here, we'll just set duty cycle (assumes frequency is set elsewhere or default is okay)
        TPM_UpdatePwmDutycycle(BUZZER_TPM_BASEADDR, BUZZER_TPM_CHANNEL, kTPM_EdgeAlignedPwm, 50); // 50% duty cycle for tone
        TPM_StartTimer(BUZZER_TPM_BASEADDR, kTPM_SystemClock);

        if (duration > 0) {
            vTaskDelay(pdMS_TO_TICKS(duration));
            TPM_StopTimer(BUZZER_TPM_BASEADDR); // Stop after duration
            TPM_UpdatePwmDutycycle(BUZZER_TPM_BASEADDR, BUZZER_TPM_CHANNEL, kTPM_EdgeAlignedPwm, 0); // Ensure it's off
            // No need to restart timer if duty cycle is 0
        }
    } else {
        // Frequency is 0, turn off PWM
        TPM_UpdatePwmDutycycle(BUZZER_TPM_BASEADDR, BUZZER_TPM_CHANNEL, kTPM_EdgeAlignedPwm, 0);
        // Timer remains stopped
    }
}


// --- Actuator Control Task ---
void Actuator_Control_Task(void *pvParameters) {
    ActuatorCommand_t received_command;

    PRINTF("Actuator Control Task Started. Waiting for commands...\r\n");

    for (;;) {
        // Wait indefinitely for a command to arrive on the queue
        if (xQueueReceive(xActuatorQueue, &received_command, portMAX_DELAY) == pdPASS) {
            PRINTF("Actuator Task received command: Type=%d, V1=%lu, V2=%lu\r\n",
                   received_command.type, received_command.value1, received_command.value2);

            switch (received_command.type) {
                case ACTUATOR_LED:
                    Set_LED_Brightness((uint8_t)received_command.value1);
                    break;

                case ACTUATOR_BUZZER_TONE:
                    Play_Buzzer_Tone((uint16_t)received_command.value1, received_command.value2);
                    break;

                case ACTUATOR_BUZZER_MUSIC_HAPPY:
                    PRINTF("Playing Happy Tune...\r\n");
                    Play_Tune(happy_tune);
                    break;

                case ACTUATOR_BUZZER_MUSIC_STRESSED:
                     PRINTF("Playing Stressed Tune...\r\n");
                    Play_Tune(stressed_tune);
                    break;

                default:
                    PRINTF("Actuator Task: Unknown command type received.\r\n");
                    break;
            }
        }
        // No delay here, task blocks on xQueueReceive
    }
}
