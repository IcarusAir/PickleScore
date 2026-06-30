/*
 * DISTRACTED DEVELOPMENT
 * PICKLESCORE
 *
 * system_globals.h - global system state memory and includes
 * 
 */

#ifndef SYSTEM_GLOBALS_H

#define SYSTEM_GLOBALS_H

#include <Arduino.h>
#include <stdio.h>
#include <Wire.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include <Adafruit_GFX.h>
// Switch Comments to toggle emulator
#include <Adafruit_SSD1306.h>
//#include <Adafruit_SSD1306_EMULATOR.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_RST -1
#define SCREEN_ADDR 0x3D //0x3D for 128x64, 0x3C for 128x32

#define BUTTON1_PIN GPIO_NUM_0
#define BUTTON2_PIN GPIO_NUM_1

#define SDA_PIN GPIO_NUM_3
#define SCL_PIN GPIO_NUM_10

#define BUTTON1_BIT (1 << 0)
#define BUTTON2_BIT (1 << 1)
#define DEBOUNCE_DELAY 30 // in ms

#define DEBUG_MODE false

typedef struct {
    EventGroupHandle_t button_event_group;
    TimerHandle_t boot_timer;
} button_task_params_t;

enum state {
    startup,
    /* Pickleball Doubles*/
    pd_team1_right,
    pd_team1_left,
    pd_team2_right,
    pd_team2_left,
    pd_between_sets,
    pd_between_games,
    /* Badminton Singles*/
    bs_team1,
    bs_team2,
    bs_between_games,
    bs_end_of_games
};

extern Adafruit_SSD1306 display;
extern enum state system_state;
extern bool boot_flash_flag;

extern uint8_t court_data;

extern uint8_t team1_score;
extern uint8_t team2_score;

extern QueueHandle_t global_display_queue;

#endif