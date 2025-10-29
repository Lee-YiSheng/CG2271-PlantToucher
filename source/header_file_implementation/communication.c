#include "communication.h"
#include "fsl_lpuart.h"     // MCUXpresso LPUART driver
#include "fsl_port.h"       // For pin muxing
#include "board.h"          // Board specific definitions
#include "project_config.h" // For timings, priorities
#include "rtos_manager.h"   // Access to queues/semaphores/mutex
#include "sensor_driver.h"  // For SensorData_t struct
#include "fsl_debug_console.h" // For PRINTF
#include <string.h>         // For strlen, strstr
#include <stdlib.h>         // For atof

// --- Module Variables ---
#define RX_BUFFER_SIZE 64           // Size of the buffer to hold incoming UART data
static char rx_buffer[RX_BUFFER_SIZE]; // Static buffer for ISR
static volatile uint8_t rx_index = 0;   // Index for the rx_buffer
static SemaphoreHandle_t xUARTRxSemaphore = NULL; // Signals task when a full line is received

// Global handles (defined elsewhere, e.g., main.c)
extern QueueHandle_t xSensorQueue;
extern SemaphoreHandle_t xUARTMutex;

// --- UART1 Interrupt Service Routine ---
// This ISR is triggered when a byte is received on UART1.
// It collects bytes into rx_buffer until a newline ('\n') is found.
void UART1_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    char received_char;

    // Check if the RX data register is full
    if ((kLPUART_RxDataRegFullFlag) & LPUART_GetStatusFlags(UART1)) {
        received_char = LPUART_ReadByte(UART1); // Read the received byte

        // Store character if it's not newline and buffer isn't full
        if ((received_char != '\n') && (received_char != '\r') && (rx_index < RX_BUFFER_SIZE - 1)) {
            rx_buffer[rx_index++] = received_char;
        } else {
            // End of line/message detected or buffer full
            rx_buffer[rx_index] = '\0'; // Null-terminate the string
            rx_index = 0;               // Reset buffer index for next message

            // Signal the ESP32_Communication_Task that a complete message is ready
            // Only give semaphore if it has been created
            if (xUARTRxSemaphore != NULL) {
                 xSemaphoreGiveFromISR(xUARTRxSemaphore, &xHigherPriorityTaskWoken);
            }
        }
    }

    // If giving the semaphore woke a higher priority task, yield immediately from ISR
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

    // Add memory barrier for ARM Cortex-M0+ if necessary (usually handled by FreeRTOS port)
    __DSB();
}

// --- UART Initialization ---
void UART_Init(void) {
    lpuart_config_t config;

    // 1. Configure UART1 Pins (using MCUXpresso Config Tools is recommended)
    // Example: PORTC pin 4 for TX, PORTC pin 3 for RX
    CLOCK_EnableClock(kCLOCK_PortC);
    PORT_SetPinMux(PORTC, 4U, kPORT_MuxAlt3); // UART1_TX
    PORT_SetPinMux(PORTC, 3U, kPORT_MuxAlt3); // UART1_RX

    // 2. Get default configuration: 115200 baud, 8N1
    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = 115200U; // Set desired baud rate
    config.enableTx     = true;
    config.enableRx     = true;

    // 3. Initialize LPUART instance
    // Make sure clock source is configured correctly (e.g., kCLOCK_CoreSysClk)
    LPUART_Init(UART1, &config, CLOCK_GetFreq(kCLOCK_CoreSysClk));

    // 4. Create the semaphore used by the ISR
    xUARTRxSemaphore = xSemaphoreCreateBinary();
    if (xUARTRxSemaphore == NULL) {
        PRINTF("Error creating UART Rx Semaphore!\r\n");
        while(1); // Halt on error
    }
     vQueueAddToRegistry(xUARTRxSemaphore, "UARTRxSema"); // Optional: Name for debugger

    // 5. Enable UART1 receive interrupt in the peripheral
    LPUART_EnableInterrupts(UART1, kLPUART_RxDataRegFullInterruptEnable);

    // 6. Enable UART1 interrupt in the NVIC, set priority lower than configMAX_SYSCALL_INTERRUPT_PRIORITY
    // Priority must be numerically higher (lower logical priority) than max syscall priority
    // Example: If max syscall is 1, set this to 2 or 3.
    NVIC_SetPriority(UART1_IRQn, 3); // Example priority
    EnableIRQ(UART1_IRQn);

    PRINTF("UART1 Initialized for ESP32 Communication.\r\n");
}

