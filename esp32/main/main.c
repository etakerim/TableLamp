#include "lamp.h"


gptimer_handle_t timer = NULL;

TaskHandle_t gpio_task = NULL;
TaskHandle_t timer_task = NULL;

SemaphoreHandle_t light_mutex = NULL;
Light light = {
    .status = false,
    .movement = true,
    .temperature = 6000,
    .brightness = 80,
    .threshold = 50
};



bool IRAM_ATTR timer_on_alarm(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    BaseType_t higher_priority_woken = pdFALSE;
    vTaskNotifyGiveFromISR(timer_task, &higher_priority_woken);
    return higher_priority_woken == pdTRUE;
}


void IRAM_ATTR switch_isr_handler(void *args)
{
    BaseType_t higher_priority_woken = pdFALSE;
    vTaskNotifyGiveFromISR(gpio_task, &higher_priority_woken);
    portYIELD_FROM_ISR(higher_priority_woken);
}


void receive_commands_task(void* arg)
{
    char *stream = malloc(BL_BUF_SIZE);
    CommandAction cmd = BLCMD_NO_ACTION;
    bool movement = true;

    while (1) {
        int length = uart_read_bytes(UART_PORT_NUM, stream, BL_BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);
        if (length == 0)
            continue;

        stream[length] = '\0';
        uint16_t lux = 5; // light_sensor_read_lux();

        if (xSemaphoreTake(light_mutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
            cmd = parse_commands(&light, stream);
            movement = light.movement;
            xSemaphoreGive(light_mutex);
        }

        switch (cmd) {
            case BLCMD_DATA_CHANGE:
                if (xSemaphoreTake(light_mutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
                    if (light.status && lux <= light.threshold) {
                        if (light.movement) {
                            gptimer_set_raw_count(timer, 0);
                            gptimer_start(timer);
                        }
                        led_output(light.temperature, light.brightness);
                    } else {
                        if (light.movement) {
                            gptimer_stop(timer);
                        }
                        led_off();
                    }
                    xSemaphoreGive(light_mutex);
                }
                break;

            case BLCMD_DETECTION:
                movement_detection(movement, timer, timer_on_alarm, switch_isr_handler);
                if (light.status && light.movement) {
                    gptimer_set_raw_count(timer, 0);
                    gptimer_start(timer);
                }
                break;

            case BLCMD_REQUEST:
                int send_len = snprintf(
                    stream, BL_BUF_SIZE - 1, "@LAMP:%d,%d,%d,%d,%d",
                    light.status, light.movement, light.temperature,
                    light.brightness, light.threshold
                );
                uart_write_bytes(UART_PORT_NUM, (const char *) stream, send_len);  // TODO
                break;

            default:
                break;
        }
    }
    free(stream);
}



void light_switch_task(void* arg)
{
    bool target_state = (bool) arg;

    while (1) {
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) == pdTRUE) {
            uint16_t lux = 5; //TODO: light_sensor_read_lux();
            gptimer_set_raw_count(timer, 0);

            if (xSemaphoreTake(light_mutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
                if (light.status != target_state) {
                    light.status = target_state;
                    if (light.status && lux <= light.threshold) {
                        if (light.movement) {
                            gptimer_start(timer);
                        }
                        led_output(light.temperature, light.brightness);
                    } else {
                        if (light.movement) {
                            gptimer_stop(timer);
                        }
                        led_off();
                    }
                }
                xSemaphoreGive(light_mutex);
            }
        }
    }
}


void app_main(void)
{
    light_mutex = xSemaphoreCreateMutex();

    timer_setup(&timer, PIR_TIMEOUT_S, timer_on_alarm);
    pir_sensor_config(switch_isr_handler);
    led_config();

    i2c_config();
    light_sensor_config();

    bluetooth_config();

    // esp_err_t err = nvs_flash_init();
    // if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    //   ESP_ERROR_CHECK(nvs_flash_erase());     // NVS partition was truncated and needs to be erased
    //    ESP_ERROR_CHECK(nvs_flash_init());      //  Retry nvs_flash_init
    //}
    //nvs_load(&light);
    //if (light.status && lux <= light.threshold) {
    //     led_output(light.temperature, light.brightness);
    //} else {
    //     led_off();
    //}
    // movement_detection(light.movement, timer, timer_on_alarm, switch_isr_handler);

    xTaskCreate(light_switch_task, "detect_movement", 2048, (void *)true, 1, &gpio_task);
    xTaskCreate(light_switch_task, "timeout_light", 2048, (void *)false, 1, &timer_task);
    xTaskCreate(receive_commands_task, "receive_commands", 4096, NULL, 10, NULL);

    // IDLE Task
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
