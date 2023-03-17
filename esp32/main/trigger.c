#include "lamp.h"

// TIMER_SCALE is value of timer at 1 second
#define TIMER_DIVIDER         16
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)

void pir_sensor_config(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = GPIO_PIR,
        .pull_down_en = 1,
        .pull_up_en = 0
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_PIR, gpio_isr_handler, NULL);
}


void timer_setup(gptimer_handle_t *gptimer, uint64_t seconds, gptimer_alarm_cb_t action)
{
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1
    };
    gptimer_alarm_config_t timer_alarm = {
        .alarm_count=seconds
    };
    gptimer_event_callbacks_t cbs = {
        .on_alarm = action, 
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, gptimer));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(*gptimer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(*gptimer, &timer_alarm));
    ESP_ERROR_CHECK(gptimer_enable(*gptimer));
}

void clock_start(void)
{
    
}

void clock_restart(void)
{
    
}


/*
void light_switch_config(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = GPIO_LIGHT_SWITCH,
        .pull_down_en = 1,
        .pull_up_en = 0
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_LIGHT_SWITCH, gpio_isr_handler, NULL);
}
*/