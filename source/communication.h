#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

#include "FreeRTOS.h"
#include "task.h"

// Define the size of the buffer used in the ISR
// This MUST match RX_BUFFER_SIZE in communication.c
#define UART_RX_BUFFER_SIZE 64

/**
 * @brief Struct to pass a complete UART message from the ISR to the Recv Task.
 * Inspired by the TMessage struct in your lab example.
 */
typedef struct {
    char buffer[UART_RX_BUFFER_SIZE];
} UARTMessage_t;


// --- Function Prototypes ---

/**
 * @brief Initializes LPUART1 for communication with the ESP32.
 * Configures pins PTB16 (RX) and PTB17 (TX).
 */
void UART_Init(void);

/**
 * @brief (Task) Periodically sends "GET_DHT\n" to the ESP32.
 */
void ESP32_Send_Task(void *pvParameters);

/**
 * @brief (Task) Waits for complete messages from the ISR queue,
 * parses them, and sends DHT data to the xSensorQueue.
 */
void ESP32_Receive_Task(void *pvParameters);


#endif /* COMMUNICATION_H_ */
