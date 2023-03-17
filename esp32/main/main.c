#include "lamp.h"

TaskHandle_t gpio_task = NULL;
TaskHandle_t timer_task = NULL;
gptimer_handle_t gptimer = NULL;

SemaphoreHandle_t light_mutex;
Light light = {
    .status = false,
    .temperature = 6500,
    .brightness = 100,
    .threshold = 10
};


void IRAM_ATTR gpio_isr_handler(void *args)
{
    BaseType_t higher_priority_woken = pdFALSE;
    vTaskNotifyGiveFromISR(gpio_task, &higher_priority_woken);
    portYIELD_FROM_ISR(higher_priority_woken);
}

bool timer_on_alarm(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    BaseType_t higher_priority_woken = pdFALSE;
    vTaskNotifyGiveFromISR(timer_task, &higher_priority_woken);
    return higher_priority_woken == pdTRUE;
}

void light_switch_task(void* arg)
{
    bool status = (bool) arg;
    while(1) {
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) == pdTRUE) {
            uint16_t lux = light_sensor_read_lux();

            if (xSemaphoreTake(light_mutex, (TickType_t) 500) == pdTRUE) {
                light.status = status;
                if (light.status && lux <= light.threshold) {
                    // Todo: start timer
                    led_output(light.temperature, light.brightness);
                }
                xSemaphoreGive(light_mutex);
            }
        }
    }
}

void receive_commands_task(void* arg)
{
    uint8_t *stream = malloc(BL_BUF_SIZE);

    while (1) {
        int length = uart_read_bytes(UART_PORT_NUM, stream, BL_BUF_SIZE, portMAX_DELAY);

        if (length > 0) {
            stream[length] = '\0';
            if (parse_commands(&light, (char *) stream)) {
                nvs_save(&light);
                int16_t lux = light_sensor_read_lux();

                if (xSemaphoreTake(light_mutex, (TickType_t) 500) == pdTRUE) {
                    if (light.status && lux <= light.threshold) {
                        //Todo: start timer
                        led_output(light.temperature, light.brightness);
                    }
                    xSemaphoreGive(light_mutex);
                }
            }
        }
    }
    free(stream);
}


void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());     // NVS partition was truncated and needs to be erased 
        ESP_ERROR_CHECK(nvs_flash_init());      //  Retry nvs_flash_init
    }
    nvs_load(&light);

    light_mutex = xSemaphoreCreateMutex();
    if (light_mutex == NULL)
        return;


    i2c_config();
    light_sensor_config();

    bluetooth_config();    
    pir_sensor_config();
    led_config();

    timer_setup(&gptimer, 180, timer_on_alarm);

    xTaskCreate(light_switch_task, "detect_movement", 512, (void *)true, 10, &gpio_task);
    xTaskCreate(light_switch_task, "timeout_light", 512, (void *)false, 10, &timer_task);
    xTaskCreate(receive_commands_task, "receive_commands", 1024, NULL, 10, NULL);

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }


    // todo: program read gpio pin
}
/*
void app_main(void)
{
    uint16_t temperature = 6500;
    uint8_t intensity = 50;
    led_config();
    led_output(temperature, intensity);
}

void app_main(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = GPIO_PIR,
        .pull_down_en = 1,
        .pull_up_en = 0
    };
    gpio_config(&io_conf);

    while (1) {
        printf("P: %d\n", gpio_get_level(GPIO_PIR));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
*/