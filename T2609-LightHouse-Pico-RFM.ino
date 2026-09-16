/******************************************************************************
    T2606_Picolina_RFM69_Sensor

*******************************************************************************
    I2C device found at address 0x10  !   VEML7700  
    I2C device found at address 0x44  !   SHT21

*******************************************************************************

******************************************************************************/

#include    <Wire.h>
#include    "main.h"
#include    "secrets.h"
#include    "r69.h"
#include    "atask.h"
#include    "io.h"
#include    "sensor.h"
#include    "super.h"

void modem_task(void);
void print_debug_task(void){ atask_print_status(true); }

 main_ctrl_st main_ctrl = {0};

atask_st modem_handle              = {"Radio Modem    ", 100,0, 0, 255, 0, 1, modem_task};
atask_st debug_th                  = {"Debug Task     ", 2000,    0,     0,  255,    0,  1,  print_debug_task };

void setup() {
    io_initialize();
    Serial1.setTX(PIN_TX0);   // UART0 == Serial1
    Serial1.setRX(PIN_RX0);
    Serial.begin(115200);
    Serial1.begin(9600);
    delay(2000);
    //while (!Serial); 
    Serial.print(__APP__); Serial.print(F(" Compiled: "));
    Serial.print(__DATE__); Serial.print(" ");
    Serial.print(__TIME__); Serial.println();
    Serial.println(__SENSOR__);

    atask_initialize();
    io_task_initialize();
    super_initialize();
    r69_initialize();
    sensor_initialize();
}

void loop() {
    atask_run();
}

