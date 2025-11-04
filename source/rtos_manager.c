/*
 * rtos_manager.c
 *
 * Initializes RTOS objects and creates all tasks.
 */

#include "rtos_manager.h"
#include "project_config.h"
#include "sensor_driver.h"
#include "actuator_driver.h"
#include "fsl_debug_console.h" // For PRINTF

// --- FIX: Added includes for the tasks being created ---
#include "dummy_tasks.h"
#include "communication.h"

// Prototypes for real tasks (if not in a header) would go here
// void Sensor_Polling_Task(void *pvParameters);
// void Actuator_Control_Task(void *pvParameters);
// void Plant_Logic_Task(void *pvParameters);
// void Water_Danger_Task(void *pvParameters);
// -----------------------------------------------------


// --- Global RTOS Handles (defined in main.c) ---
extern QueueHandle_t xSensorQueue;
extern QueueHandle_t xActuatorQueue;
extern SemaphoreHandle_t xUARTMutex;
extern SemaphoreHandle_t xWaterLevelSemaphore;
extern QueueHandle_t xUARTRxQueue;

/**
 * @brief Initializes the necessary FreeRTOS objects (Queues, Semaphores, Mutexes).
 */
void RTOS_Objects_Init(void) {
    // 1. Create the queue for sensor data
    // This now uses the size of the *new* SensorData_t
    xSensorQueue = xQueueCreate(SENSOR_QUEUE_LENGTH, sizeof(SensorData_t));
    if (xSensorQueue == NULL) {
        PRINTF("Error creating Sensor Queue!\r\n");
        while(1);
    }
    vQueueAddToRegistry(xSensorQueue, "SensorQueue");

    // 2. Create the queue for actuator commands
    // This now uses the size of the *new* ActuatorCommand_t
    xActuatorQueue = xQueueCreate(ACTUATOR_QUEUE_LENGTH, sizeof(ActuatorCommand_t));
     if (xActuatorQueue == NULL) {
        PRINTF("Error creating Actuator Queue!\r\n");
        while(1);
    }
    vQueueAddToRegistry(xActuatorQueue, "ActuatorQueue");

    // 3. Create the mutex to protect UART2 communication
    xUARTMutex = xSemaphoreCreateMutex();
     if (xUARTMutex == NULL) {
        PRINTF("Error creating UART Mutex!\r\n");
        while(1);
    }
    vQueueAddToRegistry(xUARTMutex, "UARTMutex");

    // 4. Create the binary semaphore for the water level sensor ISR
    xWaterLevelSemaphore = xSemaphoreCreateBinary();
     if (xWaterLevelSemaphore == NULL) {
        PRINTF("Error creating Water Level Semaphore!\r\n");
        while(1);
    }
    vQueueAddToRegistry(xWaterLevelSemaphore, "WaterLvlSema");


    // 5. Create the queue for the UART ISR to send strings to the Recv Task
        xUARTRxQueue = xQueueCreate(5, sizeof(UARTMessage_t)); // 5 messages, each a UARTMessage_t struct
         if (xUARTRxQueue == NULL) {
            PRINTF("Error creating UART Rx Queue!\r\n");
            while(1);
        }
        vQueueAddToRegistry(xUARTRxQueue, "UARTTxQueue");

        PRINTF("RTOS Objects Initialized.\r\n");
}


/**
 * @brief Creates all the application-specific FreeRTOS tasks (USING DUMMIES).
 */
void RTOS_Tasks_Create(void) {
    BaseType_t status;

    // Task 1: DUMMY Sensor Polling
    status = xTaskCreate(Dummy_Sensor_Polling_Task,
                         "SensorTask",
                         SENSOR_TASK_STACK_SIZE,
                         NULL,
                         SENSOR_TASK_PRIORITY,
                         NULL);
    if (status != pdPASS) { PRINTF("Error creating Dummy Sensor Task!\r\n"); }

    // Task 2: DUMMY Actuator Control
    status = xTaskCreate(Dummy_Actuator_Control_Task,
                         "ActuatorTask",
                         ACTUATOR_TASK_STACK_SIZE,
                         NULL,
                         ACTUATOR_TASK_PRIORITY,
                         NULL);
    if (status != pdPASS) { PRINTF("Error creating Dummy Actuator Task!\r\n"); }

    // Task 3a: ESP32 Send Task
        status = xTaskCreate(ESP32_Send_Task,
                             "ESP32Send",
                             COMM_TASK_STACK_SIZE,
                             NULL,
                             COMM_TASK_PRIORITY, // Use the same priority
                             NULL);
         if (status != pdPASS) { PRINTF("Error creating ESP32 Send Task!\r\n"); }

        // Task 3b: ESP32 Receive Task
        status = xTaskCreate(ESP32_Receive_Task,
                             "ESP32Recv",
                             COMM_TASK_STACK_SIZE,
                             NULL,
                             COMM_TASK_PRIORITY, // Use the same priority
                             NULL);
         if (status != pdPASS) { PRINTF("Error creating ESP32 Receive Task!\r\n"); }

    // Task 4: DUMMY Plant Logic
    status = xTaskCreate(Dummy_Plant_Logic_Task,
                         "LogicTask",
                         LOGIC_TASK_STACK_SIZE,
                         NULL,
                         LOGIC_TASK_PRIORITY,
                         NULL);
    if (status != pdPASS) { PRINTF("Error creating Dummy Logic Task!\r\n"); }

    // Task 5: DUMMY Water Level Emergency Handler
    status = xTaskCreate(Dummy_Water_Danger_Task,
                         "WaterDanger",
                         WATER_DANGER_TASK_STACK_SIZE,
                         NULL,
                         WATER_DANGER_TASK_PRIORITY,
                         NULL);
    if (status != pdPASS) { PRINTF("Error creating Dummy Water Danger Task!\r\n"); }

    // Task 6: Simulate ISR Task (for testing pre-emption)
     status = xTaskCreate(SimulateWaterISRTask,
                          "SimISR",
                          configMINIMAL_STACK_SIZE,
                          NULL,
                          tskIDLE_PRIORITY + 1,
                          NULL);
    if (status != pdPASS) { PRINTF("Error creating Simulate ISR Task!\r\n"); }


    PRINTF("RTOS Dummy Tasks Created.\r\n");
}