// --- Send Command Helper ---
BaseType_t Send_Command_To_ESP32(const char *command) {
    // Acquire the UART mutex before accessing the peripheral
    if (xSemaphoreTake(xUARTMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Send the command string
        LPUART_WriteBlocking(UART1, (const uint8_t *)command, strlen(command));
        // Send a newline character as a command terminator
        LPUART_WriteBlocking(UART1, (const uint8_t *)"\n", 1);

        // Release the mutex
        xSemaphoreGive(xUARTMutex);
        return pdTRUE; // Success
    } else {
        PRINTF("Error: Could not acquire UART mutex to send command.\r\n");
        return pdFALSE; // Failed to acquire mutex
    }
}

// --- JSON Parsing Helper ---
// Basic parser, assumes simple structure: {"temp":xx.x,"humidity":yy.y}
// Error handling is minimal. Consider a more robust JSON library if needed.
BaseType_t Parse_DHT_Data(const char *json_string, float *temp, float *humidity) {
    // Find keys
    const char *temp_key = "\"temp\":";
    const char *hum_key = "\"humidity\":";
    char *temp_start = strstr(json_string, temp_key);
    char *hum_start = strstr(json_string, hum_key);

    if (temp_start && hum_start) {
        // Point after the key and colon
        temp_start += strlen(temp_key);
        hum_start += strlen(hum_key);

        // Convert the numeric part to float
        *temp = atof(temp_start);
        *humidity = atof(hum_start);

        // Basic validation (e.g., check if values are within expected range)
        if (*temp >= -40.0 && *temp <= 80.0 && *humidity >= 0.0 && *humidity <= 100.0) {
            return pdTRUE; // Success
        } else {
             PRINTF("Warning: Parsed DHT values out of range (T:%.1f, H:%.1f)\r\n", *temp, *humidity);
             return pdFALSE; // Parsed values seem invalid
        }
    }
    PRINTF("Error: Could not find 'temp' or 'humidity' keys in JSON: %s\r\n", json_string);
    return pdFALSE; // Keys not found
}


// --- ESP32 Communication Task ---
void ESP32_Communication_Task(void *pvParameters) {
    SensorData_t sensor_data_update; // Structure to hold parsed DHT data
    sensor_data_update.source = SENSOR_DHT11; // Identify the source

    TickType_t xLastWakeTime = xTaskGetTickCount(); // For vTaskDelayUntil

    PRINTF("ESP32 Communication Task Started.\r\n");

    for (;;) {
         // Use vTaskDelayUntil for precise periodic execution
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(DHT11_POLL_INTERVAL_MS));

        // 1. Send command to ESP32 to request DHT11 data
        if (Send_Command_To_ESP32("GET_DHT") == pdTRUE) {
            // 2. Wait for the ISR to signal that a complete response has arrived
            // Block on the semaphore with a timeout
            if (xSemaphoreTake(xUARTRxSemaphore, pdMS_TO_TICKS(UART_RX_TIMEOUT_MS)) == pdTRUE) {
                // Response received within timeout

                // 3. Acquire mutex again (briefly) to safely access rx_buffer
                if (xSemaphoreTake(xUARTMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    // Make a local copy of the buffer to parse outside the mutex lock
                    char local_rx_buffer[RX_BUFFER_SIZE];
                    strncpy(local_rx_buffer, rx_buffer, RX_BUFFER_SIZE -1);
                    local_rx_buffer[RX_BUFFER_SIZE - 1] = '\0'; // Ensure null termination
                    xSemaphoreGive(xUARTMutex); // Release mutex quickly

                    // 4. Parse the received data (local copy)
                    if (Parse_DHT_Data(local_rx_buffer, &sensor_data_update.value1, &sensor_data_update.value2) == pdTRUE) {
                        // Successfully parsed Temp and Humidity

                        // 5. Send the updated DHT11 data to the Sensor Queue
                        if (xQueueSend(xSensorQueue, &sensor_data_update, pdMS_TO_TICKS(100)) != pdPASS) {
                            PRINTF("Error: Failed to send DHT data to Sensor Queue.\r\n");
                        } else {
                             PRINTF("DHT Data Sent: T=%.1f H=%.1f\r\n", sensor_data_update.value1, sensor_data_update.value2);
                        }
                    } else {
                        // Parsing failed
                        PRINTF("Error: Failed to parse ESP32 response: %s\r\n", local_rx_buffer);
                    }
                } else {
                    PRINTF("Error: Could not acquire UART mutex to read buffer.\r\n");
                }
            } else {
                // Timeout waiting for ESP32 response
                PRINTF("Timeout: No response from ESP32 for GET_DHT command.\r\n");
                // Optionally send error values to the queue
                sensor_data_update.value1 = -99.9; // Indicate error
                sensor_data_update.value2 = -99.9; // Indicate error
                 xQueueSend(xSensorQueue, &sensor_data_update, 0); // Send error indicator
            }
        } else {
             PRINTF("Failed to send GET_DHT command (Mutex busy?).\r\n");
        }
    } // end for(;;)
}
