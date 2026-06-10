#include <Arduino.h>
#include <stdio.h>
#include <Wire.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include <Adafruit_GFX.h>
// Switch Comments to toggle emulator
//#include <Adafruit_SSD1306.h>
#include <Adafruit_SSD1306_EMULATOR.h>

// Information on interrupts and GPIO: 
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/api-reference/peripherals/gpio.html#_CPPv411gpio_configPK13gpio_config_t
#include "driver/gpio.h"
#include "esp_intr_alloc.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_RST -1
#define SCREEN_ADDR 0x3D //0x3D for 128x64, 0x3C for 128x32

#define BUTTON1_PIN 0
#define BUTTON2_PIN 1
#define BUTTON3_PIN 10

#define BUTTON1_BIT (1 << 0)
#define BUTTON2_BIT (1 << 1)
#define BUTTON3_BIT (1 << 2)

#define DEBUG_MODE false

// ============================= GLOBAL DEFINITIONS ====================================

// Adafruit Display variables - Switch Comments to toggle emulator
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, SCREEN_RST);
Adafruit_SSD1306_EMULATOR display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, SCREEN_RST);

enum state {
    startup,
    /* Pickleball Doubles*/
    pd_team1_right,
    pd_team1_left,
    pd_team2_right,
    pd_team2_left,
    pd_between_games,
    pd_end_of_games,
    /* Badminton Singles*/
    bs_team1,
    bs_team2,
    bs_between_games,
    bs_end_of_games
};

enum state system_state = startup;

// An 8-bit set of flags showing which courts are highlighted and current scoring flags
// Bits 7:4 - court light data (UL - 7, LL - 6, UR - 5, LR - 4)
// Bits 1:0 - Scoring flags (team 1 - 1, team 2 - 0)
uint8_t court_data = 0x0;

int team1_score = 0;
int team2_score = 0;

EventGroupHandle_t global_event_group_handler;

// ============================= FUNCTION DEFINITIONS ====================================
void displayInit();
void displayInvertCourtSelect();
void displayInvertCourtDeselect();
void incrementScore();
void printDebug();
void gpioSetup();
void isrSetup();

void button_pin0_isr(void* arg);
void button_pin1_isr(void* arg);
void button_pin10_isr(void* arg);


// =============================== SETUP Functions =====================================

void gpioSetup() 
{
    // Initialize Button Pins
    gpio_pad_select_gpio(GPIO_NUM_0);
    gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
    gpio_pulldown_en(GPIO_NUM_0);
    gpio_pullup_dis(GPIO_NUM_0);
    gpio_set_intr_type(GPIO_NUM_0, GPIO_INTR_POSEDGE);

    gpio_pad_select_gpio(GPIO_NUM_1);
    gpio_set_direction(GPIO_NUM_1, GPIO_MODE_INPUT);
    gpio_pulldown_en(GPIO_NUM_1);
    gpio_pullup_dis(GPIO_NUM_1);
    gpio_set_intr_type(GPIO_NUM_1, GPIO_INTR_POSEDGE);

    gpio_pad_select_gpio(GPIO_NUM_10);
    gpio_set_direction(GPIO_NUM_10, GPIO_MODE_INPUT);
    gpio_pulldown_en(GPIO_NUM_10);
    gpio_pullup_dis(GPIO_NUM_10);
    gpio_set_intr_type(GPIO_NUM_10, GPIO_INTR_POSEDGE);
}

void isrSetup() 
{
    // Route ISRs
    if(gpio_install_isr_service(0 /* No Flags */) != ESP_OK){Serial.println("Issue Installing ISR Service");}

    if(gpio_isr_handler_add(GPIO_NUM_0,button_pin0_isr,NULL) != ESP_OK){Serial.printf("Issue Linking ISR for pin 0");}
    if(gpio_isr_handler_add(GPIO_NUM_1,button_pin1_isr,NULL) != ESP_OK){Serial.printf("Issue Linking ISR for pin 1");}
    if(gpio_isr_handler_add(GPIO_NUM_10,button_pin10_isr,NULL) != ESP_OK){Serial.printf("Issue Linking ISR for pin 10");}

}


