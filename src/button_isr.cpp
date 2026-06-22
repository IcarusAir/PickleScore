/*
 * DISTRACTED DEVELOPMENT
 * PICKLESCORE
 *
 * button_isr.cpp - GPIO Interrupt Service Routines for push buttons
 * 
 */

#include "button_isr.h"

// Debouncing Variables
uint8_t button_1_state = 0;
uint8_t last_button_1_state = 0;
unsigned long last_debounce_time_b1 = 0;

uint8_t button_2_state = 0;
uint8_t last_button_2_state = 0;
unsigned long last_debounce_time_b2 = 0;

// ISR for GPIO Pin 0 - connected to button that scores for team 1
void button_pin0_isr(void* arg) 
{
    EventGroupHandle_t event_group = (EventGroupHandle_t) arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result;

    button_1_state = gpio_get_level(BUTTON1_PIN);
    /* Encapsulate with debounce logic */
    if (millis() - last_debounce_time_b1 > DEBOUNCE_DELAY)
    {
        // Only pass if this is a meaningful change of state
        if (button_1_state != last_button_1_state) // Eats inputs, but also prevents release checks
        {
            last_button_1_state = button_1_state;
            // Only pass on logic HIGH
            if (button_1_state == 1)
            {
                // ISR Routine
                result = xEventGroupSetBitsFromISR(event_group, BUTTON1_BIT, &xHigherPriorityTaskWoken);
                if (result != pdFAIL)
                {
                    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
                }
            }
        } 
        else if (last_button_1_state == 1)
        {
            // Goes here when a button press ends on a 1, make current button state a 0
            //Serial.println('X');
            last_button_1_state = 0;
        }
        last_debounce_time_b1 = millis();
    }

}

void button_pin1_isr(void* arg) 
{
    EventGroupHandle_t event_group = (EventGroupHandle_t) arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result;

    button_2_state = gpio_get_level(BUTTON2_PIN);
    /* Encapsulate with debounce logic */
    if (millis() - last_debounce_time_b2 > DEBOUNCE_DELAY)
    {
        // Only pass if this is a meaningful change of state
        if (button_2_state != last_button_2_state)
        {
            last_button_2_state = button_2_state;
            // Only pass on logic HIGH
            if (button_2_state == 1)
            {
                // ISR Routine
                result = xEventGroupSetBitsFromISR(event_group, BUTTON2_BIT, &xHigherPriorityTaskWoken);
                if (result != pdFAIL)
                {
                    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
                }
            }
        }
        else if (last_button_2_state == 1)
        {
            // Goes here when a button press ends on a HI, make current button state a LO
            //Serial.println('X');
            last_button_2_state = 0;
        }
        last_debounce_time_b2 = millis();
    }
}