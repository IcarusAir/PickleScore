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

#define BUTTON1_PIN GPIO_NUM_0
#define BUTTON2_PIN GPIO_NUM_1
#define BUTTON3_PIN GPIO_NUM_10

#define BUTTON1_BIT (1 << 0)
#define BUTTON2_BIT (1 << 1)
#define BUTTON3_BIT (1 << 2)

#define DEBUG_MODE false

// ============================= GLOBAL DEFINITIONS ====================================

// Adafruit Display variables - Switch Comments to toggle emulator
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, SCREEN_RST);
Adafruit_SSD1306_EMULATOR display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, SCREEN_RST);

enum state {
    startup = 0,
    /* Pickleball Doubles*/
    pd_team1_right = 1,
    pd_team1_left = 2,
    pd_team2_right = 3,
    pd_team2_left = 4,
    pd_between_games = 5,
    pd_end_of_games = 6,
    /* Badminton Singles*/
    bs_team1 = 7,
    bs_team2 = 8,
    bs_between_games = 9,
    bs_end_of_games = 10
};

enum state system_state = startup;

// An 8-bit set of flags showing which courts are highlighted and current scoring flags
// Bits 7:4 - court light data (LL - 7, UL - 6, UR - 5, LR - 4)
// Bits 1:0 - Scoring flags (team 1 - 1, team 2 - 0)
uint8_t court_data = 0x0;

uint8_t team1_score = 0;
uint8_t team2_score = 0;

QueueHandle_t global_display_queue; // A queue makes more sense for a display (multiple display requests can happen)
EventGroupHandle_t global_event_group_handler;

// ============================= FUNCTION DEFINITIONS ====================================
void displayCourtInit();
void displayInvertCourtSelect(uint8_t court);
void displayInvertCourtDeselect(uint8_t court);
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
    gpio_pad_select_gpio(BUTTON1_PIN);
    gpio_set_direction(BUTTON1_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en(BUTTON1_PIN);
    gpio_pullup_dis(BUTTON1_PIN);
    gpio_set_intr_type(BUTTON1_PIN, GPIO_INTR_POSEDGE);

    gpio_pad_select_gpio(BUTTON2_PIN);
    gpio_set_direction(BUTTON2_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en(BUTTON2_PIN);
    gpio_pullup_dis(BUTTON2_PIN);
    gpio_set_intr_type(BUTTON2_PIN, GPIO_INTR_POSEDGE);

    gpio_pad_select_gpio(BUTTON3_PIN);
    gpio_set_direction(BUTTON3_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en(BUTTON3_PIN);
    gpio_pullup_dis(BUTTON3_PIN);
    gpio_set_intr_type(BUTTON3_PIN, GPIO_INTR_POSEDGE);
}

void isrSetup() 
{
    // Install ISR Service
    if(gpio_install_isr_service(0 /* No Flags */) != ESP_OK){Serial.println("Issue Installing ISR Service");}

    // Route ISRs
    if(gpio_isr_handler_add(BUTTON1_PIN,button_pin0_isr,NULL) != ESP_OK){Serial.printf("Issue Linking ISR for pin 0");}
    if(gpio_isr_handler_add(BUTTON2_PIN,button_pin1_isr,NULL) != ESP_OK){Serial.printf("Issue Linking ISR for pin 1");}
    if(gpio_isr_handler_add(BUTTON3_PIN,button_pin10_isr,NULL) != ESP_OK){Serial.printf("Issue Linking ISR for pin 10");}

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


// Display Task
// Interrupting the current display.display() will crash the emulator and cause graphical glitches in the screen
// This is a higher priority task that ensures that a screen 
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
    if (system_state == pd_team1_right || system_state == pd_team1_left || system_state == pd_team2_right || system_state == pd_team2_left) 
    {   
        // TODO Needs to catch when we are interrupting the cycle from deselect to select, causing graphics problems
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
                    court_data = 0x00;
                    break;
                case 0x40: // UL court highlighed
                    displayInvertCourtDeselect(1);
                    court_data = 0x00;
                    break;
                case 0x20: // UR court highlighed
                    displayInvertCourtDeselect(2);
                    court_data = 0x00;
                    break;
                case 0x10: // LR court highlighed
                    displayInvertCourtDeselect(3);
                    court_data = 0x00;
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

void end_game_task(void* pvParameters) 
{

}


// =============================== Helper Functions ================================================

// TODO: Remove logic based on the system state, these should only use accepted parameters!


// Show boot up screen
void displayCourtInit() 
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
            display.drawPixel(50,19,SSD1306_WHITE);
            display.drawPixel(52,19,SSD1306_WHITE);
            break;

        case 1: //pd_team1_left
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
            break;

        case 2: //pd_team2_right
            display.fillRect(73, 5, 30, 16, SSD1306_INVERSE);
            display.drawRect(72, 4, 32, 18, SSD1306_BLACK);

            // Fill in missing points
            display.drawPixel(73,21,SSD1306_WHITE);
            display.drawPixel(72,5,SSD1306_WHITE);
            display.drawPixel(102,21,SSD1306_WHITE);
            display.drawPixel(100,21,SSD1306_WHITE);
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

void incrementScore() 
{
    // Increase the score of the serving team
    char tens;
    char ones;
    int buff = 0;

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

    //display.display();
    xQueueSendToBack(global_display_queue, (void *) &buff, 0);
}

// Still Incomplete
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
    displayCourtInit();

    // ========== RTOS Task Setup ================

    // Non-Timer Tasks
    global_display_queue = xQueueCreate(3, sizeof(int));
    status = xTaskCreate(display_task, "Display Task", 2048, (void*) global_display_queue, 3, NULL); 
    /* Give the display task higher priority so that it can finish before being interrupted by another task */
    if (status != pdPASS)
    {
        Serial.println("Task Creation Failed!");
        while(1);
    }

    global_event_group_handler = xEventGroupCreate();
    status = xTaskCreate(score_update_task, "Score Update Task", 2048, (void*) global_event_group_handler, 2, NULL);
    if (status != pdPASS)
    {
        Serial.println("Task Creation Failed!");
        while(1);
    }

    // Timer Tasks (Handles might need to be global to stop/restart the timers)
    TimerHandle_t hightlight_timer_handle = xTimerCreate("Highlight Court Timer", 
        3000 / portTICK_PERIOD_MS, 
        pdTRUE, 
        (void*) 0, 
        highlight_court_task);
    status = xTimerStart(hightlight_timer_handle, 0);
    if (status != pdPASS) {
        Serial.println("Timer Start Failed!");
        while(1);
    }

    TimerHandle_t dehightlight_timer_handle = xTimerCreate("Dehighlight Court Timer", 
        3000 / portTICK_PERIOD_MS, 
        pdTRUE, 
        (void*) 1, 
        highlight_court_task);
    /* Start timer 2 after a 250 ms interval. */
    vTaskDelay(1500 / portTICK_PERIOD_MS);
    status = xTimerStart(dehightlight_timer_handle, 0);
    if (status != pdPASS) {
        Serial.println("Timer Start Failed!");
        while(1);
    }


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

