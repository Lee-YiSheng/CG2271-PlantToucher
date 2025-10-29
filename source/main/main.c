// main.c
#include "../header files/project_config.h"
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"


// 全局变量定义
QueueHandle_t xSensorQueue;
QueueHandle_t xGPTQueue;
SemaphoreHandle_t xUARTMutex;
SemaphoreHandle_t xUltrasonicSemaphore;

// 任务声明
void Sensor_Task(void *pvParameters);
void Actuator_Task(void *pvParameters);
void ESP_Comm_Task(void *pvParameters);

// 简化版中断设置
void Setup_Ultrasonic_Interrupt(void) {
    // 这里配置GPIO中断
    // 暂时使用模拟方式
}

int main(void) {
    // 硬件初始化
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();

    // 创建RTOS对象 - 减小内存使用
    xSensorQueue = xQueueCreate(2, sizeof(SensorData_t));      // 减小队列大小
    xGPTQueue = xQueueCreate(2, sizeof(GPTCommand_t));
    xUARTMutex = xSemaphoreCreateMutex();
    xUltrasonicSemaphore = xSemaphoreCreateBinary();

    // 创建任务 - 减小栈大小
    xTaskCreate(Sensor_Task, "Sensor_Task", 128, NULL, 2, NULL);
    xTaskCreate(Actuator_Task, "Actuator_Task", 128, NULL, 2, NULL);
    xTaskCreate(ESP_Comm_Task, "ESP_Comm_Task", 256, NULL, 3, NULL);

    // 配置中断
    Setup_Ultrasonic_Interrupt();

    // 启动调度器
    vTaskStartScheduler();

    for(;;);
    return 0;
}
