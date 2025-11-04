#include "dummy_tasks.h"
#include "project_config.h"
#include "sensor_driver.h"  // For SensorData_t
#include "actuator_driver.h"// For ActuatorCommand_t
#include "fsl_debug_console.h" // For PRINTF
#include "queue.h"
#include "semphr.h"

// --- Global RTOS Handles (defined in main.c or rtos_manager.c) ---
extern QueueHandle_t xSensorQueue;
extern QueueHandle_t xActuatorQueue;
extern SemaphoreHandle_t xWaterLevelSemaphore;

// --- Dummy Task Implementations ---

/**
 * @brief Dummy task simulating sensor polling.
 * Sends fake photoresistor data periodically to xSensorQueue.
 */
void Dummy_Sensor_Polling_Task(void *pvParameters) {
    PRINTF("[Dummy Sensor Task]: Started.\r\n");
    SensorData_t fake_sensor_data;
    fake_sensor_data.source = SENSOR_PHOTORESISTOR; // Simulate photoresistor
    uint16_t light_value = 500; // Starting light value

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(2000); // Simulate polling every 2 seconds

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency); // Wait for the next cycle

        // Simulate changing light value
        light_value = (light_value + 50) % 1024; // Cycle between 0-1023 approx
        fake_sensor_data.value1 = (float)light_value;
        fake_sensor_data.value2 = 0; // Not used for photoresistor

        PRINTF("[Dummy Sensor Task]: Sending Light Data = %.0f\r\n", fake_sensor_data.value1);

        // Send data to the sensor queue
        if (xQueueSend(xSensorQueue, &fake_sensor_data, pdMS_TO_TICKS(100)) != pdPASS) {
            PRINTF("[Dummy Sensor Task]: Failed to send to Sensor Queue!\r\n");
        }
    }
}

/**
 * @brief Dummy task simulating actuator control.
 * Waits for commands on xActuatorQueue and prints them.
 */
void Dummy_Actuator_Control_Task(void *pvParameters) {
    PRINTF("[Dummy Actuator Task]: Started. Waiting for commands...\r\n");
    ActuatorCommand_t received_command;

    for (;;) {
        // Wait indefinitely for a command
        if (xQueueReceive(xActuatorQueue, &received_command, portMAX_DELAY) == pdPASS) {
            PRINTF("[Dummy Actuator Task]: Received Command -> Type: %d, Val1: %lu, Val2: %lu\r\n",
                   received_command.type, received_command.value1, received_command.value2);

            // Simulate doing something based on the command
            switch(received_command.type) {
                case ACTUATOR_LED:
                    PRINTF("    -> Simulating Set LED Brightness to %lu%%\r\n", received_command.value1);
                    break;
                case ACTUATOR_BUZZER_TONE:
                     PRINTF("    -> Simulating Play Buzzer Tone: Freq=%lu Hz, Duration=%lu ms\r\n",
                            received_command.value1, received_command.value2);
                    break;
                 case ACTUATOR_BUZZER_MUSIC_HAPPY:
                     PRINTF("    -> Simulating Play Happy Music\r\n");
                    break;
                 case ACTUATOR_BUZZER_MUSIC_STRESSED:
                     PRINTF("    -> Simulating Play Stressed Music\r\n");
                    break;
                default:
                    PRINTF("    -> Simulating Unknown Actuator Command\r\n");
                    break;
            }
        }
        // Task blocks on xQueueReceive, no vTaskDelay needed here
    }
}

/**
 * @brief Dummy task simulating plant logic.
 * Waits for data on xSensorQueue, prints it, and sends dummy commands to xActuatorQueue.
 */
