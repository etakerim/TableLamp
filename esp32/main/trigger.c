#include "lamp.h"


void pir_sensor_config(gpio_isr_t isr_handler)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = GPIO_PIR,
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIR_SENSOR, isr_handler, NULL));
}


void timer_setup(gptimer_handle_t *gptimer, uint64_t seconds, gptimer_alarm_cb_t action)
{
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1500               // Maximum prescaler: 65536 (res = 1200 Hz)
    };
    gptimer_alarm_config_t timer_alarm = {
        .alarm_count=(1500 * seconds),
    };
    gptimer_event_callbacks_t cbs = {
        .on_alarm = action, 
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, gptimer));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(*gptimer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(*gptimer, &timer_alarm));
    ESP_ERROR_CHECK(gptimer_enable(*gptimer));
}

void timer_start(gptimer_handle_t gptimer)
{
    gptimer_set_raw_count(gptimer, 0);
    gptimer_start(gptimer);
}

void timer_restart(gptimer_handle_t gptimer)
{
    gptimer_set_raw_count(gptimer, 0);
}

void timer_stop(gptimer_handle_t gptimer)
{
    gptimer_stop(gptimer);
}
