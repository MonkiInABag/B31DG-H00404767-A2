#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_timer.h"
#include "esp_log.h"
#include "esp_task_wdt.h"

#include "driver/gpio.h"

#include "pins.h"
#include "workkernel.h"
#include "tasks.h"
#include "monitor.h"

// Task scheduling parameters
#define FRAME_US 10000LL
#define HYPERFRAME_FRAMES 10
#define TASKS_GAURD_TIME_US 2700LL

// Task execution state
static int64_t T0_us = 0;
static int last_frame = -1;
static int frame_ran_s = -1;

// Forward declarations for task runners
uint32_t s_id = 0;
uint32_t a_id = 0;
uint32_t b_id = 0;
uint32_t agg_id = 0;
uint32_t c_id = 0;
uint32_t d_id = 0;

// Task runner functions that wrap task execution with monitor calls
static void Run_TaskA(void)
{
    beginTaskA(a_id++);
    task_A();
    endTaskA();
}

static void Run_TaskB(void)
{
    beginTaskB(b_id++);
    task_B();
    endTaskB();
}

static void Run_TaskAGG(void)
{
    beginTaskAGG(agg_id++);
    task_AGG();
    endTaskAGG();
}

static void Run_TaskC(void)
{
    beginTaskC(c_id++);
    task_C();
    endTaskC();
}

static void Run_TaskD(void)
{
    beginTaskD(d_id++);
    task_D();
    endTaskD();
}


void app_main(void)
{
    // Initialization
    pins_init();
    tasks_init();
    monitorInit();

    // Configure periodic and final reports
    monitorSetPeriodicReportEverySeconds(10);
    monitorSetFinalReportAfterSeconds(60);

    // Disable task watchdog
    esp_task_wdt_delete(NULL);

    // Wait for SYNC signal to align the start of the schedule
    while (!pins_sync_seen()) {
        // Wait for sync
    }

    // Record T0 at SYNC and start the schedule
    T0_us = pins_sync_T0_us();
    synch();

    while (1)
    {
        // Poll monitor for any reports to print
        monitorPollReports();

        // Check deadlines and set error LED if any deadline was missed
        if (!monitorAllDeadlinesMet())
        {
            gpio_set_level(PIN_ERROR_LED, 1);
        }

        // Determine current frame based on time elapsed since T0
        int64_t now_us = esp_timer_get_time();
        int64_t elapsed_us = now_us - T0_us;
        int frame = (int)((elapsed_us / FRAME_US) % HYPERFRAME_FRAMES);

        // Run tasks for the current frame if not already run
        if (frame != last_frame) {
            last_frame = frame;

            switch (frame) {
                case 0:
                    Run_TaskA();
                    Run_TaskB();
                    Run_TaskAGG();
                    break;

                case 1:
                    Run_TaskA();
                    Run_TaskC();
                    break;

                case 2:
                    Run_TaskA();
                    Run_TaskB();
                    Run_TaskAGG();
                    break;

                case 3:
                    Run_TaskA();
                    Run_TaskD();
                    break;

                case 4:
                    Run_TaskA();
                    Run_TaskB();
                    Run_TaskAGG();
                    break;

                case 5:
                    Run_TaskA();
                    Run_TaskC();
                    break;

                case 6:
                    Run_TaskA();
                    Run_TaskB();
                    Run_TaskAGG();
                    break;

                case 7:
                    Run_TaskA();
                    Run_TaskD();
                    break;

                case 8:
                    Run_TaskA();
                    Run_TaskB();
                    Run_TaskAGG();
                    break;

                case 9:
                    Run_TaskA();
                    break;

                default:
                    break;
            }   
        }
        // After running the frame's tasks, check if we can run the sporadic 
        if (frame_ran_s != frame) 
        {
            // Calculate time left in the current frame after task execution
            int64_t post_tasks_us = esp_timer_get_time();
            int64_t tasks_exec_us = post_tasks_us - T0_us;
            int64_t current_frameIDX = (tasks_exec_us / FRAME_US);
            int64_t next_frame_start_us = T0_us + (current_frameIDX + 1) * FRAME_US;
            int64_t time_left_us = next_frame_start_us - post_tasks_us;

            // If there is enough time left in the frame, check for sporadic pending and run Task S
            if (time_left_us >= TASKS_GAURD_TIME_US) 
            {
                // Check if there is a pending sporadic event and run Task S if so
                if(pins_take_sporadic_pending()) {
                    beginTaskS(s_id++);
                    task_S();
                    endTaskS();
                    frame_ran_s = frame; 
                }
            }
        }

    }
}