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


// After what time in seconds in light turned off
#define PIR_TIMEOUT_S       20

#define MUTEX_TIMEOUT_MS    ((TickType_t)(500))

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
    bool movement;
    uint8_t brightness;
    uint16_t temperature;
    uint16_t threshold;
} Light;

typedef enum {
    BLCMD_NO_ACTION,
    BLCMD_DATA_CHANGE,
    BLCMD_DETECTION,
    BLCMD_REQUEST
} CommandAction;


// PIR sensor
void pir_sensor_config(gpio_isr_t isr_handler);
void pir_add_isr(gpio_isr_t isr_handler);
void pir_remove_isr(void);
void movement_detection(
    bool allow, gptimer_handle_t gptimer, 
    gptimer_alarm_cb_t timer_action, gpio_isr_t gpio_handler
);

// LED lights
void led_config(void);
void led_off(void);
void led_set_color(RGB rgb);
void led_output(uint16_t temperature, uint8_t brightness);

float clamp(float value, float min, float max);
RGB kelvin_to_rgb(uint16_t kelvin);
HSV rgb_to_hsv(RGB rgb);
RGB hsv_to_rgb(HSV hsv);


// Light sensor
void i2c_config(void);
esp_err_t i2c_read(uint8_t i2c_address, uint8_t cmd, uint8_t *value, size_t len);
esp_err_t i2c_write(uint8_t i2c_address, uint8_t cmd, uint8_t value);
esp_err_t i2c_read_word(uint8_t i2c_address, uint8_t cmd, uint16_t *value);

void light_sensor_config(void);
uint16_t light_sensor_read_lux(void);

// Bluetooth
void bluetooth_config(void);
CommandAction parse_commands(Light *light, char *stream);
void bluetooth_send_status(char *stream, Light *light);

// Non-volatile storage
esp_err_t nvs_config();
esp_err_t nvs_load(Light *light);
esp_err_t nvs_save(Light *light);


void timer_setup(gptimer_handle_t *gptimer, uint64_t seconds, gptimer_alarm_cb_t action);



#endif

/*

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
