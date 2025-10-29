/*
 * Copyright 2016-2023 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    dummymain.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MCXC444.h"
#include "fsl_debug_console.h"

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

/* Project specific includes */
#include "project_config.h"
#include "rtos_manager.h"   // Include your RTOS setup functions
#include "communication.h"  // Include UART init and task
#include "sensor_drivers.h"  // For SensorData_t struct definition (used by queues)
#include "actuator_drivers.h"// For ActuatorCommand_t struct definition (used by queues)
#include "dummy_tasks.h"    // Include dummy task declarations for testing

/*-----------------------------------------------------------*/
/* Global RTOS Handle Definitions                   */
/*-----------------------------------------------------------*/
// Define the handles that are declared as 'extern' in other files
QueueHandle_t xSensorQueue = NULL;
QueueHandle_t xActuatorQueue = NULL;
SemaphoreHandle_t xUARTMutex = NULL;
SemaphoreHandle_t xWaterLevelSemaphore = NULL; // Use this name consistently

/*-----------------------------------------------------------*/
/* Main Function                         */
/*-----------------------------------------------------------*/
int main(void) {

    /* Init board hardware. */
    BOARD_InitBootPins();       // Initialize pin muxing
    BOARD_InitBootClocks();     // Initialize system clocks
    BOARD_InitBootPeripherals();// Initialize peripherals configured by tools (e.g., TPM)
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();   // Initialize debug UART for PRINTF
#endif

    PRINTF("Hardware Initialized.\r\n");

    /* Initialize Application Specific Peripherals */
    // Initialize UART1 for ESP32 communication (must be done before tasks using it are created/run)
    UART_Init();

    // Initialize Actuator peripherals (e.g., PWM for LED/Buzzer)
    // Even though the actuator task is a dummy, init might be needed for shared peripherals (like TPM)
    // Or if the dummy task eventually calls real driver functions.
    // Actuator_Init(); // Uncomment if Member 2 provides this

    /* Initialize RTOS Objects */
    // Create Queues, Semaphores, Mutexes defined in rtos_manager.c
    RTOS_Objects_Init();

    /* Create RTOS Tasks */
    // Create all tasks (dummy and real) defined in rtos_manager.c
    RTOS_Tasks_Create();

    PRINTF("Starting FreeRTOS Scheduler...\r\n");

    /* Start the scheduler */
    // This function will take control and start running the tasks.
    // Code below this line will typically not execute unless there's an error in scheduler start.
    vTaskStartScheduler();

    /* Should never reach here */
    PRINTF("Error: Scheduler failed to start.\r\n");
    for(;;); // Infinite loop indicates failure
    return 0;
}
/*-----------------------------------------------------------*/
