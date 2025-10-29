// uart_driver.h
#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>

void UART_Init(void);
void Send_To_ESP32(const char *data);
int Receive_From_ESP32(char *buffer, uint32_t size);
void Parse_GPT_Response(const char *json, GPTCommand_t *cmd);

#endif
