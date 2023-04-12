#include "lamp.h"
#include "lightsensor.h"




static void sensor_write(uint8_t reg, uint8_t value)
{
    i2c_write(TSL2591_ADDRESS, reg | COMMAND_BIT, value);
}

static void sensor_read(uint8_t reg, uint8_t *value)
{
    i2c_read(TSL2591_ADDRESS, reg | COMMAND_BIT, value, 1);
}

static void sensor_read_word(uint8_t reg, uint16_t *value)
{
    i2c_read_word(TSL2591_ADDRESS, reg | COMMAND_BIT, value);
}



static void tsl2591_enable(void)
{
    sensor_write(ENABLE_REGISTER, ENABLE_AIEN | ENABLE_POWERON | ENABLE_AEN | ENABLE_NPIEN);
}

static void tsl2591_disable(void)
{
    sensor_write(ENABLE_REGISTER, ENABLE_POWEROFF);
}


static void tsl2591_set_gain(uint8_t gain)
{
    uint8_t control = 0;

    sensor_read(CONTROL_REGISTER, &control);
    control &= 0xcf; 
    control |= gain;
    sensor_write(CONTROL_REGISTER, control);
}

static uint8_t tsl2591_get_gain(void)
{   
    uint8_t control = 0;
    sensor_read(CONTROL_REGISTER, &control);
    return control & 0x30;
}


static void tsl2591_set_integral_time(uint8_t time)
{
    uint8_t control = 0;

    sensor_read(CONTROL_REGISTER, &control);
    control &= 0xf8;
    control |= time;
    sensor_write(CONTROL_REGISTER, control);
}


static uint16_t tsl2591_read_lux(uint8_t gain, uint8_t time)
{
    tsl2591_enable();
    for (uint8_t i = 0; i < time + 2; i++){
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    uint16_t channel_0 = 0;
    uint16_t channel_1 = 0;
    
    sensor_read_word(CHAN0_LOW, &channel_0);
    sensor_read_word(CHAN1_LOW, &channel_1); 
    tsl2591_disable();

    tsl2591_enable();
    sensor_write(0xE7, 0x13);
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
                sensor_read_word(CHAN0_LOW, &channel_0);
                sensor_read_word(CHAN1_LOW, &channel_1);
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
    /*
    uint8_t data = 0;
    sensor_read(0x12, &data);
    printf("ID: 0x%x\n", data);  // Should be 0x50
    */

    tsl2591_enable();
    tsl2591_set_gain(MEDIUM_AGAIN);                         // 25X GAIN
    tsl2591_set_integral_time(ATIME_200MS);                 // 200ms Integration time
    sensor_write(PERSIST_REGISTER, 0x01);                   // Filter
    tsl2591_disable();
}

uint16_t light_sensor_read_lux(void)
{
    return tsl2591_read_lux(MEDIUM_AGAIN, ATIME_200MS);
}
