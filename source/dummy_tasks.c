#include "dummy_tasks.h"
#include "project_config.h"
#include "fsl_debug_console.h"
#include "queue.h"
#include "semphr.h"

// --- FIX: Include the headers defining the new structs ---
#include "sensor_driver.h"
#include "actuator_driver.h"

extern QueueHandle_t xSensorQueue;
extern QueueHandle_t xActuatorQueue;
extern SemaphoreHandle_t xWaterLevelSemaphore;

/**
 * @brief Dummy: Simulates Sensor Polling (Photoresistor)
 */
void Dummy_Sensor_Polling_Task(void *pvParameters) {
    PRINTF("[Dummy Sensor Task]: Started.\r\n");
    SensorData_t fake_sensor_data;

    // --- FIX: Set the source ---
    fake_sensor_data.source = SENSOR_PHOTORESISTOR;
    fake_sensor_data.water_level = 0; // Not from this sensor
    fake_sensor_data.temperature = 0; // Not from this sensor
    fake_sensor_data.humidity = 0;    // Not from this sensor

    uint16_t light_value = 500;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(2000);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        light_value = (light_value + 50) % 1024;

        // --- FIX: Use the correct struct member ---
        fake_sensor_data.light_intensity = light_value;

        PRINTF("[Dummy Sensor Task]: Sending Light Data = %lu\r\n", fake_sensor_data.light_intensity);

        if (xQueueSend(xSensorQueue, &fake_sensor_data, pdMS_TO_TICKS(100)) != pdPASS) {
            PRINTF("[Dummy Sensor Task]: Failed to send to Sensor Queue!\r\n");
        }
    }
}

/**
 * @brief Dummy: Simulates Actuator Control
 */
void Dummy_Actuator_Control_Task(void *pvParameters) {
    PRINTF("[Dummy Actuator Task]: Started. Waiting for commands...\r\n");

    // --- FIX: Use the new ActuatorCommand_t struct ---
    ActuatorCommand_t received_command;

    for (;;) {
        if (xQueueReceive(xActuatorQueue, &received_command, portMAX_DELAY) == pdPASS) {

            // --- FIX: Print the new struct members ---
            PRINTF("[Dummy Actuator Task]: Received Command -> LED: %u, Music: %d\r\n",
                   received_command.led_intensity, received_command.buzzer_command);

            // --- Simulate doing something ---
            // Set_LED_Intensity(received_command.led_intensity);
            // Play_Music(received_command.buzzer_command);
        }
    }
}

/**
 * @brief Dummy: Simulates Plant Logic
 */
void Dummy_Plant_Logic_Task(void *pvParameters) {
    PRINTF("[Dummy Logic Task]: Started. Waiting for sensor data...\r\n");

    // --- FIX: Use the new struct definitions ---
    SensorData_t received_data;
    ActuatorCommand_t command_to_send;

    for (;;) {
        if (xQueueReceive(xSensorQueue, &received_data, portMAX_DELAY) == pdPASS) {

            // --- FIX: Check the .source field to make decisions ---
            switch (received_data.source) {
                case SENSOR_PHOTORESISTOR:
                    PRINTF("[Dummy Logic Task]: Received Light Data = %lu\r\n", received_data.light_intensity);
                    if (received_data.light_intensity < 300) {
                        command_to_send.led_intensity = 200; // Bright
                        command_to_send.buzzer_command = MUSIC_OFF;
                        PRINTF("    -> Logic: Light low, sending LED command.\r\n");
                        xQueueSend(xActuatorQueue, &command_to_send, 0);
                    } else if (received_data.light_intensity > 800) {
                        command_to_send.led_intensity = 20; // Dim
                        command_to_send.buzzer_command = MUSIC_HAPPY;
                        PRINTF("    -> Logic: Light good, sending LED command.\r\n");
                        xQueueSend(xActuatorQueue, &command_to_send, 0);
                    }
                    break;

                case SENSOR_DHT11:
                    PRINTF("[Dummy Logic Task]: Received DHT Data = T:%.1f, H:%.1f\r\n",
                           received_data.temperature, received_data.humidity);
                    if (received_data.temperature > 30.0) {
                        command_to_send.led_intensity = 50;
                        command_to_send.buzzer_command = MUSIC_SAD; // Too hot
                        PRINTF("    -> Logic: Temp high, sending SAD music command.\r\n");
                        xQueueSend(xActuatorQueue, &command_to_send, 0);
                    }
                    break;

                case SENSOR_WATER_LEVEL:
                    // This logic would be for your real sensor task
                    PRINTF("[Dummy Logic Task]: Received Water Data = %lu\r\n", received_data.water_level);
                    break;
            }
        }
    }
}

/**
 * @brief Dummy: Simulates high-priority water handler (triggered by semaphore)
 */
void Dummy_Water_Danger_Task(void *pvParameters) {
    PRINTF("[Dummy Water Danger Task]: Started. Waiting for low water signal...\r\n");

    // --- FIX: Use the new ActuatorCommand_t ---
    ActuatorCommand_t alert_command;
    alert_command.led_intensity = 255; // Max brightness flash
    alert_command.buzzer_command = MUSIC_ALERT;

    for(;;) {
        if (xSemaphoreTake(xWaterLevelSemaphore, portMAX_DELAY) == pdTRUE) {
            PRINTF("\r\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
            PRINTF("[Dummy Water Danger Task]: LOW WATER LEVEL DETECTED!\r\n");
            PRINTF("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");

            // --- FIX: Send the new command struct ---
            PRINTF("    -> Sending ALERT command to Actuator Task.\r\n");
            xQueueSend(xActuatorQueue, &alert_command, 0); // Send immediately

            vTaskDelay(pdMS_TO_TICKS(5000));
            PRINTF("[Dummy Water Danger Task]: Alert finished, waiting again...\r\n");
        }
    }
}

/**
 * @brief Helper task to simulate the water level ISR giving the semaphore.
 */
void SimulateWaterISRTask(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(10000);
    PRINTF("[Simulate ISR Task]: Started. Will give water semaphore in 10 seconds.\r\n");

    vTaskDelay(xDelay);

    PRINTF("[Simulate ISR Task]: Giving Water Level Semaphore now!\r\n");
    xSemaphoreGive(xWaterLevelSemaphore);

    PRINTF("[Simulate ISR Task]: Semaphore given. Suspending self.\r\n");
    vTaskSuspend(NULL);
}
