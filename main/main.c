#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/uart.h"

//LED and PB definitions
static const gpio_num_t LED_PINS[6] = {GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_22, GPIO_NUM_23};
static const gpio_num_t PB_PIN = GPIO_NUM_15;

//7seg definitions
static const gpio_num_t digitPins[2] = {GPIO_NUM_17, GPIO_NUM_16};
static const gpio_num_t segmentPins[8] = {GPIO_NUM_32, GPIO_NUM_27, GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_21, GPIO_NUM_26, GPIO_NUM_25};

//Constants
#define BITS 6  //binary bits used in this project
#define UART_NUM UART_NUM_0

static const char *TAG = "main";

static bool INC_Requested = false;
static int SevSegNum = 0;
static uint64_t T0_now = 0;
static uint64_t TS_prev = 0;
static uint64_t T1_prev = 0;
static uint64_t T2_prev = 0;
static uint64_t DB_T_prev = 0;
static bool PB_pressed = false;
static int iteration_time = 600;
static int hold_count = 0;

// 7seg lookup table for number translation
// segments: a,b,c,d,e,f,g,dp (dp not used in this project)
const uint8_t digitToSegments[10] = {
    0b00111111, //0
    0b00000110, //1
    0b01011011, //2
    0b01001111, //3
    0b01100110, //4
    0b01101101, //5
    0b01111101, //6
    0b00000111, //7
    0b01111111, //8
    0b01101111  //9
};

//Convert number to 6-bit binary string by using char array
void dec_to_bin(int num, char *buffer, int bufferSize){
    buffer[bufferSize-1] = '\0';   //need a "null" terminator at the end to make this a proper string in C
    int index = bufferSize - 2;
    for (int i=0; i<BITS; i++){   //populate bits
        if(num&1){                  //LSB check of each number, pass into buffer
            buffer[index] = '1';
        }
        else{
            buffer[index] = '0';
        }
        index--;                 //decrement to work from LSB to MSB
        num >>= 1;               //bitwise right-shift, peels off LSB (ex 0110->0011)
    }
}

//Set 7seg digit GPIO outputs (common cathode)
void set7SegDigit(uint8_t segments){
    for (int i=0; i< 8; i++){
        gpio_set_level(segmentPins[i], (segments >> i) & 1);
    }
}

void blankSegments(){                   //blank both segments to prevent leftover enable overlap
    for (int i=0; i<8; i++){
        gpio_set_level(segmentPins[i], 0); // Turn off all segments
    }
}

void displayNumber(int number){
    int digit0 = number % 10;          // right digit
    int digit1 = (number / 10) % 10;   // left digit

    // --- Display left digit (digit 1) ---
    gpio_set_level(digitPins[1], 1);  // Ensure right digit is OFF
    set7SegDigit(0);                  // Blank segments
    vTaskDelay(pdMS_TO_TICKS(1));     // <== DEAD TIME, prevents pulse overlap
    set7SegDigit(digitToSegments[digit1]);
    gpio_set_level(digitPins[0], 0);  // Enable left digit
    vTaskDelay(pdMS_TO_TICKS(5));
    gpio_set_level(digitPins[0], 1);  // Disable left digit

    // --- Display right digit (digit 0) ---
    gpio_set_level(digitPins[0], 1);  // Ensure left digit is OFF
    set7SegDigit(0);                  // Blank segments
    vTaskDelay(pdMS_TO_TICKS(1));     // <== DEAD TIME, prevents pulse overlap
    set7SegDigit(digitToSegments[digit0]);
    gpio_set_level(digitPins[1], 0);  // Enable right digit
    vTaskDelay(pdMS_TO_TICKS(5));
    gpio_set_level(digitPins[1], 1);  // Disable right digit

    set7SegDigit(0); // Ensure segments are always blank between cycles
}



// Debounce function, updates PB_pressed
void debounce(){
    int check1 = gpio_get_level(PB_PIN);
    if (check1 == 0){
        DB_T_prev = T0_now;
        PB_pressed = false;
    } else if ((check1 == 1) && (T0_now - DB_T_prev) > 50){
        PB_pressed = true;
    }
}

//use ESP-IDF command to obtain a familiar milliseconds call
static uint64_t millis(){
    return esp_timer_get_time()/1000;
}

void app_main(void)                                //setup task
{
    // Configure GPIOs
    gpio_config_t io_conf = {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    // Set LEDs as output
    for (int i=0; i<6; i++){
        io_conf.pin_bit_mask |= (1ULL << LED_PINS[i]);
    }
    gpio_config(&io_conf);

    // Set pushbutton pin as input
    gpio_config_t pb_conf = {
        .pin_bit_mask = (1ULL << PB_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, // Enable pull-up assuming button shorts to GND
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&pb_conf);

    // Configure 7-seg pins as outputs
    io_conf.pin_bit_mask = 0;
    for (int i=0; i<2; i++){
        io_conf.pin_bit_mask |= (1ULL << digitPins[i]);
    }
    for (int i=0; i<8; i++){
        io_conf.pin_bit_mask |= (1ULL << segmentPins[i]);
    }
    gpio_config(&io_conf);

    char binary[BITS + 1] = {0};

    while (1){                                //Main Loop
        T0_now = millis();                     //reference timer

        // Print to UART every 1s
        if ((T0_now - TS_prev) > 1000){
            TS_prev = T0_now;
            ESP_LOGI(TAG, "------------------------------------");
            // Print binary for debugging
            ESP_LOGI(TAG, "Binary: %s", binary);
        }

        dec_to_bin(SevSegNum, binary, sizeof(binary));

        // Clear LEDs
        for (int i = 0; i < BITS; i++){
            gpio_set_level(LED_PINS[i], 0);
        }
        // Set LEDs according to binary bits LSB to MSB
        for (int i = 0; i < BITS; i++){
            if (binary[i] == '1'){
                gpio_set_level(LED_PINS[i], 1);
            }
        }

        debounce();

        uint64_t T1_held = T0_now - T1_prev;

        // Refresh 7-seg display with multiplexing (run for ~20ms total)
        for (int i = 0; i < 300; i++){
            displayNumber(SevSegNum);
        }

        if (!INC_Requested && PB_pressed){
            INC_Requested = true;
            SevSegNum++;
        } else if (!PB_pressed){
            T1_prev = T0_now;
            hold_count = 0;
            iteration_time = 600;
        }

        if (INC_Requested && !PB_pressed){
            INC_Requested = false;
        }

        if (INC_Requested && PB_pressed && T1_held > 1000 && (T0_now - T2_prev) > iteration_time){
            T2_prev = T0_now;
            if (hold_count < 31){
                hold_count++;
            }
            SevSegNum++;
        }

        // Adjust iteration_time based on hold_count
        switch (hold_count){
            case 1: case 2:
                iteration_time = 300;
                break;
            case 3: case 4: case 5: case 6:
                iteration_time = 250;
                break;
            case 7: case 8: case 9: case 10:
            case 11: case 12: case 13: case 14:
                iteration_time = 180;
                break;
            case 15: case 16: case 17: case 18:
            case 19: case 20: case 21: case 22:
            case 23: case 24: case 25: case 26:
            case 27: case 28: case 29: case 30:
                iteration_time = 80;
                break;
            case 31:
                iteration_time = 40;
                break;
        }

        if (SevSegNum > 63){
            SevSegNum = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
