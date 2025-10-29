// sensor_drivers.h
#ifndef SENSOR_DRIVERS_H
#define SENSOR_DRIVERS_H

#include <stdint.h>

// 函数声明
void Ultrasonic_Init(void);
uint32_t Get_Ultrasonic_Distance(void);
void Calculate_Distance(void);
uint32_t Read_Light_Sensor(void);

// 中断处理函数声明
void Ultrasonic_ISR(void);

void Read_DHT11_From_ESP32(float *temperature, float *humidity);

#endif
