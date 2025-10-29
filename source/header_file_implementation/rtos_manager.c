#include "rtos_manager.h"
#include "project_config.h" // For task stack sizes and priorities
#include "sensor_drivers.h"  // For SensorData_t struct definition
#include "actuator_drivers.h"// For ActuatorCommand_t struct definition
#include "fsl_debug_console.h" // For PRINTF

// --- Global RTOS Handles ---
// These handles are declared here and defined in main.c (or another global scope)
// Ensure they are DEFINED (not extern) in one C file, e.g., main.c
// Example definition in main.c:
//   QueueHandle_t xSensorQueue = NULL;
//   QueueHandle_t xActuatorQueue = NULL;
//   SemaphoreHandle_t xUARTMutex = NULL;
//   SemaphoreHandle_t xWaterLevelSemaphore = NULL;
extern QueueHandle_t xSensorQueue;
extern QueueHandle_t xActuatorQueue;
extern SemaphoreHandle_t xUARTMutex;
extern SemaphoreHandle_t xWaterLevelSemaphore;

// --- External Task Function Prototypes ---
// Declare the task functions that are implemented in other C files.
// Make sure these function signatures match their definitions exactly.
void Sensor_Polling_Task(void *pvParameters);
void Actuator_Control_Task(void *pvParameters);
void Plant_Logic_Task(void *pvParameters);
void ESP32_Communication_Task(void *pvParameters);
void Water_Danger_Task(void *pvParameters); // Task specifically for handling low water

/**
 * @brief Initializes the necessary FreeRTOS objects (Queues, Semaphores, Mutexes).
 */
void RTOS_Objects_Init(void) {
    // 1. Create the queue for sensor data (Photoresistor, DHT11)
    // Sensor task writes here, Logic task reads from here.
    xSensorQueue = xQueueCreate(SENSOR_QUEUE_LENGTH, sizeof(SensorData_t));
    if (xSensorQueue == NULL) {
        PRINTF("Error creating Sensor Queue!\r\n");
        // Handle error appropriately, e.g., enter an infinite loop or reset
        while(1);
    }
    vQueueAddToRegistry(xSensorQueue, "SensorQueue"); // Optional: Name for debugger view

    // 2. Create the queue for actuator commands (LED, Buzzer)
    // Logic task writes here, Actuator task reads from here.
    xActuatorQueue = xQueueCreate(ACTUATOR_QUEUE_LENGTH, sizeof(ActuatorCommand_t));
     if (xActuatorQueue == NULL) {
        PRINTF("Error creating Actuator Queue!\r\n");
        while(1);
    }
    vQueueAddToRegistry(xActuatorQueue, "ActuatorQueue");

    // 3. Create the mutex to protect UART1 communication with ESP32
    // Ensures only one task accesses UART1 at a time.
    xUARTMutex = xSemaphoreCreateMutex();
     if (xUARTMutex == NULL) {
        PRINTF("Error creating UART Mutex!\r\n");
        while(1);
    }
    vQueueAddToRegistry(xUARTMutex, "UARTMutex");

    // 4. Create the binary semaphore for the water level sensor ISR
    // ISR gives this semaphore, Water_Danger_Task takes it.
    xWaterLevelSemaphore = xSemaphoreCreateBinary();
     if (xWaterLevelSemaphore == NULL) {
        PRINTF("Error creating Water Level Semaphore!\r\n");
        while(1);
    }
    vQueueAddToRegistry(xWaterLevelSemaphore, "WaterLvlSema");

    PRINTF("RTOS Objects Initialized.\r\n");
}

/**
 * @brief Creates all the application-specific FreeRTOS tasks.
 */
