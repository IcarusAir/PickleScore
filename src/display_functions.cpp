/*
 * DISTRACTED DEVELOPMENT
 * PICKLESCORE
 *
 * display_functions.cpp - Helper functions that update the OLED display
 * 
 */

#include "display_functions.h"

// Show boot up screen
void displayBootScreen()
{
    display.drawBitmap(18, 3, distracted_dev_logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
    display.setTextColor(SSD1306_WHITE);
    display.display();
}

// Draw and display court menu

// clear - set whether or not the court redraw should clear all data
void displayDrawCourt(bool clear) 
{
    int buff = 0;
    
    // If invoked between sets and games, clear the top bar
    if (clear) {
        display.fillRect(1,1,128,64,SSD1306_BLACK); // Clear entire screen
    } else {
        display.fillRect(1,1,128,36,SSD1306_BLACK); // Clear just the top half of the screen (don't clear set bubbles)
    }   
    xQueueSendToBack(global_display_queue, (void *) &buff, 0);

    displayCourtDontUpdate();

    // Sets
    display.drawCircle(11,45,4,SSD1306_WHITE);
    display.drawCircle(11,57,4,SSD1306_WHITE);
    display.drawCircle(116,45,4,SSD1306_WHITE);
    display.drawCircle(116,57,4,SSD1306_WHITE);

    // Score
    display.fillRect(29,43,29,16,SSD1306_BLACK);
    display.fillRect(77,43,29,16,SSD1306_BLACK);
    display.fillRect(59,48,9,3,SSD1306_WHITE);
    display.setCursor(29,43);
    display.setTextSize(2);
    display.write('0');
    display.write('0');

    display.setCursor(77,43);
    display.write('0');
    display.write('0');
    
    //display.display();
    xQueueSendToBack(global_display_queue, (void *) &buff, 0);

}

void displayCourtDontUpdate() {
    // Draw Court Diagram
    display.drawRect(25, 5, 30, 16, SSD1306_WHITE);
    display.drawRect(25, 20, 30, 16, SSD1306_WHITE);
    display.drawRect(54, 5, 20, 31, SSD1306_WHITE);
    display.drawRect(73, 5, 30, 16, SSD1306_WHITE);
    display.drawRect(73, 20, 30, 16, SSD1306_WHITE);
}

/* displayInvertCourtSelect
 * Updates display with a selected highlight (inverted and outlined)

 * uint8_t court - A value of 0, 1, 2 or 3 indicating which court to dehighlight
 *   ************  0: pd_team1_right - The lower left court
 *   ************  1: pd_team1_left - The upper left court
 *   ************  2: pd_team2_right - The upper right court
 *   ************  3: pd_team2_left - The lower right court
 */
void displayInvertCourtSelect(uint8_t court) 
{
    int buff = 0;
    switch (court) {
        case 0: //pd_team1_right
            display.fillRect(25, 20, 30, 16, SSD1306_INVERSE);
            display.drawRect(24, 19, 32, 18, SSD1306_WHITE);
            break;

        case 1: //pd_team1_left
            display.fillRect(25, 5, 30, 16, SSD1306_INVERSE);
            display.drawRect(24, 4, 32, 18, SSD1306_WHITE);
            break;

        case 2: //pd_team2_right
            display.fillRect(73, 5, 30, 16, SSD1306_INVERSE);
            display.drawRect(72, 4, 32, 18, SSD1306_WHITE);
            break;

        case 3: //pd_team2_left
            display.fillRect(73, 20, 30, 16, SSD1306_INVERSE);
            display.drawRect(72, 19, 32, 18, SSD1306_WHITE);
            break;

        default:
            return;
    }

    //display.display();
    xQueueSendToBack(global_display_queue, (void *) &buff, 0);
}

/* displayInvertCourtDeselect
 * Updates display with a deselected highlight (basic outline)

 * uint8_t court - A value of 0, 1, 2 or 3 indicating which court to dehighlight
 *   ************  0: pd_team1_right - The lower left court
 *   ************  1: pd_team1_left - The upper left court
 *   ************  2: pd_team2_right - The upper right court
 *   ************  3: pd_team2_left - The lower right court
 */
void displayInvertCourtDeselect(uint8_t court) 
{
    int buff = 0;
    switch (court) {
        case 0: //pd_team1_right
            display.fillRect(25, 20, 30, 16, SSD1306_INVERSE);
            display.drawRect(24, 19, 32, 18, SSD1306_BLACK);

            // Fill in missing points
            display.drawPixel(25,19,SSD1306_WHITE);
            display.drawPixel(55,35,SSD1306_WHITE);
            display.drawPixel(54,19,SSD1306_WHITE);
            // display.drawPixel(50,19,SSD1306_WHITE);
            // display.drawPixel(52,19,SSD1306_WHITE);
            break;

        case 1: //pd_team1_left
            display.fillRect(25, 5, 30, 16, SSD1306_INVERSE);
            display.drawRect(24, 4, 32, 18, SSD1306_BLACK);

            // Fill in missing points
            display.drawPixel(25,21,SSD1306_WHITE);
            display.drawPixel(55,5,SSD1306_WHITE);
            display.drawPixel(54,21,SSD1306_WHITE);
            // display.drawPixel(48,21,SSD1306_WHITE);
            // display.drawPixel(50,21,SSD1306_WHITE);
            // display.drawPixel(52,21,SSD1306_WHITE);
            // display.drawPixel(55,14,SSD1306_WHITE);
            // display.drawPixel(55,16,SSD1306_WHITE);
            // display.drawPixel(55,18,SSD1306_WHITE);
            break;

        case 2: //pd_team2_right
            display.fillRect(73, 5, 30, 16, SSD1306_INVERSE);
            display.drawRect(72, 4, 32, 18, SSD1306_BLACK);

            // Fill in missing points
            display.drawPixel(73,21,SSD1306_WHITE);
            display.drawPixel(72,5,SSD1306_WHITE);
            display.drawPixel(102,21,SSD1306_WHITE);
            // display.drawPixel(100,21,SSD1306_WHITE);
            break;

        case 3: //pd_team2_left
            display.fillRect(73, 20, 30, 16, SSD1306_INVERSE);
            display.drawRect(72, 19, 32, 18, SSD1306_BLACK);

            // Fill in missing points
            display.drawPixel(73,19,SSD1306_WHITE);
            display.drawPixel(72,35,SSD1306_WHITE);
            display.drawPixel(102,19,SSD1306_WHITE);
            break;

        default:
            return;
    }

    //display.display();
    xQueueSendToBack(global_display_queue, (void *) &buff, 0);
}

void displayIncrementScore(uint8_t team) 
{
    // Increase the score of the serving team
    char tens;
    char ones;
    int buff = 0;

    if (team == 1) {
        if (team1_score < 11) {
            team1_score++;
            // Clear old score
            display.fillRect(29,43,29,16,SSD1306_BLACK);

            // Write new score
            display.setCursor(29,43);
            display.setTextSize(2);
            tens = (char) team1_score/10 + 48;
            ones = (char) team1_score%10 + 48;
            display.write(tens);
            display.write(ones);
        }
    } else if (team == 2) {
        if (team2_score < 11) {
            team2_score++;
            // Clear old score
            display.fillRect(77,43,29,16,SSD1306_BLACK);

            // Write new score
            display.setCursor(77,43);
            display.setTextSize(2);
            tens = (char) team2_score/10 + 48;
            ones = (char) team2_score%10 + 48;
            display.write(tens);
            display.write(ones);
        }
    } else {
        return;
    }

    //display.display();
    xQueueSendToBack(global_display_queue, (void *) &buff, 0);
}

/* displayLosePossession
 * Called when the currently receiving team scores, updates position of the 1 or 2.
 * 
 */
void displayLosePossession(uint8_t team) 
{
    int buff = 0;

    if (team == 1) 
    {
        // The current server is team 1, they just lost a point
        if ((court_data & 0x08) == 0x00) {
            // Only lost possession once, change from 1 to 2
            // Clear old number
            display.fillRect(9,16,5,7,SSD1306_BLACK);
            // Draw new number
            display.setCursor(9,16);
            display.setTextSize(1);
            display.write('2');
        } else {
            // Second possession lost, put 1 on other team side
            // Clear old number
            display.fillRect(9,16,5,7,SSD1306_BLACK);
            // Draw new Number
            display.setCursor(114,16);
            display.setTextSize(1);
            display.write('1');
        }
    } 
    else if (team == 2) 
    {
        // The current server is team 2, they just lost a point
        if ((court_data & 0x04) == 0x00) {
            // Only lost possession once, change from 1 to 2
            // Clear old number
            display.fillRect(114,16,5,7,SSD1306_BLACK);
            // Draw new number
            display.setCursor(114,16);
            display.setTextSize(1);
            display.write('2');
        } else {
            // Second possession lost, put 1 on other team side
            // Clear old number
            display.fillRect(114,16,5,7,SSD1306_BLACK);
            // Draw new Number
            display.setCursor(9,16);
            display.setTextSize(1);
            display.write('1');
        }
    } else {
        return;
    }
    //display.display();
    xQueueSendToBack(global_display_queue, (void*) &buff, 0);
}

/* displayEndSet
 * Called when the currently receiving team scores 11 (for pickleball), winning a set but not a game
 * 
 */
void displayEndSet(uint8_t team)
{
    int buff = 0;

    if (team == 1) {
        // team 1 just won their first set
        // Fill in Set Bubble
        display.fillCircle(11,45,2,SSD1306_WHITE);

        // Clear Court and display win message
        display.fillRect(1,1,128,36,SSD1306_BLACK);
        display.setCursor(11,3);
        display.setTextSize(1);
        display.print("Team 1 set win");
    } 
    else if (team == 2) {
        // team 2 just won their first set
        // Fill in Set Bubble
        display.fillCircle(116,45,2,SSD1306_WHITE);

        // Clear Court and display win message
        display.fillRect(1,1,128,36,SSD1306_BLACK);
        display.setCursor(11,3);
        display.setTextSize(1);
        display.print("Team 2 set win");
    }

    xQueueSendToBack(global_display_queue, (void*) &buff, 0);
}

/* displayEndGame
 * Called when the currently receiving team wins a game, fill second set bubble and display
 * 
 */
void displayEndGame(uint8_t team)
{
    int buff = 0;

    if (team == 1) {
        // team 1 just won their first set
        // Fill in Set Bubble
        display.fillCircle(11,57,2,SSD1306_WHITE);

        // Clear Court and display win message
        display.fillRect(1,1,128,36,SSD1306_BLACK);
        display.setCursor(11,3);
        display.setTextSize(1);
        display.print("Team 1 game win");
    } 
    else if (team == 2) {
        // team 2 just won their first set
        // Fill in Set Bubble
        display.fillCircle(116,57,2,SSD1306_WHITE);

        // Clear Court and display win message
        display.fillRect(1,1,128,36,SSD1306_BLACK);
        display.setCursor(11,3);
        display.setTextSize(1);
        display.print("Team 2 game win");
    }

    xQueueSendToBack(global_display_queue, (void*) &buff, 0);
}

