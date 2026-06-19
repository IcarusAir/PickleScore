/*
 * DISTRACTED DEVELOPMENT
 * PICKLESCORE
 *
 * This code is developed with the following libraries:
 *  - Adafruit Graphics
 *  - Adafruit SSD1306 Driver
 *  - Arduino development library
 *  - ESP-IDF development library
 *  - FreeRTOS
 * 
 * main.cpp - Firmware for PickleScore device
 */


#include "main.h"

// =============================== SETUP Functions =====================================

void gpioSetup() 
{
    // Initialize Button Pins
    gpio_pad_select_gpio(BUTTON1_PIN);
    gpio_set_direction(BUTTON1_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en(BUTTON1_PIN);
    gpio_pullup_dis(BUTTON1_PIN);
    gpio_set_intr_type(BUTTON1_PIN, GPIO_INTR_ANYEDGE);

    gpio_pad_select_gpio(BUTTON2_PIN);
    gpio_set_direction(BUTTON2_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en(BUTTON2_PIN);
    gpio_pullup_dis(BUTTON2_PIN);
    gpio_set_intr_type(BUTTON2_PIN, GPIO_INTR_ANYEDGE);

    // Initialize I2C Pins
    gpio_pad_select_gpio(SDA_PIN);
    gpio_pullup_en(SDA_PIN);
    gpio_pulldown_dis(SDA_PIN);

    gpio_pad_select_gpio(SCL_PIN);
    gpio_pullup_en(SCL_PIN);
    gpio_pulldown_dis(SCL_PIN);
}

// =============================== Main Application and Loop =======================================

void setup() 
{
    // Initialize Serial
    Serial.begin(9600);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    BaseType_t status;

    gpioSetup();
    Wire.begin(SDA_PIN, SCL_PIN);

    // Create Button task parameters (need to do so before we route ISRs)
    button_task_params_t button_params = {xEventGroupCreate(), 
                                          xTimerCreate("Boot Screen Timer", 
                                                        750 / portTICK_PERIOD_MS, 
                                                        pdTRUE, 
                                                        (void*) -1, 
                                                        flash_boot_screen_task)
                                         };

    // Install ISR Service
    if(gpio_install_isr_service(0 /* No Flags */) != ESP_OK){Serial.println("Issue Installing ISR Service");}

    // Route ISRs
    if(gpio_isr_handler_add(BUTTON1_PIN,button_pin0_isr,(void*) button_params.button_event_group) != ESP_OK){Serial.printf("Issue Linking ISR for pin 0");}
    if(gpio_isr_handler_add(BUTTON2_PIN,button_pin1_isr,(void*) button_params.button_event_group) != ESP_OK){Serial.printf("Issue Linking ISR for pin 1");}

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDR)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }

    display.clearDisplay();
    displayBootScreen();

    // ========== RTOS Task Setup ================

    // Non-Timer Tasks
    global_display_queue = xQueueCreate(3, sizeof(int));
    
    // Schedule DISPLAY TASK
    status = xTaskCreate(display_task, "Display Task", 2048, (void*) global_display_queue, 3, NULL); 
        /* Give the display task higher priority so that it can finish before being interrupted by another task */
    if (status != pdPASS)
    {
        Serial.println("Task Creation Failed!");
        while(1);
    }

    // Schedule BUTTON TASK - Main State handling task
    status = xTaskCreate(button_handle_task, "Button Handle Task", 2048, &button_params, 2, NULL);
    if (status != pdPASS)
    {
        Serial.println("Task Creation Failed!");
        while(1);
    }

    // Timer Tasks (Handles might need to be global to stop/restart the timers)

    // Start Boot Screen Timer
    status = xTimerStart(button_params.boot_timer, 0);
    if (status != pdPASS) {
        Serial.println("Timer Start Failed!");
        while(1);
    }

    TimerHandle_t hightlight_timer_handle = xTimerCreate("Highlight Court Timer", 
        750 / portTICK_PERIOD_MS, 
        pdTRUE, 
        (void*) 0, 
        highlight_court_task);
    status = xTimerStart(hightlight_timer_handle, 0);
    if (status != pdPASS) {
        Serial.println("Timer Start Failed!");
        while(1);
    }

    TimerHandle_t dehightlight_timer_handle = xTimerCreate("Dehighlight Court Timer", 
        750 / portTICK_PERIOD_MS, 
        pdTRUE, 
        (void*) 1, 
        highlight_court_task);
    /* Start timer 2 after a 250 ms interval. */
    vTaskDelay(375 / portTICK_PERIOD_MS);
    status = xTimerStart(dehightlight_timer_handle, 0);
    if (status != pdPASS) {
        Serial.println("Timer Start Failed!");
        while(1);
    }

    //vTaskStartScheduler();
    //REMOVED, The ESP-IDF handles this internally, ESP has it's own version of FreeRTOS that uses all the same functionality
    //  but operates slightly differently: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos.html

}

void loop() {}

