/*
 * DISTRACTED DEVELOPMENT
 * PICKLESCORE
 *
 * system_globals.cpp - global system state memory and includes
 * 
 */

#include "system_globals.h"

// Adafruit Display variables - Switch Comments to toggle emulator
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, SCREEN_RST);
//Adafruit_SSD1306_EMULATOR display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, SCREEN_RST);

enum state system_state = startup;
bool boot_flash_flag = false;

// An 8-bit set of flags showing which courts are highlighted and current scoring flags
// Bits 7:4 - court light data (LL - 7, UL - 6, UR - 5, LR - 4)
// Bit 3 - Team 1 server number - 0: server 1, 1: server 2
// Bit 2 - Team 2 server number - 0: server 1, 1: server 2
// Bit 1 - Team 1 set 1 taken flag
// Bit 0 - Team 2 set 1 taken flag 
uint8_t court_data = 0x0;

uint8_t team1_score = 0;
uint8_t team2_score = 0;

uint8_t winning_points = 11;

QueueHandle_t global_display_queue; // A queue makes more sense for a display (multiple display requests can happen)