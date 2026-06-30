/*
 * DISTRACTED DEVELOPMENT
 * PICKLESCORE
 *
 * tasks.cpp - FreeRTOS tasks that are triggered asynchronously
 * 
 */

#include "tasks.h"


// Display Task
// Interrupting the current display.display() will crash the emulator and cause graphical glitches in the screen
// This is a higher priority task that ensures that a screen update doesn't interrupt another screen update
void display_task(void* pvParameters)
{
    QueueHandle_t display_queue = (QueueHandle_t)pvParameters;
    BaseType_t status;
    int buff;

    while(1)
    {
        // Receive from Queue
        status = xQueueReceive(display_queue, &buff, portMAX_DELAY);
        if (status != pdPASS)
        {
            Serial.println("Issue Receiving from Queue!");
        }
        display.display();
    }

}

// The job of the button handle task is to listen for a button to be pressed and handle the result of the press based on the system state
void button_handle_task(void* pvParameters) 
{
    button_task_params_t *params = (button_task_params_t*) pvParameters;
    EventGroupHandle_t button_event_group = params->button_event_group;
    TimerHandle_t boot_timer = params->boot_timer;
    EventBits_t bits;

    while(1) 
    {
        bits = xEventGroupWaitBits(button_event_group,
            BUTTON1_BIT | BUTTON2_BIT,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY);

        if ((bits & BUTTON1_BIT) == BUTTON1_BIT) 
        {
            switch (system_state) {
                case startup: // On startup - Any button press starts the game
                    system_state = pd_team1_right;
                    display.clearDisplay();
                    displayDrawCourt(true);
                    xTimerStop(boot_timer, 0);
                    displayLosePossession(1);
                    court_data |= 0x08; // Set bit 3, server starts on 2
                    break;

                case pd_team1_right: // Increase team 1's score, they have posession, players switch places
                    system_state = pd_team1_left;
                    displayIncrementScore(1);
                    if (team1_score >= 11) { // Detect set win
                        if ((court_data & 0x02) == 0x02) { // Detect game win
                            system_state = pd_between_games;
                            displayEndGame(1);
                            
                        } else {
                            system_state = pd_between_sets;
                            court_data |= 0x02; // Set 'set' bit for team 1
                            displayEndSet(1);
                        }
                    }
                    break;
                
                case pd_team1_left: // Increase team 1's score, they have posession, players switch places
                    system_state = pd_team1_right;
                    displayIncrementScore(1);
                    if (team1_score >= 11) { // Detect set win
                        if ((court_data & 0x02) == 0x02) { // Detect game win
                            system_state = pd_between_games;
                            displayEndGame(1);
                            
                        } else {
                            system_state = pd_between_sets;
                            court_data |= 0x02; // Set 'set' bit for team 1
                            displayEndSet(1);
                        }
                    }
                    break;

                case pd_team2_right: // Team 2 loses a serve, switch to other server or to other team
                    if ((court_data & 0x04) == 0x00) {
                        displayLosePossession(2);
                        court_data |= 0x04; // Set bit, team 2 is on server 2
                        system_state = pd_team2_left;
                    } else {
                        displayLosePossession(2);
                        court_data &= 0xFB; // clear bit, team 2 loses possession
                        system_state = pd_team1_right;
                    }
                    break;

                case pd_team2_left:// Team 2 loses a serve, switch to other server or to other team
                    if ((court_data & 0x04) == 0x00) {
                        displayLosePossession(2);
                        court_data |= 0x04; // Set bit, team 2 is on server 2
                        system_state = pd_team2_right;
                    } else {
                        displayLosePossession(2);
                        court_data &= 0xFB; // clear bit, team 2 loses possession
                        system_state = pd_team1_right;
                    }
                    break;

                case pd_between_sets:
                    // Reset everything except set data for next set
                    team1_score = 0;
                    team2_score = 0;
                    displayDrawCourt(false);
                    system_state = pd_team1_right;
                    court_data &= 0x03;
                    // Make sure first serve starts on 2
                    displayLosePossession(1);
                    court_data |= 0x08; // Set bit 3, server starts on 2
                    break;
                
                case pd_between_games:
                    // Reset everything for next game
                    team1_score = 0;
                    team2_score = 0;
                    displayDrawCourt(true);
                    system_state = pd_team1_right;
                    court_data = 0x00;
                    // Make sure first serve starts on 2
                    displayLosePossession(1);
                    court_data |= 0x08;
                    break;
                
                default:
                    break;
            }
        }
        if ((bits & BUTTON2_BIT) == BUTTON2_BIT) 
        {
            switch (system_state) {
                case startup: // On startup - Any button press starts the game
                    system_state = pd_team1_right;
                    display.clearDisplay();
                    displayDrawCourt(true);
                    xTimerStop(boot_timer, 0);
                    displayLosePossession(1);
                    court_data |= 0x08; // Set bit 3, server starts on 2
                    break;

                case pd_team1_right: // Team 1 loses a serve, switch to other server or to other team
                    if ((court_data & 0x08) == 0x00) {
                        displayLosePossession(1);
                        court_data |= 0x08; // Set bit, team 1 is on server 2
                        system_state = pd_team1_left;
                    } else {
                        displayLosePossession(1);
                        court_data &= 0xF7; // clear bit, team 1 loses possession
                        system_state = pd_team2_right;
                    }
                    break;
                
                case pd_team1_left: // Team 1 loses a serve, switch to other server or to other team
                    if ((court_data & 0x08) == 0x00) {
                        displayLosePossession(1);
                        court_data |= 0x08; // Set bit, team 1 is on server 2
                        system_state = pd_team1_right;
                    } else {
                        displayLosePossession(1);
                        court_data &= 0xF7; // clear bit, team 1 loses possession
                        system_state = pd_team2_right;
                    }
                    break;

                case pd_team2_right: // Increase team 2's score, they have posession, players switch places
                    system_state = pd_team2_left;
                    displayIncrementScore(2);
                    if (team2_score >= 11) { // Detect set win
                        if ((court_data & 0x01) == 0x01) { // Detect game win
                            system_state = pd_between_games;
                            displayEndGame(2);
                            
                        } else {
                            system_state = pd_between_sets;
                            court_data |= 0x01; // Set 'set' bit for team 2
                            displayEndSet(2);
                        }
                    }
                    break;

                case pd_team2_left: // Increase team 2's score, they have posession, players switch places
                    system_state = pd_team2_right;
                    displayIncrementScore(2);
                    if (team2_score >= 11) { // Detect set win
                        if ((court_data & 0x01) == 0x01) { // Detect game win
                            system_state = pd_between_games;
                            displayEndGame(2);
                            
                        } else {
                            system_state = pd_between_sets;
                            court_data |= 0x01; // Set 'set' bit for team 2
                            displayEndSet(2);
                        }
                    }
                    break;

                case pd_between_sets:
                    // Reset everything for next game
                    team1_score = 0;
                    team2_score = 0;
                    displayDrawCourt(false);
                    system_state = pd_team1_right;
                    court_data &= 0x03;
                    // Make sure first serve starts on 2
                    displayLosePossession(1);
                    court_data |= 0x08; // Set bit 3, server starts on 2
                    break;

                case pd_between_games:
                    // Reset everything for next game
                    team1_score = 0;
                    team2_score = 0;
                    displayDrawCourt(true);
                    system_state = pd_team1_right;
                    court_data = 0x00;
                    // Make sure first serve starts on 2
                    displayLosePossession(1);
                    court_data |= 0x08;
                    break;
                
                default:
                    break;
            }
        }        
    }

}



void end_game_task(void* pvParameters) 
{

}

