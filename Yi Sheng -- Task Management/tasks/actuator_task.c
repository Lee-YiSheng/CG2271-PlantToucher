// actuator_task.c
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "board.h"
#include "FreeRTOS.h"
#include "project_config.h"

extern QueueHandle_t xGPTQueue;

void Actuator_Task(void *pvParameters) {
    GPTCommand_t gpt_cmd;

    for(;;) {
        // 从队列接收GPT指令
        if(xQueueReceive(xGPTQueue, &gpt_cmd, portMAX_DELAY)) {

            // 控制LED
            if(gpt_cmd.led_cmd == 1) {
                GPIO_PinWrite(LED_GPIO, LED_PIN, 1);
            } else {
                GPIO_PinWrite(LED_GPIO, LED_PIN, 0);
            }

            // 控制蜂鸣器
            if(gpt_cmd.buzzer_cmd == 1) {
                GPIO_PinWrite(BUZZER_GPIO, BUZZER_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(200));
                GPIO_PinWrite(BUZZER_GPIO, BUZZER_PIN, 0);
            }
        }
    }
}