// =============================== Interrupt Service Routines ===========================

// ISR for GPIO Pin 0 - connected to button that scores for team 1
void button_pin0_isr(void* arg) 
{
    // TODO look into debouncing buttons with an RC filter or if there is an internal ISR option to do so.
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result;
    result = xEventGroupSetBitsFromISR(global_event_group_handler, BUTTON1_BIT, &xHigherPriorityTaskWoken);
    if (result != pdFAIL)
    {
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    }
}

void button_pin1_isr(void* arg) 
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result;
    result = xEventGroupSetBitsFromISR(global_event_group_handler, BUTTON2_BIT, &xHigherPriorityTaskWoken);
    if (result != pdFAIL)
    {
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    }
}

void button_pin10_isr(void* arg) 
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result;
    result = xEventGroupSetBitsFromISR(global_event_group_handler, BUTTON3_BIT, &xHigherPriorityTaskWoken);
    if (result != pdFAIL)
    {
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    }
}


// =============================== RTOS Task Functions =============================================


// The job of the score update task is to listen for a button to be pressed and update the score based on the logic of a classic
// Pickleball game scoring system; This also involves updating the current server
void score_update_task(void* pvParameters) 
{
    EventGroupHandle_t event_handler = (EventGroupHandle_t)pvParameters;
    EventBits_t bits;

    while(1) 
    {
        bits = xEventGroupWaitBits(event_handler,
            BUTTON1_BIT | BUTTON2_BIT | BUTTON3_BIT,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY);
        
        if ((bits & BUTTON1_BIT) == BUTTON1_BIT) 
        {
            system_state = pd_team1_right;
            incrementScore();
        }
        if ((bits & BUTTON2_BIT) == BUTTON2_BIT) 
        {
            system_state = pd_team2_right;
            incrementScore();
        }
        if ((bits & BUTTON3_BIT) == BUTTON3_BIT) 
        {
            // Nothing yet
        }
        
    }

}

void highlight_court_task(TimerHandle_t timer)
{
    // Only blink the court light when waiting for service
    if (system_state == pd_team1_left || system_state == pd_team1_right || system_state == pd_team2_left ||  system_state == pd_team2_left) 
    {
        if ( (int) pvTimerGetTimerID(timer) == 0) 
        {
            displayInvertCourtSelect();
        } 
        else if ( (int) pvTimerGetTimerID(timer) == 1) 
        {
            displayInvertCourtDeselect();
        }
        
    }
}

void end_game_task(void* pvParameters) 
{

}


// =============================== Helper Functions ================================================

// Show boot up screen
void displayInit() 
{
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
    display.setCursor(29,43);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.write('0');
    display.write('0');

    display.setCursor(77,43);
    display.write('0');
    display.write('0');
    

    display.display();

}

void displayInvertCourtSelect() 
{
    if (system_state == pd_team1_right) {
        display.fillRect(25, 20, 30, 16, SSD1306_INVERSE);
        display.drawRect(24, 19, 32, 18, SSD1306_WHITE);
    } else if (system_state == pd_team1_left) {
        display.fillRect(25, 5, 30, 16, SSD1306_INVERSE);
        display.drawRect(24, 4, 32, 18, SSD1306_WHITE);
    } else if (system_state == pd_team2_right) {
        display.fillRect(73, 5, 30, 16, SSD1306_INVERSE);
        display.drawRect(72, 4, 32, 18, SSD1306_WHITE);
    } else if (system_state == pd_team2_left) {
        display.fillRect(73, 20, 30, 16, SSD1306_INVERSE);
        display.drawRect(72, 19, 32, 18, SSD1306_WHITE);
    } else {
        return;
    }

    display.display();
}

