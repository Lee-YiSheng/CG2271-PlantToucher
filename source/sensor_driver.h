#ifndef SENSOR_DRIVER_H_
#define SENSOR_DRIVER_H_

#include <stdint.h>

/**
 * @brief Enum to identify the source of the sensor data.
 */
typedef enum {
    SENSOR_PHOTORESISTOR, // Data from photoresistor (light)
    SENSOR_DHT11,         // Data from ESP32 (temp/humidity)
    SENSOR_WATER_LEVEL    // Data from water level sensor
} SensorSource_t;

/**
 * @brief The one, official struct for all sensor data.
 * Based on your framework doc [cite: 94-100] AND adding a 'source' field.
 */
typedef struct {
    SensorSource_t source;     // <-- ADDED THIS FIELD
    uint32_t water_level;      // Valid if source is SENSOR_WATER_LEVEL
    uint32_t light_intensity;  // Valid if source is SENSOR_PHOTORESISTOR
    float temperature;         // Valid if source is SENSOR_DHT11
    float humidity;            // Valid if source is SENSOR_DHT11
} SensorData_t;


/*
 * These are the functions from your sensor C file
 */
void initSensors(void);
uint32_t ReadPhotoresistor(void);

// Add prototypes for your tasks if they are defined in this C file
void Sensor_Task(void *pvParameters);
// 函数声明
void Ultrasonic_Init(void);
uint32_t Get_Ultrasonic_Distance(void);
void Calculate_Distance(void);
uint32_t Read_Light_Sensor(void);

// 中断处理函数声明
void Ultrasonic_ISR(void);

void Read_DHT11_From_ESP32(float *temperature, float *humidity);

#endif
