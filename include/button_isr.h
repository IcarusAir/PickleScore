/*
 * DISTRACTED DEVELOPMENT
 * PICKLESCORE
 *
 * button_isr.h - GPIO Interrupt Service Routines for push buttons
 * 
 */

#ifndef BUTTON_ISR_H

#define BUTTON_ISR_H

// Information on interrupts and GPIO: 
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/api-reference/peripherals/gpio.html#_CPPv411gpio_configPK13gpio_config_t
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_intr_alloc.h"
#include "system_globals.h"

void button_pin0_isr(void* arg);
void button_pin1_isr(void* arg);

#endif