//void RTOS_Tasks_Create(void) {
//    BaseType_t status;
//
//    // Task 1: Sensor Polling (Member 1: Xiao Jielei)
//    status = xTaskCreate(Sensor_Polling_Task,             // Task function
//                         "SensorTask",                  // Task name
//                         SENSOR_TASK_STACK_SIZE,        // Stack size in words
//                         NULL,                          // Task parameter
//                         SENSOR_TASK_PRIORITY,          // Task priority
//                         NULL);                         // Task handle (optional)
//    if (status != pdPASS) {
//        PRINTF("Error creating Sensor Task!\r\n");
//    }
//
//    // Task 2: Actuator Control (Member 2: Liu Zehui)
//    status = xTaskCreate(Actuator_Control_Task,
//                         "ActuatorTask",
//                         ACTUATOR_TASK_STACK_SIZE,
//                         NULL,
//                         ACTUATOR_TASK_PRIORITY,
//                         NULL);
//    if (status != pdPASS) {
//        PRINTF("Error creating Actuator Task!\r\n");
//    }
//
//    // Task 3: ESP32 Communication (Member 3: Yi Sheng - YOUR TASK)
//    status = xTaskCreate(ESP32_Communication_Task,
//                         "ESP32Task",
//                         COMM_TASK_STACK_SIZE,
//                         NULL,
//                         COMM_TASK_PRIORITY,
//                         NULL);
//     if (status != pdPASS) {
//        PRINTF("Error creating ESP32 Communication Task!\r\n");
//    }
//
//    // Task 4: Plant Logic (Member 4: Lim Zerui)
//    status = xTaskCreate(Plant_Logic_Task,
//                         "LogicTask",
//                         LOGIC_TASK_STACK_SIZE,
//                         NULL,
//                         LOGIC_TASK_PRIORITY,
//                         NULL);
//    if (status != pdPASS) {
//        PRINTF("Error creating Logic Task!\r\n");
//    }
//
//     // Task 5: Water Level Emergency Handler (Triggered by ISR via Semaphore)
//    status = xTaskCreate(Water_Danger_Task,
//                         "WaterDanger",
//                         WATER_DANGER_TASK_STACK_SIZE,
//                         NULL,
//                         WATER_DANGER_TASK_PRIORITY, // Highest priority
//                         NULL);
//    if (status != pdPASS) {
//        PRINTF("Error creating Water Danger Task!\r\n");
//    }
//
//    PRINTF("RTOS Tasks Created.\r\n");
//}

/**
 * @brief Creates all the application-specific FreeRTOS tasks (USING DUMMIES).
 */
void RTOS_Tasks_Create(void) {
    BaseType_t status;

    // Task 1: DUMMY Sensor Polling
    status = xTaskCreate(Dummy_Sensor_Polling_Task,       // Use Dummy Task function
                         "SensorTask",
                         SENSOR_TASK_STACK_SIZE,
                         NULL,
                         SENSOR_TASK_PRIORITY,
                         NULL);
    if (status != pdPASS) { PRINTF("Error creating Dummy Sensor Task!\r\n"); }

    // Task 2: DUMMY Actuator Control
    status = xTaskCreate(Dummy_Actuator_Control_Task,     // Use Dummy Task function
                         "ActuatorTask",
                         ACTUATOR_TASK_STACK_SIZE,
                         NULL,
                         ACTUATOR_TASK_PRIORITY,
                         NULL);
    if (status != pdPASS) { PRINTF("Error creating Dummy Actuator Task!\r\n"); }

    // Task 3: ESP32 Communication (YOUR REAL TASK)
    status = xTaskCreate(ESP32_Communication_Task,
                         "ESP32Task",
                         COMM_TASK_STACK_SIZE,
                         NULL,
                         COMM_TASK_PRIORITY,
                         NULL);
     if (status != pdPASS) { PRINTF("Error creating ESP32 Communication Task!\r\n"); }

    // Task 4: DUMMY Plant Logic
    status = xTaskCreate(Dummy_Plant_Logic_Task,          // Use Dummy Task function
                         "LogicTask",
                         LOGIC_TASK_STACK_SIZE,
                         NULL,
                         LOGIC_TASK_PRIORITY,
                         NULL);
    if (status != pdPASS) { PRINTF("Error creating Dummy Logic Task!\r\n"); }

    // Task 5: DUMMY Water Level Emergency Handler
    status = xTaskCreate(Dummy_Water_Danger_Task,         // Use Dummy Task function
                         "WaterDanger",
                         WATER_DANGER_TASK_STACK_SIZE,
                         NULL,
                         WATER_DANGER_TASK_PRIORITY,
                         NULL);
    if (status != pdPASS) { PRINTF("Error creating Dummy Water Danger Task!\r\n"); }

    // Task 6: Simulate ISR Task (for testing pre-emption)
     status = xTaskCreate(SimulateWaterISRTask,
                          "SimISR",
                          configMINIMAL_STACK_SIZE, // Small stack is fine
                          NULL,
                          tskIDLE_PRIORITY + 1,      // Low priority
                          NULL);
    if (status != pdPASS) { PRINTF("Error creating Simulate ISR Task!\r\n"); }


    PRINTF("RTOS Dummy Tasks Created.\r\n");
}
