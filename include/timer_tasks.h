/*
 * DISTRACTED DEVELOPMENT
 * PICKLESCORE
 *
 * timer_tasks.h - FreeRTOS tasks that are triggered with software timers
 * 
 */

#ifndef TIMER_TASKS_H

#define TIMER_TASKS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "display_functions.h"

void flash_boot_screen_task(TimerHandle_t timer);
void highlight_court_task(TimerHandle_t timer);

#endif