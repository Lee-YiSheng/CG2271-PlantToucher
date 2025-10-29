// esp_comm_task.c
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "string.h"
#include "stdio.h"
#include "../header files/project_config.h"
#include "../header files/uart_driver.h"

extern QueueHandle_t xSensorQueue;
extern QueueHandle_t xGPTQueue;
extern SemaphoreHandle_t xUARTMutex;
bool testing = true;

void ESP_Comm_Task(void *pvParameters) {
    SensorData_t sensor_data;
    GPTCommand_t gpt_cmd;
    char json_buffer[256];
    if (testing) {
    	PRINTF("ESP32 Communication Task Started.\r\n");
    }
    for(;;) {
        // 从队列接收传感器数据
        if(xQueueReceive(xSensorQueue, &sensor_data, portMAX_DELAY)) {

            if(xSemaphoreTake(xUARTMutex, portMAX_DELAY)) {
                // 构建JSON发送到ESP32
                snprintf(json_buffer, sizeof(json_buffer),
                    "{\"temp\":%.1f,\"humidity\":%.1f,\"distance\":%d,\"light\":%d}",
                    sensor_data.temperature, sensor_data.humidity,
                    sensor_data.distance, sensor_data.light);

                Send_To_ESP32(json_buffer);

                // 接收GPT响应
                if(Receive_From_ESP32(json_buffer, sizeof(json_buffer))) {
                    // 解析GPT指令
                    Parse_GPT_Response(json_buffer, &gpt_cmd);

                    // 发送到执行器队列
                    xQueueSend(xGPTQueue, &gpt_cmd, 0);
                }

                xSemaphoreGive(xUARTMutex);
            }
        }
    }
}
