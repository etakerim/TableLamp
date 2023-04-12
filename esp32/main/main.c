#include "lamp.h"


gptimer_handle_t timer = NULL;

TaskHandle_t gpio_task = NULL;
TaskHandle_t timer_task = NULL;

SemaphoreHandle_t light_mutex = NULL;
SemaphoreHandle_t lux_sensor_mutex = NULL;

Light light = {
    .status = false,
    .movement = true,
    .temperature = 5000,
    .brightness = 80,
    .threshold = 100
};


// Call inside mutex
void lamp_scene(uint16_t lux)
{
    if (light.status && lux <= light.threshold) {
        // Confirm request status in case of change by Bluetooth, but low lighting
        light.status = true;  
        if (light.movement) {
            gptimer_set_raw_count(timer, 0);
            gptimer_start(timer);
        }
        led_output(light.temperature, light.brightness);
    } else {
        light.status = false;
        if (light.movement) {
            gptimer_stop(timer);
        }
        led_off();
    }
}

uint16_t illuminance_level(void)
{
    uint16_t lx = 0;
    if (xSemaphoreTake(lux_sensor_mutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
        lx = light_sensor_read_lux();
        xSemaphoreGive(lux_sensor_mutex);
    }
    return lx;
}


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
        uint16_t lux = illuminance_level();

        // Needs mutex when accessing RGB LED
        if (xSemaphoreTake(light_mutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
            cmd = parse_commands(&light, stream);
            xSemaphoreGive(light_mutex);
            nvs_save(&light);
        }


        switch (cmd) {
            case BLCMD_DATA_CHANGE:
                if (xSemaphoreTake(light_mutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
                    lamp_scene(lux);
                    xSemaphoreGive(light_mutex);
                }
                bluetooth_send_status(stream, &light);
                break;

            case BLCMD_REQUEST:
                bluetooth_send_status(stream, &light);
                break;

            case BLCMD_DETECTION:
                movement_detection(movement, timer, timer_on_alarm, switch_isr_handler);
                if (light.status && light.movement) {
                    gptimer_set_raw_count(timer, 0);
                    gptimer_start(timer);
                }
                break;

            default:
                break;
        }
    }
    free(stream);
}



void light_switch_task(void* arg)
{
    char *stream = malloc(BL_BUF_SIZE);
    bool target_state = (bool) arg;
    bool change = false;

    while (1) {
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) == pdTRUE) {
            uint16_t lux = illuminance_level();
            gptimer_set_raw_count(timer, 0);

            if (xSemaphoreTake(light_mutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
                if (light.status != target_state) {
                    light.status = target_state;
                    lamp_scene(lux);
                    change = true;
                }
                xSemaphoreGive(light_mutex);
            }

            if (change) {
                change = false;
                bluetooth_send_status(stream, &light);
            }
        }
    }
    free(stream);
}

void debug_status(Light *light)
{
    printf(
        ("Light ------------ \n"
        "\tStatus: %d\n"
        "\tMovement: %d\n"
        "\tTemperature: %d K\n"
        "\tBrightness: %d %%\n"
        "\tThreshold: < %d lx\n"),
        light->status, light->movement, light->temperature,
        light->brightness, light->threshold
    );
}


void app_main(void)
{
    light_mutex = xSemaphoreCreateMutex();
    lux_sensor_mutex = xSemaphoreCreateMutex();

    nvs_config();
    // nvs_save(&light);  // Uncomment to rewrite NVS with defaults
    nvs_load(&light);

    timer_setup(&timer, PIR_TIMEOUT_S, timer_on_alarm);
    pir_sensor_config(switch_isr_handler);
    led_config();

    i2c_config();
    light_sensor_config();

    bluetooth_config();

    // Init lamp based on boot settings
    movement_detection(light.movement, timer, timer_on_alarm, switch_isr_handler);
    uint16_t lux = illuminance_level();
    lamp_scene(lux);        // Can call without mutex here - no other task runs at this time
    // debug_status(&light);

    xTaskCreate(light_switch_task, "detect_movement", 2048, (void *)true, 1, &gpio_task);
    xTaskCreate(light_switch_task, "timeout_light", 2048, (void *)false, 1, &timer_task);
    xTaskCreate(receive_commands_task, "receive_commands", 4096, NULL, 10, NULL);

    // IDLE Task
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
