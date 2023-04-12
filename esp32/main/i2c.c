#include "lamp.h"


#define I2C_MASTER_NUM              0
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000

#define I2C_TIMEOUT                 (I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS)


void i2c_config(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(
        I2C_MASTER_NUM,
        conf.mode,
        I2C_MASTER_RX_BUF_DISABLE,
        I2C_MASTER_TX_BUF_DISABLE, 
        0
    ));
}

esp_err_t i2c_read(uint8_t i2c_address, uint8_t cmd, uint8_t *value, size_t len)
{
    esp_err_t err;
    err = i2c_master_write_read_device(
        I2C_MASTER_NUM, i2c_address, &cmd, 1, value, len, I2C_TIMEOUT
    );
    return err;
}

esp_err_t i2c_write(uint8_t i2c_address, uint8_t cmd, uint8_t value)
{
    uint8_t write_buf[2] = {cmd, value};
    esp_err_t err = i2c_master_write_to_device(
        I2C_MASTER_NUM, i2c_address, write_buf, 2, I2C_TIMEOUT
    );
    return err;
}

esp_err_t i2c_read_word(uint8_t i2c_address, uint8_t cmd, uint16_t *value)
{
    uint8_t buffer[2] = {};
    esp_err_t err = i2c_read(i2c_address, cmd, buffer, 2);

    *value = (buffer[1] << 8) | (buffer[0] & 0xFF);
    return err;
}
