/*
 * communication.c
 *
 * Handles periodic communication with the ESP32 to fetch DHT11 data.
 */

#include "communication.h"
#include "fsl_lpuart.h"     // MCUXpresso LPUART driver
#include "fsl_port.h"       // For pin muxing
#include "board.h"          // Board specific definitions
#include "project_config.h" // For timings, priorities
#include "rtos_manager.h"   // Access to queues/semaphores/mutex
#include "sensor_driver.h"  // <-- FIX: Added to define SensorData_t and enums
#include "fsl_debug_console.h" // For PRINTF
#include <string.h>         // For strlen, strstr
#include <stdlib.h>         // For atof
#include "sensor_driver.h"

// --- Module Variables ---
#define RX_BUFFER_SIZE 64           // Size of the buffer to hold incoming UART data
static char rx_buffer[RX_BUFFER_SIZE]; // Static buffer for ISR
static volatile uint8_t rx_index = 0;   // Index for the rx_buffer
static SemaphoreHandle_t xUARTRxSemaphore = NULL; // Signals task when a full line is received

// Global handles (defined elsewhere, e.g., main.c)
extern QueueHandle_t xSensorQueue;
extern SemaphoreHandle_t xUARTMutex;

// --- UART1 Interrupt Service Routine ---
void LPUART1_IRQHandler(void) { // <-- FIX: Renamed from UART1_IRQHandler
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    char received_char;

    // Check if the RX data register is full
    if ((kLPUART_RxDataRegFullFlag) & LPUART_GetStatusFlags(LPUART1)) { // <-- FIX: LPUART1
        received_char = LPUART_ReadByte(LPUART1); // <-- FIX: LPUART1

        // Store character if it's not newline and buffer isn't full
        if ((received_char != '\n') && (received_char != '\r') && (rx_index < RX_BUFFER_SIZE - 1)) {
            rx_buffer[rx_index++] = received_char;
        } else {
            // End of line/message detected or buffer full
            rx_buffer[rx_index] = '\0'; // Null-terminate the string
            rx_index = 0;               // Reset buffer index for next message

            if (xUARTRxSemaphore != NULL) {
                 xSemaphoreGiveFromISR(xUARTRxSemaphore, &xHigherPriorityTaskWoken);
            }
        }
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    __DSB();
}

// --- UART Initialization ---
void UART_Init(void) {
    lpuart_config_t config;

    // 1. Configure UART1 Pins
    // The framework doc says PTB17 (TX) and PTB16 (RX)
    CLOCK_EnableClock(kCLOCK_PortB);
    PORT_SetPinMux(PORTB, 17U, kPORT_MuxAlt3); // UART1_TX
    PORT_SetPinMux(PORTB, 16U, kPORT_MuxAlt3); // UART1_RX

    // 2. Get default configuration: 115200 baud, 8N1
    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = 115200U;
    config.enableTx     = true;
    config.enableRx     = true;

    // 3. Initialize LPUART instance
    LPUART_Init(LPUART1, &config, CLOCK_GetFreq(kCLOCK_CoreSysClk)); // <-- FIX: LPUART1

    // 4. Create the semaphore used by the ISR
    xUARTRxSemaphore = xSemaphoreCreateBinary();
    if (xUARTRxSemaphore == NULL) {
        PRINTF("Error creating UART Rx Semaphore!\r\n");
        while(1);
    }
     vQueueAddToRegistry(xUARTRxSemaphore, "UARTRxSema");

    // 5. Enable UART1 receive interrupt in the peripheral
    LPUART_EnableInterrupts(LPUART1, kLPUART_RxDataRegFullInterruptEnable); // <-- FIX: LPUART1

    // 6. Enable UART1 interrupt in the NVIC
    NVIC_SetPriority(LPUART1_IRQn, 3); // <-- FIX: LPUART1_IRQn
    EnableIRQ(LPUART1_IRQn);           // <-- FIX: LPUART1_IRQn

    PRINTF("UART1 (LPUART1) Initialized for ESP32 Communication.\r\n");
}

// --- Send Command Helper ---
BaseType_t Send_Command_To_ESP32(const char *command) {
    if (xSemaphoreTake(xUARTMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        LPUART_WriteBlocking(LPUART1, (const uint8_t *)command, strlen(command)); // <-- FIX: LPUART1
        LPUART_WriteBlocking(LPUART1, (const uint8_t *)"\n", 1);                  // <-- FIX: LPUART1
        xSemaphoreGive(xUARTMutex);
        return pdTRUE;
    } else {
        PRINTF("Error: Could not acquire UART mutex to send command.\r\n");
        return pdFALSE;
    }
}

// --- JSON Parsing Helper ---
BaseType_t Parse_DHT_Data(const char *json_string, float *temp, float *humidity) {
    const char *temp_key = "\"temp\":";
    const char *hum_key = "\"humidity\":";
    char *temp_start = strstr(json_string, temp_key);
    char *hum_start = strstr(json_string, hum_key);

    if (temp_start && hum_start) {
        temp_start += strlen(temp_key);
        hum_start += strlen(hum_key);

        *temp = atof(temp_start);
        *humidity = atof(hum_start);

        if (*temp >= -40.0 && *temp <= 80.0 && *humidity >= 0.0 && *humidity <= 100.0) {
            return pdTRUE;
        } else {
             PRINTF("Warning: Parsed DHT values out of range (T:%.1f, H:%.1f)\r\n", *temp, *humidity);
             return pdFALSE;
        }
    }
    PRINTF("Error: Could not find 'temp' or 'humidity' keys in JSON: %s\r\n", json_string);
    return pdFALSE;
}


// --- ESP32 Communication Task ---
void ESP32_Communication_Task(void *pvParameters) {
    // Create a local instance of the NEW struct
    SensorData_t dht_data;

    // Set the source *once*
    dht_data.source = SENSOR_DHT11;

    // Initialize other fields to a known "invalid" state
    dht_data.water_level = 0;
    dht_data.light_intensity = 0;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    PRINTF("ESP32 Communication Task Started.\r\n");

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(DHT11_POLL_INTERVAL_MS));

        if (Send_Command_To_ESP32("GET_DHT") == pdTRUE) {
            if (xSemaphoreTake(xUARTRxSemaphore, pdMS_TO_TICKS(UART_RX_TIMEOUT_MS)) == pdTRUE) {
                if (xSemaphoreTake(xUARTMutex, pdMS_TO_TICKS(50)) == pdTRUE) {

                    char local_rx_buffer[RX_BUFFER_SIZE];
                    strncpy(local_rx_buffer, rx_buffer, RX_BUFFER_SIZE - 1);
                    local_rx_buffer[RX_BUFFER_SIZE - 1] = '\0';
                    xSemaphoreGive(xUARTMutex);

                    // --- FIX: Use .temperature and .humidity, NOT .value1/.value2 ---
                    if (Parse_DHT_Data(local_rx_buffer, &dht_data.temperature, &dht_data.humidity) == pdTRUE) {

                        // Send the struct (now with source=DHT11) to the logic task
                        if (xQueueSend(xSensorQueue, &dht_data, pdMS_TO_TICKS(100)) != pdPASS) {
                            PRINTF("Error: Failed to send DHT data to Sensor Queue.\r\n");
                        } else {
                             PRINTF("DHT Data Sent: T=%.1f H=%.1f\r\n", dht_data.temperature, dht_data.humidity);
                        }
                    } else {
                        PRINTF("Error: Failed to parse ESP32 response: %s\r\n", local_rx_buffer);
                    }
                } else {
                    PRINTF("Error: Could not acquire UART mutex to read buffer.\r\n");
                }
            } else {
                PRINTF("Timeout: No response from ESP32 for GET_DHT command.\r\n");

                dht_data.temperature = -99.9; // Indicate error
                dht_data.humidity = -99.9;
                xQueueSend(xSensorQueue, &dht_data, 0); // Send error status
            }
        } else {
             PRINTF("Failed to send GET_DHT command (Mutex busy?).\r\n");
        }
    }
}
