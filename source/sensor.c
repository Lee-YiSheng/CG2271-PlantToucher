#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_port.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#define WATER_LEVEL_PIN       0  // PTC0
#define PHOTORESISTOR_PIN     22 // PTE22

typedef struct {
    uint32_t water_level;      // 0-100%
    uint32_t light_intensity;  // 0-1023 (ADC value)
    float temperature;         // Celsius
    float humidity;            // Percentage
} SensorData_t;

static SemaphoreHandle_t xWaterLevelSemaphore;
SensorData_t sensorData;

//Init both sensors
void initSensors(void) {
	NVIC_DisableIRQ(ADC0_IRQn);
	NVIC_ClearPendingIRQ(ADC0_IRQn);

    // Enable ADC clock gating (SIM_SCGC6)
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;

    // Enable clock for PORTE & PORTC
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;

    // Configure photoresistor ADC pin (PTE22 - ADC0_SE3)
    PORTE->PCR[PHOTORESISTOR_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[PHOTORESISTOR_PIN] |= PORT_PCR_MUX(0);

    // Configure water level sensor ADC pin (PTC0 - ADC0_SE14)
	PORTC->PCR[WATER_LEVEL_PIN] &= ~PORT_PCR_MUX_MASK;
	PORTC->PCR[WATER_LEVEL_PIN] |= PORT_PCR_MUX(0);

	// Configure the ADC, Enable ADC interrupt
	ADC0->SC1[0] |= ADC_SC1_AIEN_MASK | ADC_SC1_ADCH(14);// Start interrupt-based conversion on channel 14 (PTC0)
	ADC0->SC1[0] &= ~ADC_SC1_DIFF_MASK;
	ADC0->SC1[0] |= ADC_SC1_DIFF(0b0);

    // Configure ADC0_CFG1 for 12-bit resolution
    ADC0->CFG1 &= ~ADC_CFG1_MODE_MASK;
    ADC0->CFG1 |= ADC_CFG1_MODE(0b01);

    // Use software trigger
    ADC0->SC2 &= ~ADC_SC2_ADTRG_MASK;

    // Use VALTH and VALTL
    ADC0->SC2 &= ~ADC_SC2_REFSEL_MASK;
    ADC0->SC2 |= ADC_SC2_REFSEL(0b01);

    // Don't use averaging
    ADC0->SC3 &= ~ADC_SC3_AVGE_MASK;
    ADC0->SC3 |= ADC_SC3_AVGE(0);

    // Use continuous conversion
    ADC0->SC3 &= ~ADC_SC3_ADCO_MASK;
    ADC0->SC3 |= ADC_SC3_ADCO(0);

    NVIC_SetPriority(ADC0_IRQn, 192);
    NVIC_EnableIRQ(ADC0_IRQn);
}

// Read by polling
uint32_t ReadPhotoresistor() {
    ADC0->SC1[1] &= ~ADC_SC1_ADCH_MASK;
    ADC0->SC1[1] |= ADC_SC1_ADCH(3); //starts conversion
    while (!(ADC0->SC1[1] & ADC_SC1_COCO_MASK)) {}
    return ADC0->R[1];
}

void ADC0_IRQHandler(void) {
	NVIC_ClearPendingIRQ(ADC0_IRQn);
    BaseType_t hpw = pdFALSE;

    if (ADC0->SC1[0] & ADC_SC1_COCO_MASK) {
        sensorData.water_level = ADC0->R[0];
        xSemaphoreGiveFromISR(xWaterLevelSemaphore, &hpw);
        portYIELD_FROM_ISR(hpw);

        // Restart the interrupt-driven conversion
        ADC0->SC1[0] = ADC_SC1_AIEN_MASK | ADC_SC1_ADCH(14);
    }
}

void Sensor_Task(void *pvParameters) {
    while (1) {
        sensorData.light_intensity = ReadPhotoresistor();
        PRINTF("water level: %d, light_intensity: %d\n", &sensorData.water_level, &sensorData.light_intensity);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

int main(void) {
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    xWaterLevelSemaphore = xSemaphoreCreateBinary();
    sensorData.water_level = 0;
    sensorData.light_intensity = 0;
    sensorData.temperature = 0;
    sensorData. humidity = 0;


    initSensors();
    xTaskCreate(Sensor_Task, "SensorTask", configMINIMAL_STACK_SIZE + 256, NULL, 0, NULL);
    vTaskStartScheduler();

    while (1);

    return 0;
}
