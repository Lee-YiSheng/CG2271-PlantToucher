// uart_driver.c
#include "../header files/project_config.h"

void Send_To_ESP32(const char *data) {
    // 使用板载配置的UART
    LPUART_WriteBlocking(LPUART0, (const uint8_t *)data, strlen(data));
    LPUART_WriteBlocking(LPUART0, (const uint8_t *)"\n", 1);
}

int Receive_From_ESP32(char *buffer, uint32_t size) {
    uint32_t index = 0;
    uint8_t received_char;
    uint32_t timeout = 1000000;

    while (index < size - 1 && timeout--) {
        if (LPUART_GetStatusFlags(LPUART0) & kLPUART_RxDataRegFullFlag) {
            received_char = LPUART_ReadByte(LPUART0);
            buffer[index++] = received_char;

            if (received_char == '\n') {
                break;
            }
        }
    }

    buffer[index] = '\0';
    return (index > 0) ? 1 : 0;
}

void Parse_GPT_Response(const char *json, GPTCommand_t *cmd) {
    // 默认值
    cmd->led_cmd = 0;
    cmd->buzzer_cmd = 0;

    // 简单的字符串查找
    if (strstr(json, "\"led\":1") != NULL) {
        cmd->led_cmd = 1;
    }

    if (strstr(json, "\"buzzer\":1") != NULL) {
        cmd->buzzer_cmd = 1;
    }
}
