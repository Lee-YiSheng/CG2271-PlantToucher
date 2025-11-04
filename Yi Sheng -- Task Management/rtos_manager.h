#ifndef RTOS_MANAGER_H_
#define RTOS_MANAGER_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/**
 * @brief Initializes the necessary FreeRTOS objects (Queues, Semaphores, Mutexes).
 *
 * This function must be called before RTOS_Tasks_Create and before the scheduler starts.
 * It creates the shared resources used for inter-task communication and synchronization.
 */
void RTOS_Objects_Init(void);

/**
 * @brief Creates all the application-specific FreeRTOS tasks.
 *
 * This function uses xTaskCreate to instantiate each task required by the system.
 * It must be called after RTOS_Objects_Init and before the scheduler starts.
 */
void RTOS_Tasks_Create(void);

#endif /* RTOS_MANAGER_H_ */
