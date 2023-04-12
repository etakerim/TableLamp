#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H


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


#define COMMAND_BIT           (0xA0)

#endif