#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

#include "FreeRTOS.h"
#include "semphr.h"
#include <stdint.h>

/**
 * @brief Initializes UART1 peripheral for communication with ESP32.
 *
 * Configures the pins, baud rate (115200), and enables receive interrupts.
 * Creates the semaphore used by the ISR to signal received messages.
 */
void UART_Init(void);

/**
 * @brief FreeRTOS task responsible for communicating with the ESP32.
 *
 * Periodically requests DHT11 data, waits for a response (signaled by ISR),
 * parses the response, and sends the updated sensor data to the SensorQueue.
 * Uses a mutex to protect UART access.
 *
 * @param pvParameters Unused task parameter.
 */
void ESP32_Communication_Task(void *pvParameters);

/**
 * @brief Sends a command string (null-terminated) to the ESP32 via UART1.
 *
 * This function handles acquiring the UART mutex for thread safety.
 * It appends a newline character '\\n' as a command terminator.
 *
 * @param command The null-terminated command string to send.
 * @return pdTRUE if successful, pdFALSE if mutex could not be taken.
 */
BaseType_t Send_Command_To_ESP32(const char *command);

/**
 * @brief Parses a string (expected JSON format) received from ESP32 for DHT11 data.
 *
 * Example format: {"temp":25.5,"humidity":60.2}
 *
 * @param json_string The null-terminated string received from ESP32.
 * @param temp Pointer to a float where the parsed temperature will be stored.
 * @param humidity Pointer to a float where the parsed humidity will be stored.
 * @return pdTRUE if parsing was successful, pdFALSE otherwise.
 */
BaseType_t Parse_DHT_Data(const char *json_string, float *temp, float *humidity);

#endif /* COMMUNICATION_H_ */
