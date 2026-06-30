/*
 * DISTRACTED DEVELOPMENT
 * PICKLESCORE
 *
 * tasks.h - FreeRTOS tasks that are triggered asynchronously
 * 
 */

#ifndef TASKS_H

#define TASKS_H

#include "system_globals.h"
#include "display_functions.h"


void display_task(void* pvParameters);
void button_handle_task(void* pvParameters);

#endif