#include "lamp.h"


void app_main(void)
{
    uint16_t temperature = 2000;
    uint8_t intensity = 0;
    led_config();

    RGB color = {.r = 0, .g = 0, .b = 0};
    bool red_up = true;
    bool green_up = false;
    bool blue_up = false;

    while (1) {
        if (intensity >= 90)
            intensity = 0;

        if (red_up) {
            color.r += 8;
            if (color.r >= 248) {
                color.r = 0;
                red_up = false;
                green_up = true;
            }
        }
        if (green_up) {
            color.g += 8;
            if (color.g >= 248) {
                color.g = 0;
                green_up = false;
                blue_up = true;
            }
        }
        if (blue_up) {
            color.b += 8;
            if (color.b >= 248) {
                color.b = 0;
                blue_up = false;
                red_up = true;
            }
        }
        printf("COLOR: %d, %d, %d\n", color.r, color.g, color.b);
        
        led_output(temperature, intensity);
        intensity += 1;
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = GPIO_PIR,
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&io_conf);

    while (1) {
        printf("P: %d\n", gpio_get_level(PIR_SENSOR));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


void app_main(void)
{
    uint16_t temperature = 6500;
    uint8_t intensity = 100;
    led_config();

    led_output(temperature, intensity);
    while (1) {
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}