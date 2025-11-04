#ifndef DUMMY_TASKS_H_
#define DUMMY_TASKS_H_

#include "FreeRTOS.h"
#include "task.h"

// Dummy task simulating sensor polling (Member 1: Xiao Jielei)
void Dummy_Sensor_Polling_Task(void *pvParameters);

// Dummy task simulating actuator control (Member 2: Liu Zehui)
void Dummy_Actuator_Control_Task(void *pvParameters);

// Dummy task simulating plant logic (Member 4: Lim Zerui)
void Dummy_Plant_Logic_Task(void *pvParameters);

// Dummy task simulating the high-priority water level handler
void Dummy_Water_Danger_Task(void *pvParameters);

// Helper task to simulate the water level ISR giving the semaphore
void SimulateWaterISRTask(void *pvParameters);

#endif /* DUMMY_TASKS_H_ */
