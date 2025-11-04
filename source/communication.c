/*
 * communication.c
 *
 * Handles UART communication with the ESP32 using NXP SDK drivers
 * and a two-task (send/receive) RTOS model inspired by the lab example.
 */

#include "communication.h"    // Header with task prototypes and UARTMessage_t
#include "fsl_lpuart.h"       // MCUXpresso LPUART driver
#include "fsl_port.h"         // For pin muxing
#include "board.h"
#include "project_config.h"   // For timings, priorities
#include "rtos_manager.h"     // Access to queues/semaphores/mutex
#include "sensor_driver.h"    // For SensorData_t struct and enums
#include "fsl_debug_console.h"// For PRINTF
#include <string.h>           // For strlen, strstr, strncpy
#include <stdlib.h>           // For atof

// --- Module Variables ---
// This must match UART_RX_BUFFER_SIZE in communication.h
#define RX_BUFFER_SIZE 64

// --- Global RTOS Handles (defined in main.c) ---
extern QueueHandle_t xSensorQueue;       // The main queue for all sensor data
extern QueueHandle_t xUARTMutex;         // Mutex to protect LPUART_WriteBlocking
extern QueueHandle_t xUARTRxQueue;       // Queue for ISR to send strings to Recv_Task

// --- UART1 Interrupt Service Routine ---
// This ISR is triggered when a byte is received on LPUART1.
// It collects bytes until a newline ('\n') is found and sends the
// complete string to the xUARTRxQueue.
void LPUART1_IRQHandler(void) {
    // Static buffer and index, just like the lab example
    static char rx_buffer[RX_BUFFER_SIZE];
    static volatile uint8_t rx_index = 0;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    char received_char;

    // Check if the RX data register is full
    if ((kLPUART_RxDataRegFullFlag) & LPUART_GetStatusFlags(LPUART1)) {
        received_char = LPUART_ReadByte(LPUART1);

        // Store character if it's not newline and buffer isn't full
        if ((received_char != '\n') && (received_char != '\r') && (rx_index < RX_BUFFER_SIZE - 1)) {
            rx_buffer[rx_index++] = received_char;
        } else if (rx_index > 0) { // Check if we have received at least one char
            // End of line/message detected
            rx_buffer[rx_index] = '\0'; // Null-terminate the string

            // Create message struct and send it to the queue
            UARTMessage_t msg;
            strncpy(msg.buffer, rx_buffer, RX_BUFFER_SIZE);
            xQueueSendFromISR(xUARTRxQueue, &msg, &xHigherPriorityTaskWoken);

            rx_index = 0; // Reset buffer index for next message
        }
        // else: ignore empty newlines
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    __DSB();
}

// --- UART Initialization ---
void UART_Init(void) {
    lpuart_config_t config;

    // 1. Configure UART1 Pins (PTB17 for TX, PTB16 for RX)
    // As per the "Smart Plant Monitoring System Framework" doc [cite: 74-75]
    CLOCK_EnableClock(kCLOCK_PortB);
    PORT_SetPinMux(PORTB, 17U, kPORT_MuxAlt3); // LPUART1_TX
    PORT_SetPinMux(PORTB, 16U, kPORT_MuxAlt3); // LPUART1_RX

    // 2. Get default configuration: 115200 baud, 8N1
    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = 115200U;
    config.enableTx     = true;
    config.enableRx     = true;

    // 3. Initialize LPUART instance
    LPUART_Init(LPUART1, &config, CLOCK_GetFreq(kCLOCK_CoreSysClk));

    // 4. Enable LPUART1 receive interrupt in the peripheral
    LPUART_EnableInterrupts(LPUART1, kLPUART_RxDataRegFullInterruptEnable);

    // 5. Enable LPUART1 interrupt in the NVIC
    // Priority must be numerically higher (lower logical priority)
    // than configMAX_SYSCALL_INTERRUPT_PRIORITY
    NVIC_SetPriority(LPUART1_IRQn, 3);
    EnableIRQ(LPUART1_IRQn);

    PRINTF("LPUART1 Initialized for ESP32 Communication.\r\n");
}

// --- Send Command Helper (Thread-Safe) ---
// This function blocks, but it is thread-safe thanks to the mutex.
static BaseType_t Send_Command_To_ESP32(const char *command) {
    if (xSemaphoreTake(xUARTMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Send the command string
        LPUART_WriteBlocking(LPUART1, (const uint8_t *)command, strlen(command));
        // Send a newline character as a command terminator
        // The lab example shows the ESP32 expects a newline.
        LPUART_WriteBlocking(LPUART1, (const uint8_t *)"\n", 1);

        xSemaphoreGive(xUARTMutex);
        return pdTRUE; // Success
    } else {
        PRINTF("Error: Could not acquire UART mutex to send command.\r\n");
        return pdFALSE; // Failed to acquire mutex
    }
}

// --- JSON Parsing Helper ---
// Basic parser, assumes simple structure: {"temp":xx.x,"humidity":yy.y}
static BaseType_t Parse_DHT_Data(const char *json_string, float *temp, float *humidity) {
    const char *temp_key = "\"temp\":";
    const char *hum_key = "\"humidity\":";
    char *temp_start = strstr(json_string, temp_key);
    char *hum_start = strstr(json_string, hum_key);

    if (temp_start && hum_start) {
        temp_start += strlen(temp_key);
        hum_start += strlen(hum_key);

        *temp = atof(temp_start);
        *humidity = atof(hum_start);

        // Basic validation
        if (*temp >= -40.0 && *temp <= 80.0 && *humidity >= 0.0 && *humidity <= 100.0) {
            return pdTRUE; // Success
        } else {
             PRINTF("Warning: Parsed DHT values out of range (T:%.1f, H:%.1f)\r\n", *temp, *humidity);
             return pdFALSE;
        }
    }
    PRINTF("Error: Could not find 'temp' or 'humidity' keys in JSON: %s\r\n", json_string);
    return pdFALSE; // Keys not found
}


// --- Task: Periodically Send Request to ESP32 ---
void ESP32_Send_Task(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    PRINTF("ESP32 Send Task Started.\r\n");

    for (;;) {
        // Use vTaskDelayUntil for precise periodic execution
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(DHT11_POLL_INTERVAL_MS));

        if (Send_Command_To_ESP32("GET_DHT") == pdFALSE) {
             PRINTF("Send_Task: Failed to send GET_DHT (Mutex busy?).\r\n");
        }
    }
}

// --- Task: Receive and Parse Data from ESP32 ---
void ESP32_Receive_Task(void *pvParameters) {
    UARTMessage_t received_msg;
    SensorData_t dht_data;

    // This task is only responsible for DHT11 data.
    dht_data.source = SENSOR_DHT11;
    dht_data.light_intensity = 0; // Not used
    dht_data.water_level = 0;     // Not used

    PRINTF("ESP32 Receive Task Started.\r\n");

    for (;;) {
        // Block indefinitely waiting for a message from the ISR queue
        if (xQueueReceive(xUARTRxQueue, &received_msg, portMAX_DELAY) == pdPASS) {

            // PRINTF("ESP32_Recv_Task: Got string: %s\r\n", received_msg.buffer);

            // 1. Parse the received data
            if (Parse_DHT_Data(received_msg.buffer, &dht_data.temperature, &dht_data.humidity) == pdTRUE) {
                // 2. Send the valid data to the main Sensor Queue
                if (xQueueSend(xSensorQueue, &dht_data, pdMS_TO_TICKS(100)) != pdPASS) {
                    PRINTF("ESP32_Recv_Task: Failed to send DHT data to Sensor Queue.\r\n");
                } else {
                     PRINTF("DHT Data Sent: T=%.1f H=%.1f\r\n", dht_data.temperature, dht_data.humidity);
                }
            } else {
                // Parsing failed
                PRINTF("ESP32_Recv_Task: Failed to parse ESP32 response: %s\r\n", received_msg.buffer);
            }
        }
    }
}
