/*
 * FreeRTOS Kernel V10.4.3
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See https://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

/* Ensure stdint is only used by the compiler, and not the assembler. */
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
    #include <stdint.h>
    extern uint32_t SystemCoreClock;
#endif

// -- REQUIRED FOR ASSIGNMENT --
#define configUSE_PREEMPTION                    1 // Enable pre-emptive scheduler
#define configUSE_TIME_SLICING                  1 // Enable time slicing for equal priority tasks
// -----------------------------

#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configCPU_CLOCK_HZ                      ( SystemCoreClock )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 ) // 1ms tick rate
#define configMAX_PRIORITIES                    ( 5 ) // Max priorities for tasks
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 90 ) // Minimum stack size in words
#define configMAX_TASK_NAME_LEN                 ( 16 ) // Max length of task names
#define configUSE_16_BIT_TICKS                  0 // Use 32-bit ticks

// --- Memory Management ---
#define configSUPPORT_DYNAMIC_ALLOCATION        1 // Enable dynamic memory allocation (e.g., for queues, semaphores)
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 10 * 1024 ) ) // 10KB heap size, adjust if needed
#define configAPPLICATION_ALLOCATED_HEAP        0 // Let FreeRTOS manage the heap
#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP 0

// --- Hooks ---
#define configUSE_IDLE_HOOK                     0 // Disable idle hook
#define configUSE_TICK_HOOK                     0 // Disable tick hook
#define configCHECK_FOR_STACK_OVERFLOW          0 // Disable stack overflow checking (can be enabled for debugging)
#define configUSE_MALLOC_FAILED_HOOK            0 // Disable malloc failed hook
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

// --- Run time and task stats gathering ---
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                1 // Enable trace facility for debugging
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

// --- Co-routine definitions ---
#define configUSE_CO_ROUTINES                   0 // Disable co-routines
#define configMAX_CO_ROUTINE_PRIORITIES         ( 2 )

// --- Software timer definitions ---
#define configUSE_TIMERS                        1 // Enable software timers
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 ) // High priority for timer task
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )

// --- Include Definitions ---
// Enable specific FreeRTOS API functions
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskCleanUpResources           0 // Disabled, check if needed
#define INCLUDE_vTaskSuspend                    1 // Required for delays and blocking
#define INCLUDE_vTaskDelayUntil                 1 // Required for periodic tasks
#define INCLUDE_vTaskDelay                      1 // Required for delays
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     0 // Disable stack high water mark checking
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xEventGroupSetBitFromISR        1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 1
#define INCLUDE_xTaskGetHandle                  1
#define INCLUDE_xTaskResumeFromISR              1

// --- Concurrency and Synchronization ---
#define configUSE_MUTEXES                       1 // Enable Mutexes (needed for UART protection)
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1 // Enable Counting Semaphores
#define configUSE_QUEUE_SETS                    1 // Enable Queue Sets (implicitly enables Queues)

// --- Interrupt nesting behavior configuration ---
// Set according to your processor and application requirements
// For ARM Cortex-M0+, typically these priorities need careful configuration.
// Check FreeRTOS documentation for Kinetis M0+ ports.
// Lower numeric value means higher logical priority (hardware perspective)
// FreeRTOS requires priorities >= configMAX_SYSCALL_INTERRUPT_PRIORITY to be masked
#define configKERNEL_INTERRUPT_PRIORITY         ( (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS)) & 0xFF )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS)) & 0xFF )
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 3 // Example, adjust based on NVIC priority bits
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 1 // Example, adjust based on NVIC priority bits

/* Number of priority bits implemented by the microcontroller */
#define configPRIO_BITS                         2 // For MCXC444 (4 priority levels: 0, 64, 128, 192)

// --- Assertions ---
#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

// --- FreeRTOS MPU specific definitions ---
#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS 0
#define configTOTAL_MPU_REGIONS                                8 /* Default value. */
#define configTEX_S_C_B_FLASH                                  0x07UL /* Default value. */
#define configTEX_S_C_B_SRAM                                   0x07UL /* Default value. */
#define configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY            1

// --- Optional functions ---
#define configENABLE_BACKWARD_COMPATIBILITY     0

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names. */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
