// sensor_task.c
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "../header files/sensor_drivers.h"  // 自定义传感器驱动头文件
#include "../header files/project_config.h"

// 外部声明的全局变量（在main.c中定义）
extern QueueHandle_t xSensorQueue;
extern SemaphoreHandle_t xUltrasonicSemaphore;
extern SemaphoreHandle_t xUARTMutex;

void Sensor_Task(void *pvParameters) {
    SensorData_t sensor_data;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for(;;) {
        // 轮询读取DHT11（通过ESP32）
        if(xSemaphoreTake(xUARTMutex, portMAX_DELAY)) {
            Read_DHT11_From_ESP32(&sensor_data.temperature, &sensor_data.humidity);
            xSemaphoreGive(xUARTMutex);
        }

        // 轮询读取光敏电阻
        sensor_data.light = Read_Light_Sensor();

        // 等待超声波数据（通过信号量）
        if(xSemaphoreTake(xUltrasonicSemaphore, 100 / portTICK_PERIOD_MS)) {
            sensor_data.distance = Get_Ultrasonic_Distance();
        }

        // 发送到队列
        if(xQueueSend(xSensorQueue, &sensor_data, 0) != pdPASS) {
            // 队列满处理
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}

// 超声波中断服务函数
void Ultrasonic_ISR(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // 处理回波时间计算
    Calculate_Distance();

    // 给出信号量
    xSemaphoreGiveFromISR(xUltrasonicSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