void displayInvertCourtDeselect() 
{
    if (system_state == pd_team1_right) {
        display.fillRect(25, 20, 30, 16, SSD1306_INVERSE);
        display.drawRect(24, 19, 32, 18, SSD1306_BLACK);

        // Fill in missing points
        display.drawPixel(25,19,SSD1306_WHITE);
        display.drawPixel(55,35,SSD1306_WHITE);
        display.drawPixel(54,19,SSD1306_WHITE);
        display.drawPixel(50,19,SSD1306_WHITE);
        display.drawPixel(52,19,SSD1306_WHITE);

    } else if (system_state == pd_team1_left) {
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

    } else if (system_state == pd_team2_right) {
        display.fillRect(73, 5, 30, 16, SSD1306_INVERSE);
        display.drawRect(72, 4, 32, 18, SSD1306_BLACK);

        // Fill in missing points
        display.drawPixel(73,21,SSD1306_WHITE);
        display.drawPixel(72,5,SSD1306_WHITE);
        display.drawPixel(102,21,SSD1306_WHITE);
        display.drawPixel(100,21,SSD1306_WHITE);

    } else if (system_state == pd_team2_left) {
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

void incrementScore() 
{
    // Increase the score of the serving team
    char tens;
    char ones;

    if (system_state == pd_team1_left || system_state == pd_team1_right) {
        if (team1_score < 21) {
            team1_score++;
            // Clear old score
            display.fillRect(29,43,29,16,SSD1306_BLACK);

            // Write new score
            display.setCursor(29,43);
            tens = (char) team1_score/10 + 48;
            ones = (char) team1_score%10 + 48;
            display.write(tens);
            display.write(ones);
        }
    } else if (system_state == pd_team2_left || system_state == pd_team2_right) {
        if (team2_score < 21) {
            team2_score++;
            // Clear old score
            display.fillRect(77,43,29,16,SSD1306_BLACK);

            // Write new score
            display.setCursor(77,43);
            tens = (char) team2_score/10 + 48;
            ones = (char) team2_score%10 + 48;
            display.write(tens);
            display.write(ones);
        }
    } else {
        return;
    }

    display.display();
    return;
}

void setScore(int s) 
{
    // Set the score of the serving team to a certain number


    return;
}

void printDebug() 
{
    // If DEBUG mode is on, display button inputs
    Serial.println("\n============================================");
    Serial.println("              D E B U G");
    Serial.printf("Button 1 Value: %d\n", gpio_get_level(GPIO_NUM_0));
    Serial.printf("Button 2 Value: %d\n", gpio_get_level(GPIO_NUM_1));
    Serial.printf("Button 3 Value: %d\n", gpio_get_level(GPIO_NUM_10));
    Serial.println("============================================");

}

// =============================== Main Application and Loop =======================================

void setup() 
{
    // Initialize Serial
    Serial.begin(9600);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    BaseType_t status;
    
    gpioSetup();
    isrSetup();

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDR)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }

    display.clearDisplay();
    displayInit();

    // ========== RTOS Task Setup ================

    // Non-Timer Tasks
    global_event_group_handler = xEventGroupCreate();

    status = xTaskCreate(score_update_task, "Score Update Task", 2048, (void*) global_event_group_handler, 3, NULL);
    if (status != pdPASS)
    {
        Serial.println("Task Creation Failed!");
        while(1);
    }

    // Timer Tasks
    // TimerHandle_t hightlightTimer, dehighlightTimer;

    // hightlightTimer = xTimerCreate("Highlight Court Timer", 
    //                                 500, /* 500 Milliseconds */
    //                                 pdTRUE, 
    //                                 (void*) 0, 
    //                                 highlight_court_task
    //                                 );
    // if (hightlightTimer == NULL) {
    //     Serial.println("Timer Creation Failed!");
    //     while(1);
    // }
    // // Start timer 2 after a 250 ms interval.
    // vTaskDelay(250);
    // dehighlightTimer = xTimerCreate("Dehighlight Court Timer", 
    //                                 500, /* 500 Milliseconds */
    //                                 pdTRUE, 
    //                                 (void*) 1, 
    //                                 highlight_court_task
    //                                 );
    // if (dehighlightTimer == NULL) {
    //     Serial.println("Timer Creation Failed!");
    //     while(1);
    // }


    //vTaskStartScheduler();
    //REMOVED, The ESP-IDF handles this internally, ESP has it's own version of FreeRTOS that uses all the same functionality
    //  but operates slightly differently: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos.html

}

void loop() 
{
    // Check for Debug:
    if (DEBUG_MODE) 
    {

    } 

}

