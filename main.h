#ifndef __MAIN_H__
#define __MAIN_H__
#include "WString.h"
#define   __APP__ ((char*)"T2609-LightHouse-Pico-RFM")

#define DEBUG_PRINT 
#define SEND_TEST_MSG 
#define MY_ADDR_LEN    8

#include <Arduino.h>

#define SerialX  Serial1

#define INTERVAL_READ_SENSOR    10000
#define INTERVAL_SEND_TEMP      600000
#define INTERVAL_FAST_SEND_TEMP 20000

typedef struct
{
    uint8_t sensor;
    uint8_t radio;
    uint8_t display;
    uint8_t watchdog;
} error_st;

typedef struct
{
    uint8_t         node_addr;
    uint32_t next_io_tick;
    char my_addr[MY_ADDR_LEN];
    error_st        error;
} main_ctrl_st;


typedef struct
{
    char            tag;
    char            addr;         
} modem_data_st;




#endif