void Dummy_Plant_Logic_Task(void *pvParameters) {
    PRINTF("[Dummy Logic Task]: Started. Waiting for sensor data...\r\n");
    SensorData_t received_data;
    ActuatorCommand_t command_to_send;

    for (;;) {
        // Wait indefinitely for sensor data
        if (xQueueReceive(xSensorQueue, &received_data, portMAX_DELAY) == pdPASS) {
             PRINTF("[Dummy Logic Task]: Received Sensor Data -> Source: %d, Val1: %.1f, Val2: %.1f\r\n",
                   received_data.source, received_data.value1, received_data.value2);

             // Simulate decision making based on received data
             if (received_data.source == SENSOR_PHOTORESISTOR) {
                 if (received_data.value1 < 300) { // If light is low
                     command_to_send.type = ACTUATOR_LED;
                     command_to_send.value1 = 80; // Set LED to 80%
                     command_to_send.value2 = 0;
                     PRINTF("[Dummy Logic Task]: Light low, sending LED command.\r\n");
                     xQueueSend(xActuatorQueue, &command_to_send, pdMS_TO_TICKS(100));
                 } else if (received_data.value1 > 800) { // If light is high
                      command_to_send.type = ACTUATOR_LED;
                     command_to_send.value1 = 10; // Set LED to 10%
                     command_to_send.value2 = 0;
                      PRINTF("[Dummy Logic Task]: Light high, sending LED command.\r\n");
                     xQueueSend(xActuatorQueue, &command_to_send, pdMS_TO_TICKS(100));
                 }
             } else if (received_data.source == SENSOR_DHT11) {
                 if (received_data.value1 > 30.0) { // If temperature is high
                     command_to_send.type = ACTUATOR_BUZZER_MUSIC_STRESSED;
                     command_to_send.value1 = 0;
                     command_to_send.value2 = 0;
                     PRINTF("[Dummy Logic Task]: Temp high, sending Stressed Music command.\r\n");
                     xQueueSend(xActuatorQueue, &command_to_send, pdMS_TO_TICKS(100));
                 }
             }
        }
         // Task blocks on xQueueReceive
    }
}

/**
 * @brief Dummy task simulating the high-priority water level handler.
 * Waits for the xWaterLevelSemaphore and prints an alert. Sends command.
 */
void Dummy_Water_Danger_Task(void *pvParameters) {
    PRINTF("[Dummy Water Danger Task]: Started. Waiting for low water signal...\r\n");
    ActuatorCommand_t command_to_send;

    for(;;) {
        // Wait indefinitely for the semaphore from the simulated ISR
        if (xSemaphoreTake(xWaterLevelSemaphore, portMAX_DELAY) == pdTRUE) {
            PRINTF("\r\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
            PRINTF("[Dummy Water Danger Task]: LOW WATER LEVEL DETECTED!\r\n");
            PRINTF("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");

            // Simulate sending urgent commands (e.g., flash LED, loud buzzer tone)
            command_to_send.type = ACTUATOR_LED;
            command_to_send.value1 = 100; // Max brightness
            command_to_send.value2 = 0;
            xQueueSend(xActuatorQueue, &command_to_send, 0); // Send immediately

            command_to_send.type = ACTUATOR_BUZZER_TONE;
            command_to_send.value1 = 2000; // High pitch tone
            command_to_send.value2 = 1000; // Play for 1 second
             xQueueSend(xActuatorQueue, &command_to_send, 0);

            // Simulate task might suspend or wait for condition to clear after alerting
            vTaskDelay(pdMS_TO_TICKS(5000)); // Delay for 5 seconds before waiting again
            PRINTF("[Dummy Water Danger Task]: Alert finished, waiting again...\r\n");
        }
    }
}


/**
 * @brief Helper task to simulate the water level ISR giving the semaphore after a delay.
 * Used for testing pre-emption. Runs only once.
 */
void SimulateWaterISRTask(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(10000); // Wait for 10 seconds
    PRINTF("[Simulate ISR Task]: Started. Will give water semaphore in 10 seconds.\r\n");

    vTaskDelay(xDelay);

    PRINTF("[Simulate ISR Task]: Giving Water Level Semaphore now!\r\n");
    xSemaphoreGive(xWaterLevelSemaphore); // Simulate the ISR action

    PRINTF("[Simulate ISR Task]: Semaphore given. Suspending self.\r\n");
    vTaskSuspend(NULL); // Suspend this task after running once
}
