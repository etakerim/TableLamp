#include "lamp.h"


/*
TODO: CHECKLIST
- [] Debug I2C driver for read_lux (write je divný)
- [] Debug Bluetooth HC-05 - nespáruje sa
- [] Android app bluetooth send to PC z appky (refcomm terminal)
- [] Android app bluetooth send to ESP32 (change off/on switch to button)
- [] Mód na vypnutie PIR
- [] Dokumentácia

*/

/*
void app_main(void)
{    
    i2c_config();
    //light_sensor_config();

    // IDLE Task
    while (1) {
        //uint16_t lux = light_sensor_read_lux();
        //printf("Lx: %d\n", lux);
        light_sensor_config();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
*/


gptimer_handle_t timer = NULL;

TaskHandle_t gpio_task = NULL;
TaskHandle_t timer_task = NULL;

SemaphoreHandle_t light_mutex = NULL;
Light light = {
    .status = false,
    .temperature = 4000,
    .brightness = 80,
    .threshold = 50
};



bool IRAM_ATTR timer_on_alarm(gptimer_handle_t gptimer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    BaseType_t higher_priority_woken = pdFALSE;
    timer_stop(gptimer);
    vTaskNotifyGiveFromISR(timer_task, &higher_priority_woken);
    return higher_priority_woken == pdTRUE;
}


void IRAM_ATTR switch_isr_handler(void *args)
{
    BaseType_t higher_priority_woken = pdFALSE;
    timer_restart(timer);
    vTaskNotifyGiveFromISR(gpio_task, &higher_priority_woken);
    portYIELD_FROM_ISR(higher_priority_woken);
}


void receive_commands_task(void* arg)
{
    uint8_t *stream = malloc(BL_BUF_SIZE);

    while (1) {
        int length = uart_read_bytes(UART_PORT_NUM, stream, BL_BUF_SIZE, portMAX_DELAY);
        printf("L: %d\n", length);

        if (length > 0) {
            stream[length] = '\0';
            printf("%s\n", stream);
            uint16_t lux = 5; // light_sensor_read_lux();

            if (xSemaphoreTake(light_mutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
                if (parse_commands(&light, (char *) stream)) {
                    //nvs_save(&light);
                    if (light.status && lux <= light.threshold) {
                        timer_start(timer);
                        led_output(light.temperature, light.brightness);
                    } else {
                        led_off();
                    }
                }
                xSemaphoreGive(light_mutex);
            }
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

            if (xSemaphoreTake(light_mutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
                if (light.status != target_state) {
                    light.status = target_state;
                    if (light.status && lux <= light.threshold) {
                        timer_start(timer);
                        led_output(light.temperature, light.brightness);
                    } else {
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
    /*
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());     // NVS partition was truncated and needs to be erased 
        ESP_ERROR_CHECK(nvs_flash_init());      //  Retry nvs_flash_init
    }
    nvs_load(&light);
    */
    light_mutex = xSemaphoreCreateMutex();

    timer_setup(&timer, PIR_TIMEOUT_S, timer_on_alarm);
    pir_sensor_config(switch_isr_handler);
    led_config();

    i2c_config();
    light_sensor_config();

    bluetooth_config();

    xTaskCreate(light_switch_task, "detect_movement", 4096, (void *)true, 1, &gpio_task);
    xTaskCreate(light_switch_task, "timeout_light", 4096, (void *)false, 1, &timer_task);
    xTaskCreate(receive_commands_task, "receive_commands", 4096, NULL, 10, NULL);

    // IDLE Task
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
