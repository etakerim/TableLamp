#include "lamp.h"


#define I2C_MASTER_NUM              0
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000


#define TSL2591_ADDRESS       (0x29)

#define COMMAND_BIT           (0xA0)
// Register (0x00)
#define ENABLE_REGISTER       (0x00)
#define ENABLE_POWERON        (0x01)
#define ENABLE_POWEROFF       (0x00)
#define ENABLE_AEN            (0x02)
#define ENABLE_AIEN           (0x10)
#define ENABLE_SAI            (0x40)
#define ENABLE_NPIEN          (0x80)

#define CONTROL_REGISTER      (0x01)
#define SRESET                (0x80)
// AGAIN
#define LOW_AGAIN             (0x00)    // Low gain (1x)
#define MEDIUM_AGAIN          (0x10)    // Medium gain (25x)
#define HIGH_AGAIN            (0x20)    // High gain (428x)
#define MAX_AGAIN             (0x30)    // Max gain (9876x)
// ATIME
#define ATIME_100MS           (0x00)    // 100 millis   
#define ATIME_200MS           (0x01)    // 200 millis
#define ATIME_300MS           (0x02)    // 300 millis
#define ATIME_400MS           (0x03)    // 400 millis
#define ATIME_500MS           (0x04)    // 500 millis
#define ATIME_600MS           (0x05)    // 600 millis

#define AILTL_REGISTER        (0x04)
#define AILTH_REGISTER        (0x05)
#define AIHTL_REGISTER        (0x06)
#define AIHTH_REGISTER        (0x07)
#define NPAILTL_REGISTER      (0x08)
#define NPAILTH_REGISTER      (0x09)
#define NPAIHTL_REGISTER      (0x0A)
#define NPAIHTH_REGISTER      (0x0B)

#define PERSIST_REGISTER      (0x0C)
#define ID_REGISTER           (0x12)

#define STATUS_REGISTER       (0x13)

#define CHAN0_LOW             (0x14)
#define CHAN0_HIGH            (0x15)
#define CHAN1_LOW             (0x16)
#define CHAN1_HIGH            (0x14)

//LUX_DF   GA * 53   GA is the Glass Attenuation factor 
#define LUX_DF                (762.0)
// LUX_DF                408.0
#define MAX_COUNT_100MS       (36863) // 0x8FFF
#define MAX_COUNT             (65535) // 0xFFFF


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

static esp_err_t i2c_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(
        I2C_MASTER_NUM,
        TSL2591_ADDRESS,
        &reg_addr,
        1,
        data,
        len,
        I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS
    );
}

static esp_err_t i2c_write(uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};

    return i2c_master_write_to_device(
        I2C_MASTER_NUM,
        TSL2591_ADDRESS,
        write_buf,
        sizeof(write_buf),
        I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS
    );
}

static uint16_t i2c_read_word(uint8_t reg_addr)
{
    uint8_t packet[2] = {};
    i2c_read(reg_addr, packet, 2);
    return (packet[0] << 8) | (packet[0] & 0xFF);
}




static void tsl2591_enable(void)
{
    i2c_write(ENABLE_REGISTER, ENABLE_AIEN | ENABLE_POWERON | ENABLE_AEN | ENABLE_NPIEN);
}

static void tsl2591_disable(void)
{
    i2c_write(ENABLE_REGISTER, ENABLE_POWEROFF);
}

static void tsl2591_set_gain(uint8_t gain)
{
    uint8_t control = 0;

    i2c_read(CONTROL_REGISTER, &control, 1);
    control &= 0xcf; 
    control |= gain;
    i2c_write(CONTROL_REGISTER, control);
}

static uint8_t tsl2591_get_gain(void)
{   
    uint8_t data;
    i2c_read(CONTROL_REGISTER, &data, 1);
    return data & 0x30;
}


