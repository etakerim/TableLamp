#include "lamp.h"


#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT  // Same as Color resolution
#define LEDC_FREQUENCY          (5000)            // Hz


ledc_channel_config_t leds[NUM_OF_LEDS];


void led_config(void)
{
    const int led_pins[NUM_OF_LEDS] = {
        LED_RED, LED_GREEN, LED_BLUE
    };
    const int led_channels[NUM_OF_LEDS] = {
        LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2
    };

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));


    for (uint8_t i = 0; i < NUM_OF_LEDS; i++) {
        leds[i].speed_mode  = LEDC_MODE;
        leds[i].channel = led_channels[i];
        leds[i].timer_sel = LEDC_TIMER,
        leds[i].intr_type = LEDC_INTR_DISABLE;

        leds[i].gpio_num = led_pins[i];
        leds[i].duty = 0;
        leds[i].hpoint = 0;

        ESP_ERROR_CHECK(ledc_channel_config(&leds[i]));
    }
}

void led_set_color(RGB rgb)
{
    uint8_t color[] = {rgb.r, rgb.g, rgb.b};

    for (uint8_t i = 0; i < NUM_OF_LEDS; i++) {
        ledc_set_duty(leds[i].speed_mode, leds[i].channel, color[i]);
        ledc_update_duty(leds[i].speed_mode, leds[i].channel);
    }
}

void led_off(void)
{
    for (uint8_t i = 0; i < NUM_OF_LEDS; i++) {
        ledc_set_duty(leds[i].speed_mode, leds[i].channel, 0);
        ledc_update_duty(leds[i].speed_mode, leds[i].channel);
    }
}

void led_output(uint16_t temperature, uint8_t brightness)
{
    //  Color temperature in Kelvin [1000, 40000]
    //  Brigthness in % [0, 100]
    RGB rgb = kelvin_to_rgb(temperature);
    HSV hsv = rgb_to_hsv(rgb);
    hsv.v = brightness / 100.0;
    RGB color = hsv_to_rgb(hsv);
    led_set_color(color);
}

float clamp(float value, float min, float max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

RGB kelvin_to_rgb(uint16_t kelvin)
{
    // http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
    RGB rgb = {.r = 0, .g = 0, .b = 0};

    if (kelvin < 1000 || kelvin > 40000) {
        return rgb;
    }

    float temperature = kelvin / 100.0;
    float color = 0;

    // Calculate Red
    if (temperature <= 66) {
        color = 255;
    } else {
        color = temperature - 60;
        color = 329.698727446 * pow(color, -0.1332047592);
    }
    rgb.r = clamp(color, 0, 255);

    // Calculate Green
    if (temperature <= 66) {
        color = temperature;
        color = 99.4708025861 * log(color) - 161.1195681661;
    } else {
        color = temperature - 60;
        color = 288.1221695283 * pow(color, -0.0755148492);
    }
    rgb.g = clamp(color, 0, 255);

    // Calculate Blue
    if (temperature >= 66) {
        color = 255;
    } else {
        if (temperature <= 19) {
            color = 0;
        } else {
            color = temperature - 10;
            color = 138.5177312231 * log(color) - 305.0447927307;
        }
    }
    rgb.b = clamp(color, 0, 255);

    return rgb;
}   

HSV rgb_to_hsv(RGB rgb)
{
    // https://gist.github.com/fairlight1337/4935ae72bcbcc1ba5c72
    // Input RGB must be in range: [0, 1]
    // HSV is in ranges H [0, 360]; S, V [0, 1]
    HSV hsv;
    float r = rgb.r / 255.0;
    float g = rgb.g / 255.0;
    float b = rgb.b / 255.0;

    float color_max = max(max(r, g), b);
    float color_min = min(min(r, g), b);
    float delta = color_max - color_min;

    if (delta > 0) {
        if(color_max == r) {
            hsv.h = 60 * (fmod(((g - b) / delta), 6));
        } else if(color_max == g) {
            hsv.h = 60 * (((b - r) / delta) + 2);
        } else if(color_max == b) {
            hsv.h = 60 * (((r - g) / delta) + 4);
        }

        if (color_max > 0) {
            hsv.s = delta / color_max;
        } else {
            hsv.s = 0;
        }

        hsv.v = color_max;
    } else {
        hsv.h = 0;
        hsv.s = 0;
        hsv.v = color_max;
    }

    if (hsv.h < 0) {
        hsv.h = 360 + hsv.h;
    }

    return hsv;
}

RGB hsv_to_rgb(HSV hsv)
{
    RGB rgb;
    float r = 0;
    float g = 0;
    float b = 0;

    float c = hsv.v * hsv.s;
    float hue = fmod(hsv.h / 60.0, 6);
    float x = c * (1 - fabs(fmod(hue, 2) - 1));
 
    float m = hsv.v - c;

    if ((int)hue == 0) {
        r = c;
        g = x;
        b = 0;

    } else if ((int)hue == 1) {
        r = x;
        g = c;
        b = 0;

    } else if ((int)hue == 2) {
        r = 0;
        g = c;
        b = x;

    } else if ((int)hue == 3) {
        r = 0;
        g = x;
        b = c;

    } else if ((int)hue == 4) {
        r = x;
        g = 0;
        r = c;

    } else if ((int)hue == 5) {
        r = c;
        g = 0;
        b = x;
    }

    r += m;
    g += m;
    b += m;

    rgb.r = r * 255;
    rgb.g = g * 255;
    rgb.b = b * 255;

    return rgb;
}