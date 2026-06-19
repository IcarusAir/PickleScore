/*
 * DISTRACTED DEVELOPMENT
 * PICKLESCORE
 *
 * display_functions.h - Helper functions that update the OLED display
 * 
 */

#ifndef DISPLAY_FUNCTIONS_H

#define DISPLAY_FUNCTIONS_H

#include <stdint.h>
#include "system_globals.h"
#include "devlogo.h"

void displayBootScreen();
void displayCourtInit();
void displayInvertCourtSelect(uint8_t court);
void displayInvertCourtDeselect(uint8_t court);
void displayIncrementScore(uint8_t team);
void displayLosePossession(uint8_t team);


#endif