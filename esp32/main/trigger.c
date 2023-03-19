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
    pir_add_isr(isr_handler);
}

void pir_add_isr(gpio_isr_t isr_handler)
{
    gpio_isr_handler_add(PIR_SENSOR, isr_handler, NULL);
}

void pir_remove_isr(void)
{
    gpio_isr_handler_remove(PIR_SENSOR);
}


void timer_setup(gptimer_handle_t *gptimer, uint64_t seconds, gptimer_alarm_cb_t action)
{
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1500               // Maximum prescaler: 65536 (res = 1200 Hz)
    };
    gptimer_alarm_config_t timer_alarm = {
        .alarm_count = (1500 * seconds),
    };

    gptimer_new_timer(&timer_config, gptimer);
    gptimer_set_alarm_action(*gptimer, &timer_alarm);

    gptimer_event_callbacks_t cbs = {
        .on_alarm = action, 
    };
    gptimer_register_event_callbacks(*gptimer, &cbs, NULL);
    
    gptimer_enable(*gptimer);
}


void movement_detection(
    bool allow, gptimer_handle_t timer, 
    gptimer_alarm_cb_t timer_action, gpio_isr_t gpio_action
) 
{
    if (allow) {
        pir_add_isr(gpio_action);
        gptimer_event_callbacks_t cbs = {
            .on_alarm = timer_action, 
        };
        gptimer_register_event_callbacks(timer, &cbs, NULL);
        gptimer_enable(timer);
    } else {
        pir_remove_isr();
        gptimer_stop(timer);
        gptimer_disable(timer);
    }
}