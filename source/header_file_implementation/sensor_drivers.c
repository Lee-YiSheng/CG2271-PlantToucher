// sensor_drivers.c
#include "../header files/project_config.h"

// 模拟传感器数据 - 避免复杂的ADC驱动
uint32_t Read_Light_Sensor(void) {
    // 返回模拟的光敏电阻值
    return 500 + (rand() % 200);  // 500-700之间的随机值
}

void Read_DHT11_From_ESP32(float *temp, float *humidity) {
    // 模拟DHT11数据
    *temp = 25.0f + ((rand() % 100) / 10.0f);      // 25.0-35.0°C
    *humidity = 60.0f + ((rand() % 200) / 10.0f);  // 60.0-80.0%
}

uint32_t Get_Ultrasonic_Distance(void) {
    // 模拟超声波距离
    return 20 + (rand() % 50);  // 20-70cm
}

void Calculate_Distance(void) {
    // 超声波距离计算 - 这里可以放置实际的计算逻辑
    // 目前只是模拟
}