static void tsl2591_set_integral_time(uint8_t time)
{
    uint8_t control = 0;

    i2c_read(CONTROL_REGISTER, &control, 1);
    control &= 0xf8;
    control |= time;
    i2c_write(CONTROL_REGISTER, control);
}

static void tsl2591_lux_interrupt(uint16_t low, uint16_t high, uint8_t gain, uint8_t time)
{
    uint16_t atime  = 100 * time + 100;
    double again = 1.0;

    if (gain == MEDIUM_AGAIN) {
        again = 25.0;
    } else if (gain == HIGH_AGAIN) {
        again = 428.0;
    } else if (gain == MAX_AGAIN) {
        again = 9876.0;
    }
    
    double cpl = (atime * again) / LUX_DF;
    uint16_t ch = i2c_read_word(CHAN1_LOW);
    
    high =  (int)(cpl * high) + 2 * ch - 1;
    low = (int)(cpl * low) + 2 * ch + 1;
		
    tsl2591_enable();
    i2c_write(AILTL_REGISTER, low & 0xFF);
    i2c_write(AILTH_REGISTER, low >> 8);
    
    i2c_write(AIHTL_REGISTER, high & 0xFF);
    i2c_write(AIHTH_REGISTER, high >> 8);
    
    i2c_write(NPAILTL_REGISTER, 0);
    i2c_write(NPAILTH_REGISTER, 0);
    
    i2c_write(NPAIHTL_REGISTER, 0xff);
    i2c_write(NPAIHTH_REGISTER, 0xff);
    tsl2591_disable();
}

static uint16_t tsl2591_read_lux(uint8_t gain, uint8_t time)
{
    tsl2591_enable();
    for (uint8_t i = 0; i < time + 2; i++){
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    uint16_t channel_0 = i2c_read_word(CHAN0_LOW);
    uint16_t channel_1 = i2c_read_word(CHAN1_LOW);
    tsl2591_disable();

    tsl2591_enable();
    i2c_write(0xE7, 0x13);
    tsl2591_disable();
    
    uint16_t atime = 100 * time + 100;
    uint16_t max_counts;

    if (time == ATIME_100MS) {
        max_counts = MAX_COUNT_100MS;
    } else {
        max_counts = MAX_COUNT;
    }

    if (channel_0 >= max_counts || channel_1 >= max_counts) {
        
        uint8_t gain_level = tsl2591_get_gain();
        
        if (gain_level != LOW_AGAIN){
            gain_level = ((gain_level >> 4) - 1) << 4;
            tsl2591_set_gain(gain_level);
        
            channel_0 = 0;
            channel_1 = 0;
            while(channel_0 <= 0 || channel_1 <= 0) {
                i2c_read_word(CHAN0_LOW);
                i2c_read_word(CHAN1_LOW);
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        } else {
            return 0;
        }
    }

    double again = 1.0;
    if (gain == MEDIUM_AGAIN) {
        again = 25.0;
    } else if (gain == HIGH_AGAIN) {
        again = 428.0;
    } else if (gain == MAX_AGAIN) {
        again = 9876.0;
    }
    double cpl = (atime * again) / LUX_DF;
    
    return ((channel_0 - (2 * channel_1)) / cpl);
}


void light_sensor_config(void)
{
    // i2c_read(&ID_REGISTER, 1);
    tsl2591_enable();
    tsl2591_set_gain(MEDIUM_AGAIN);             // 25X GAIN
    tsl2591_set_integral_time(ATIME_200MS);     // 200ms Integration time
    i2c_write(PERSIST_REGISTER, 0x01);          // Filter
    tsl2591_disable();
}

void light_sensor_lux_interrupt(uint16_t low, uint16_t high)
{
    return tsl2591_lux_interrupt(low, high, MEDIUM_AGAIN, ATIME_200MS);
}

uint16_t light_sensor_read_lux(void)
{
    return tsl2591_read_lux(MEDIUM_AGAIN, ATIME_200MS);
}
