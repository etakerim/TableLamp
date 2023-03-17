#include "lamp.h"


void bluetooth_config(void)
{
    uart_config_t uart_config = {
        .baud_rate = BL_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BL_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX, UART_RX, -1, -1));
}


static char *next_token(char *str)
{
    if (str == NULL)
        return str;

    while (*str != '\0' && isalpha((int)*str))
        str++;
    while (*str != '\0' && isblank((int)*str))
        str++;
    return str;
}


bool parse_commands(Light *light, char *stream) 
{
    char *pos;
    uint16_t value = 0;
    bool change = false;

    pos = next_token(strstr(stream, "STATUS "));
    if (pos != NULL && isdigit((int)*pos)) {
        light->status = !light->status;
        change = true;
    }

    pos = next_token(strstr(stream, "LUX "));
    if (pos != NULL && isdigit((int)*pos)) {
        value = atoi(pos);
        if (value <= 10000) {
            light->threshold = value;
            change = true;
        }
    }

    pos = next_token(strstr(stream, "KELVIN "));
    if (pos != NULL && isdigit((int)*pos)) {
        value = atoi(pos);
        if (value >= 1000 && value <= 40000) {
            light->temperature = value;
            change = true;
        }  
    }

    pos = next_token(strstr(stream, "BRIGHT "));
    if (pos != NULL && isdigit((int)*pos)) {
        value = atoi(pos);
        if (value <= 100) {
            light->brightness = value;
            change = true;
        }
    }

    return change;
}
