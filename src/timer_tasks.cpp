/*
 * DISTRACTED DEVELOPMENT
 * PICKLESCORE
 *
 * timer_tasks.cpp - FreeRTOS tasks that are triggered with software timers
 * 
 */

#include "timer_tasks.h"


// Flash Boot Screen Task
// Just a nice visual element to show that the device is waiting on the user, is a timer task
// One boot starts, the Timer is stopped
void flash_boot_screen_task(TimerHandle_t timer)
{
    // Another layer to ensure that this doesn't happen unless on startup
    if (system_state == startup) {
        if (boot_flash_flag) 
        {
            display.fillRect(18, 52, 100, 7, SSD1306_BLACK);
            boot_flash_flag = false;
        }
        else
        {
            display.setCursor(18, 52);
            display.print("PRESS A BUTTON..");
            boot_flash_flag = true;
        }
        int buff = 0;
        xQueueSendToBack(global_display_queue, &buff, 0);
    }
}

// The highlight court task is responsible for flashing an indicator on the display to indicate the current server.
void highlight_court_task(TimerHandle_t timer)
{   
    // Only blink the court light when waiting for service
    if (system_state == pd_team1_right || system_state == pd_team1_left || system_state == pd_team2_right || system_state == pd_team2_left) 
    {   

        if ( (int) pvTimerGetTimerID(timer) == 0) 
        {
            // Based on the court state, highlight the court
            switch (system_state) {
                case pd_team1_right:
                    displayInvertCourtSelect(0);
                    court_data |= 0x80; //Set bit 7
                    break;
                case pd_team1_left:
                    displayInvertCourtSelect(1);
                    court_data |= 0x40; //Set bit 6
                    break;
                case pd_team2_right:
                    displayInvertCourtSelect(2);
                    court_data |= 0x20; //Set bit 5
                    break;
                case pd_team2_left:
                    displayInvertCourtSelect(3);
                    court_data |= 0x10; //Set bit 4
                    break;
            }
        } 
        else if ( (int) pvTimerGetTimerID(timer) == 1) 
        {   
            // Read court data, if the current light on doesn't match system state, turn it off
            switch (court_data & 0xF0) { 
                case 0x80: // LL court highlighted
                    displayInvertCourtDeselect(0);
                    court_data &= 0x7F; //Clear bit 7
                    break;
                case 0x40: // UL court highlighed
                    displayInvertCourtDeselect(1);
                    court_data &= 0xBF; //Clear bit 6
                    break;
                case 0x20: // UR court highlighed
                    displayInvertCourtDeselect(2);
                    court_data &= 0xDF; //Clear bit 5
                    break;
                case 0x10: // LR court highlighed
                    displayInvertCourtDeselect(3);
                    court_data &= 0xEF; //Clear bit 4
                    break;
                case 0x00: // Nothing is highlighted
                    return;
                default:
                    // Big problem, two or more lights on at once.
                    Serial.println("Problem with two or more lights at once!"); 
                    break;
            }

        }
        
    }
}