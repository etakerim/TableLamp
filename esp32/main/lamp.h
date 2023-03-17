#ifndef TABLE_LAMP_H
#define TABLE_LAMP_H

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "nvs_flash.h"


#define NUM_OF_LEDS     3
#define LED_RED         25
#define LED_GREEN       26
#define LED_BLUE        27

#define PIR_SENSOR      4
#define LIGHT_SWITCH    18

#define UART_RX         16     
#define UART_TX         17

#define I2C_SDA         21
#define I2C_SCL         22
#define I2C_PORT        I2C_NUM_0

#define GPIO_PIR                (1 << PIR_SENSOR)
#define GPIO_LIGHT_SWITCH       (1 << LIGHT_SWITCH)

#define BL_BUF_SIZE             (1024)
#define UART_PORT_NUM           UART_NUM_2
#define BL_BAUDRATE             (9600)


#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a): (b))

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB;

typedef struct {
    float h;
    float s;
    float v;
} HSV;


typedef struct {
    bool status;
    uint8_t brightness;
    uint16_t temperature;
    uint16_t threshold;
} Light;


void gpio_isr_handler(void* arg);

void pir_sensor_config(void);
void light_switch_config(void);
void bluetooth_config(void);
void i2c_config(void);


int led_duty_color(uint8_t level);
void led_config(void);
void led_set_color(RGB rgb);

float clamp(float value, float min, float max);

RGB kelvin_to_rgb(uint16_t kelvin);
HSV rgb_to_hsv(RGB rgb);
RGB hsv_to_rgb(HSV hsv);
void led_output(uint16_t temperature, uint8_t brightness);


void light_sensor_config(void);
void light_sensor_lux_interrupt(uint16_t low, uint16_t high);
uint16_t light_sensor_read_lux(void);


bool parse_commands(Light *light, char *stream);

esp_err_t nvs_load(Light *light);
esp_err_t nvs_save(Light *light);

void timer_setup(gptimer_handle_t *gptimer, uint64_t seconds, gptimer_alarm_cb_t action);

void clock_start(void);

void clock_restart(void);

#endif

/*
  *Hardware and tasks*

RGB Dimmable LED strip (PWM output)
    - GPIO25, OUT -> Green
    - GPIO26, OUT -> Red
    - GPIO27, OUT -> Blue

PIR sensor (PIR-SB312) - (Digital input)
    - GPIO4, INPUT PULLDOWN (Connected do flash)

Light sensor (WS-17146 TSL25911) (I2C)
    - GPIO22 -> SCL
    - GPIO21 -> SDA
    - INT (GPIO23)

Bluetooth (UART)
    - TXD2 (GPIO17) -> RXD (HC-05)
    - RXD2 (GPIO16) -> TXD (HC-05)
*/
