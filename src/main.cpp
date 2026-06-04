#include <Arduino.h>
#include <stdio.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
// Switch Comments to toggle emulator
//#include <Adafruit_SSD1306.h>
#include <Adafruit_SSD1306_EMULATOR.h>

#include "driver/gpio.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_RST -1
#define SCREEN_ADDR 0x3D //0x3D for 128x64, 0x3C for 128x32

#define BUTTON1_PIN 0
#define BUTTON2_PIN 1
#define BUTTON3_PIN 10

#define DEBUG_MODE false

// Variable declarations
int result;

int b1_val;
int b2_val;
int b3_val;

// Adafruit Display variables - Switch Comments to toggle emulator
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, SCREEN_RST);
Adafruit_SSD1306_EMULATOR display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, SCREEN_RST);

enum state {
    startup = 0,
    p_team1_right = 1,
    p_team1_left = 2,
    p_team2_right = 3,
    p_team2_left = 4
};

enum state system_state = startup;


// put function declarations here:
void displayInit();
void displayInvertCourtSelect();
void displayInvertCourtDeselect();
void printDebug();


void setup() {
    // Initialize Serial
    Serial.begin(9600);
    delay(500);
    
    // Initialize Button Pins
    gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_NUM_1, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_NUM_10, GPIO_MODE_INPUT);

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDR)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }


    display.display();
    delay(2000);


    display.drawCircle(64,32,5,SSD1306_WHITE);
    display.display();

    display.clearDisplay();
    displayInit();

}

void loop() {

    // Debug Mode Check
    if (DEBUG_MODE){printDebug();} 
    else {
        // Pseudocode time
        // We are in the main run, assume all setup is done and we are waiting for button presses, score is 0-0-2
        system_state = p_team1_right;
        displayInvertCourtSelect();
        delay(500);
        displayInvertCourtDeselect();
        delay(350);

        system_state = p_team1_left;
        displayInvertCourtSelect();
        delay(500);
        displayInvertCourtDeselect();
        delay(350);

        system_state = p_team2_right;
        displayInvertCourtSelect();
        delay(500);
        displayInvertCourtDeselect();
        delay(350);

        system_state = p_team2_left;
        displayInvertCourtSelect();
        delay(500);
        displayInvertCourtDeselect();
        delay(350);
        
    }
}

// Show boot up screen
void displayInit() {

    // Draw Court Diagram
    display.drawRect(25, 5, 30, 16, SSD1306_WHITE);
    display.drawRect(25, 20, 30, 16, SSD1306_WHITE);
    display.drawRect(54, 5, 20, 31, SSD1306_WHITE);
    display.drawRect(73, 5, 30, 16, SSD1306_WHITE);
    display.drawRect(73, 20, 30, 16, SSD1306_WHITE);

    // Highlights
    display.drawLine(34,35,64,5,SSD1306_WHITE);
    display.drawLine(36,35,66,5,SSD1306_WHITE);
    display.drawLine(38,35,68,5,SSD1306_WHITE);

    display.drawLine(86,35,102,19,SSD1306_WHITE);
    display.drawLine(88,35,102,21,SSD1306_WHITE);
    display.drawLine(90,35,102,23,SSD1306_WHITE);

    // Sets
    display.drawCircle(11,45,4,SSD1306_WHITE);
    display.drawCircle(11,57,4,SSD1306_WHITE);
    display.drawCircle(116,45,4,SSD1306_WHITE);
    display.drawCircle(116,57,4,SSD1306_WHITE);

    // Score
    display.fillRect(59,48,9,3,SSD1306_WHITE);
    display.setCursor(26,43);
    display.setTextSize(3);
    

    display.display();

}

void displayInvertCourtSelect() {
    
    if (system_state == p_team1_right) {
        display.fillRect(25, 20, 30, 16, SSD1306_INVERSE);
        display.drawRect(24, 19, 32, 18, SSD1306_WHITE);
    } else if (system_state == p_team1_left) {
        display.fillRect(25, 5, 30, 16, SSD1306_INVERSE);
        display.drawRect(24, 4, 32, 18, SSD1306_WHITE);
    } else if (system_state == p_team2_right) {
        display.fillRect(73, 5, 30, 16, SSD1306_INVERSE);
        display.drawRect(72, 4, 32, 18, SSD1306_WHITE);
    } else if (system_state == p_team2_left) {
        display.fillRect(73, 20, 30, 16, SSD1306_INVERSE);
        display.drawRect(72, 19, 32, 18, SSD1306_WHITE);
    } else {
        return;
    }

    display.display();
}

void displayInvertCourtDeselect() {

    if (system_state == p_team1_right) {
        display.fillRect(25, 20, 30, 16, SSD1306_INVERSE);
        display.drawRect(24, 19, 32, 18, SSD1306_BLACK);

        // Fill in missing points
        display.drawPixel(25,19,SSD1306_WHITE);
        display.drawPixel(55,35,SSD1306_WHITE);
        display.drawPixel(54,19,SSD1306_WHITE);
        display.drawPixel(50,19,SSD1306_WHITE);
        display.drawPixel(52,19,SSD1306_WHITE);

    } else if (system_state == p_team1_left) {
        display.fillRect(25, 5, 30, 16, SSD1306_INVERSE);
        display.drawRect(24, 4, 32, 18, SSD1306_BLACK);

        // Fill in missing points
        display.drawPixel(25,21,SSD1306_WHITE);
        display.drawPixel(55,5,SSD1306_WHITE);
        display.drawPixel(54,21,SSD1306_WHITE);
        display.drawPixel(48,21,SSD1306_WHITE);
        display.drawPixel(50,21,SSD1306_WHITE);
        display.drawPixel(52,21,SSD1306_WHITE);
        display.drawPixel(55,14,SSD1306_WHITE);
        display.drawPixel(55,16,SSD1306_WHITE);
        display.drawPixel(55,18,SSD1306_WHITE);

    } else if (system_state == p_team2_right) {
        display.fillRect(73, 5, 30, 16, SSD1306_INVERSE);
        display.drawRect(72, 4, 32, 18, SSD1306_BLACK);

        // Fill in missing points
        display.drawPixel(73,21,SSD1306_WHITE);
        display.drawPixel(72,5,SSD1306_WHITE);
        display.drawPixel(102,21,SSD1306_WHITE);
        display.drawPixel(100,21,SSD1306_WHITE);

    } else if (system_state == p_team2_left) {
        display.fillRect(73, 20, 30, 16, SSD1306_INVERSE);
        display.drawRect(72, 19, 32, 18, SSD1306_BLACK);

        // Fill in missing points
        display.drawPixel(73,19,SSD1306_WHITE);
        display.drawPixel(72,35,SSD1306_WHITE);
        display.drawPixel(102,19,SSD1306_WHITE);

    } else {
        return;
    }

    display.display();
}

void printDebug() {
    // If DEBUG mode is on, display button inputs
    Serial.println("\n============================================");
    Serial.println("              D E B U G");
    Serial.printf("Button 1 Value: %d\n", gpio_get_level(GPIO_NUM_0));
    Serial.printf("Button 2 Value: %d\n", gpio_get_level(GPIO_NUM_1));
    Serial.printf("Button 3 Value: %d\n", gpio_get_level(GPIO_NUM_10));
    Serial.println("============================================");

}