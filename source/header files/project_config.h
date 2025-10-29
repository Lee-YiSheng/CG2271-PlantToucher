#ifndef PROJECT_CONFIG_H_
#define PROJECT_CONFIG_H_

#include "FreeRTOS.h"
#include "task.h"

// --- RTOS Object Sizes ---
#define SENSOR_QUEUE_LENGTH     10 // Max items in sensor data queue
#define ACTUATOR_QUEUE_LENGTH   5  // Max items in actuator command queue

// --- Task Stack Sizes (in words) ---
// Adjust these based on task complexity and local variable usage
#define SENSOR_TASK_STACK_SIZE      (configMINIMAL_STACK_SIZE + 100)
#define ACTUATOR_TASK_STACK_SIZE    (configMINIMAL_STACK_SIZE + 100)
#define COMM_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE + 150) // UART/Parsing might need more
#define LOGIC_TASK_STACK_SIZE       (configMINIMAL_STACK_SIZE + 100)
#define WATER_DANGER_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE + 50) // Simple task

// --- Task Priorities ---
// Higher number = higher priority
#define WATER_DANGER_TASK_PRIORITY  (tskIDLE_PRIORITY + 4) // Highest priority for critical safety
#define COMM_TASK_PRIORITY          (tskIDLE_PRIORITY + 3) // High priority for responsiveness
#define SENSOR_TASK_PRIORITY        (tskIDLE_PRIORITY + 2) // Medium priority for data gathering
#define LOGIC_TASK_PRIORITY         (tskIDLE_PRIORITY + 1) // Lower priority for decision making
#define ACTUATOR_TASK_PRIORITY      (tskIDLE_PRIORITY + 1) // Lower priority (runs with time-slicing with Logic)

// --- Application Specific ---
#define DHT11_POLL_INTERVAL_MS      5000 // How often to request DHT11 data (5 seconds)
#define UART_RX_TIMEOUT_MS          1000 // Timeout waiting for ESP32 response

#endif /* PROJECT_CONFIG_H_ */
