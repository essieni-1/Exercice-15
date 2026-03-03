#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define TRIG GPIO_NUM_11
#define ECHO GPIO_NUM_12
#define LOOP_DELAY_MS 1000
#define RANGE_SHORT 116
#define RANGE_LONG 23200

//global variables for timer and echo pulse time

esp_timer_handle_t oneshot_timer;
uint64_t echo_pulse_time = 0;

volatile int64_t rising_time = 0;
volatile int64_t falling_time = 0;


// ISR for the trigger pulse (sets TRIG low after 10 microseconds)
void IRAM_ATTR oneshot_timer_handler(void* arg)
{
    gpio_set_level(TRIG, 0);
}


// echo ISR

void IRAM_ATTR echo_isr_handler(void* arg)
{
    int level = gpio_get_level(ECHO);

    if (level == 1) {
        // rising edge
        rising_time = esp_timer_get_time();
    } else {
        // falling edge
        falling_time = esp_timer_get_time();
        echo_pulse_time = falling_time - rising_time;
    }
}


// initializing of pins and timer
void hc_sr04_init() {
    gpio_reset_pin(TRIG);
    gpio_set_direction(TRIG, GPIO_MODE_OUTPUT);
    gpio_set_level(TRIG, 0);

    gpio_reset_pin(ECHO);
    gpio_set_direction(ECHO, GPIO_MODE_INPUT);
    gpio_set_intr_type(ECHO, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(ECHO, echo_isr_handler, NULL);

    const esp_timer_create_args_t oneshot_timer_args = {
        .callback = &oneshot_timer_handler,
        .name = "one-shot"
    };

    esp_timer_create(&oneshot_timer_args, &oneshot_timer);
}


// main code

void app_main(void)
{
    hc_sr04_init();

    while (1)
    {
        // send 10 microseconds trigger pulse
        gpio_set_level(TRIG, 1);
        esp_timer_start_once(oneshot_timer, 10);

        // wait for the echo to complete
        vTaskDelay(pdMS_TO_TICKS(60));

        // check the valid range
        if (echo_pulse_time > RANGE_SHORT && echo_pulse_time < RANGE_LONG)
        {
            float distance = echo_pulse_time / 58.3;
            printf("Distance: %.2f cm\n", distance);
        }
        else
        {
            printf("Out of range\n");
        }
        vTaskDelay(pdMS_TO_TICKS(LOOP_DELAY_MS));
    }
}