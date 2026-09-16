#include "Arduino.h"
#include "sensor.h"
#include "main.h"
#include "io.h"
#include "atask.h"
#include "Adafruit_BMP085.h"
#include <Adafruit_BMP280.h>
#include "Adafruit_BME680.h"
#include <Adafruit_AHTX0.h>
#include "Adafruit_SHT31.h"
#include "Adafruit_VEML7700.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "r69.h"
#include "super.h"


#define SEALEVELPRESSURE_HPA    (1013.25)
#define SEND_INTERVAL           (600000)
#define SEND_BUFF_LEN           (60)
#define NBR_OF_VALUE_POINTS     (10)
#define MIN_NBR_OF_VALID_POINTS (6)
#define LOW_CHECK_MULTIPL       (0.8)
#define HIGH_CHECK_MULTIPL      (1.2)

typedef struct 
{
    float value;
    uint8_t valid;
} value_point_st;

typedef struct
{
    value_point_st vp[NBR_OF_VALUE_POINTS];
    float   filtered_value;
    uint8_t counter;
    bool    is_ready;
} value_series_st;

typedef struct
{
    uint8_t     sensor_indx;
    char        buff[SEND_BUFF_LEN];
    uint32_t    timeout;
    uint16_t    pir_state;
    uint16_t    pir_cntr;
    bool        pir_is_active;
} sensor_ctrl_st;




//TwoWire *Wirep;
extern main_ctrl_st main_ctrl;

// function prototype
void sensor_print_bmp280_data(void);
void sensor_set_bmp280_configuration(void);
void sensor_task(void);
void sensor_test_task(void);


uint8_t active_node = SENSOR_NODE;
#if SENSOR_NODE == SENSOR_NODE_PIHA1
    typedef enum
    {
        VALUE_INDEX_TEMPERATURE = 0,
        VALUE_INDEX_HUMIDITY,
        VALUE_INDEX_LUX,
        VALUE_INDEX_NBR_OF
    } value_index_et;

    sensor_node_st sensor_node =
    {
        .send_interval = 30000, 
        .next_send = 0,
    };

#elif ((SENSOR_NODE == SENSOR_NODE_KHH) || (SENSOR_NODE == SENSOR_NODE_TUPA1))
    typedef enum
    {
        VALUE_INDEX_TEMPERATURE = 0,
        VALUE_INDEX_HUMIDITY,
        VALUE_INDEX_NBR_OF
    } value_index_et;

    sensor_node_st sensor_node =
    {
        .send_interval = 30000, 
        .next_send = 0,
    };

#else
    typedef enum
    {
        VALUE_INDEX_TEMPERATURE = 0,
        VALUE_INDEX_HUMIDITY,
        VALUE_INDEX_NBR_OF
    } value_index_et;

    sensor_node_st sensor_node =
    {
        .send_interval = 200000, 
        .next_send = 0,
    };
#endif

value_series_st vseries[VALUE_INDEX_NBR_OF] = {0};

// sensor_node_st sensor_node[SENSOR_NODE_NBR_OF] =
// {
//     [SENSOR_NODE_UNDEFINED] = {.send_interval = 200000, .next_send = 0},
//     [SENSOR_NODE_PIHA1]     = {.send_interval = 300000, .next_send = 0},
//     [SENSOR_NODE_]          = {.send_interval = 600000, .next_send = 0}

// };

sensor_ctrl_st sensor_ctrl ={0};

// atask_st:            = {"Label          ", ival, next, state, prev, cntr, run, task_ptr };

atask_st sensor_handle  = {"Sensor Task   ", 1000,    0,     0,  255,    0,   1, sensor_task};


Adafruit_BMP085 bmp180;
Adafruit_BMP280 bmp280;
Adafruit_BME680 bme680(&Wire); 
Adafruit_AHTX0 aht20;
Adafruit_SHT31 sht31 = Adafruit_SHT31();
OneWire oneWire(PIN_ONE_WIRE);
DallasTemperature ds18b20(&oneWire);
Adafruit_VEML7700 veml = Adafruit_VEML7700();


sensor_st sensor[SENSOR_TYPE_NBR_OF] =
{
    [SENSOR_TYPE_UNDEFINED] = {
            .meta = {.label= "Undef  ", .i2c_addr = 0x00, .active=false, .status=0, .updated=false, 
            .show_bm = SENSOR_SHOW_BM_NONE, .counter= 0, .next_meas=0},
            .temperature = 0.0, .humidity = 0.0, .pressure = 0, .float_val = 0.0, .on_off = 0
    },
    [SENSOR_TYPE_BMP180] = {
            .meta = {.label= "BMP180 ", .i2c_addr = I2C_ADDR_BMP180, .active=false, .status=0, .updated=false, 
            .show_bm = SENSOR_SHOW_BM_TEMP, .counter= 0, .next_meas=0},
            .temperature = 0.0, .humidity = 0.0, .pressure = 0, .float_val = 0.0, .on_off = 0
    },
    [SENSOR_TYPE_BMP280] = {
            .meta = {.label= "BMP280 ", .i2c_addr = I2C_ADDR_BMP280, .active=false, .status=0, .updated=false,
            .show_bm = SENSOR_SHOW_BM_TEMP | SENSOR_SHOW_BM_HUM, .counter= 0, .next_meas=0},
            .temperature = 0.0, .humidity = 0.0, .pressure = 0, .float_val = 0.0, .on_off = 0
    },
    [SENSOR_TYPE_BME680] = {
            .meta = {.label= "BME680 ", .i2c_addr = I2C_ADDR_BME680, .active=false, .status=0, .updated=false, 
            .show_bm = SENSOR_SHOW_BM_TEMP | SENSOR_SHOW_BM_HUM | SENSOR_SHOW_BM_PRESS, 
            .counter= 0, .next_meas=0},
            .temperature = 0.0, .humidity = 0.0, .pressure = 0, .float_val = 0.0, .on_off = 0
    },
    [SENSOR_TYPE_AHT20] = {
            .meta = {.label= "AHT20  ", .i2c_addr = I2C_ADDR_AHT20 , .active=false, .status=0, .updated=false, 
            .show_bm = SENSOR_SHOW_BM_TEMP, .counter= 0, .next_meas=0},
            .temperature = 0.0, .humidity = 0.0, .pressure = 0, .float_val = 0.0, .on_off = 0
    },
    [SENSOR_TYPE_SHT31] = {
            .meta = {.label= "SHT31  ", .i2c_addr = I2C_ADDR_SHT31, .active=false, .status=0, .updated=false, 
            .show_bm = SENSOR_SHOW_BM_TEMP | SENSOR_SHOW_BM_HUM, .counter= 0, .next_meas=0},
            .temperature = 0.0, .humidity = 0.0, .pressure = 0, .float_val = 0.0, .on_off = 0
    },
    [SENSOR_TYPE_DS18B20] = {
            .meta = {.label= "DS18B20", .i2c_addr = 0x00, .active=false,  .status=0, .updated=false, 
            .show_bm = SENSOR_SHOW_BM_TEMP, .counter= 0, .next_meas=0},
            .temperature = 0.0, .humidity = 0.0, .pressure = 0, .float_val = 0.0, .on_off = 0
    },
    [SENSOR_TYPE_PIR] = {
            .meta = {.label= "PIR    ", .i2c_addr = 0x00, .active=false, .updated=false, 
            .show_bm = SENSOR_SHOW_BM_CNTR, .counter= 0, .next_meas=0},
            .temperature = 0.0, .humidity = 0.0, .pressure = 0, .float_val = 0.0, .on_off = 0
    },
    [SENSOR_TYPE_VEML7700] = {
            .meta = {.label= "VEML7700", .i2c_addr = I2C_ADDR_VEML7700, .active=false, .updated=false, 
            .show_bm = SENSOR_SHOW_BM_VALUE, .counter= 0, .next_meas=0},
            .temperature = 0.0, .humidity = 0.0, .pressure = 0, .float_val = 0.0, .on_off = 0
    },
};


void sensor_clear_values(uint8_t sindx)
{
    for(uint8_t i = 0; i < NBR_OF_VALUE_POINTS; i++){
        vseries[sindx].vp[i].value = 0.0;
    }
    vseries[sindx].is_ready = false;
    vseries[sindx].counter = 0;
}

void sensor_clear_all_values(void){
    for(uint8_t sindx = 0; sindx < VALUE_INDEX_NBR_OF; sindx++){
        sensor_clear_values(sindx);
    }
}

void sensor_add_value(uint8_t sindx, float value)
{
    /*
        Filter values from a number of samples (NBR_OF_VALUE_POINTS)
        1. collect all samples
        2. calculate total average
        3. drop values not within LOW_CHECK_MULTIPL..HIGH_CHECK_MULTIPL
        4. if enough valid smaples recalculate and save filtered value and set ready
        5. else save 0 and clear ready
    */
    float draft;
    float min_val;
    float max_val;
    float result;
    uint8_t valid_values;

    if((sindx < VALUE_INDEX_NBR_OF) && (!vseries[sindx].is_ready))
    {
        io_led_flash(LED_YELLOW, BLINK_JITTER_1, 20);
        if(vseries[sindx].counter >= NBR_OF_VALUE_POINTS ) {
            vseries[sindx].counter = 0;
        }
        vseries[sindx].vp[vseries[sindx].counter].value = value;
        vseries[sindx].counter++;
        if(vseries[sindx].counter >= NBR_OF_VALUE_POINTS){
            draft = 0.0;
            for(uint8_t i = 0; i < NBR_OF_VALUE_POINTS; i++){
                draft += vseries[sindx].vp[i].value;
            }
            draft = draft / NBR_OF_VALUE_POINTS;
            if(draft >= 0 ) {
                min_val = draft * LOW_CHECK_MULTIPL;
                max_val = draft * HIGH_CHECK_MULTIPL;
            }
            else {
                min_val = draft * HIGH_CHECK_MULTIPL;
                max_val = draft * LOW_CHECK_MULTIPL;
            }
            result = 0.0;
            valid_values = 0;
            for(uint8_t i = 0; i < NBR_OF_VALUE_POINTS; i++){
                if ((vseries[sindx].vp[i].value  > min_val) && (vseries[sindx].vp[i].value < max_val))
                {
                    result += vseries[sindx].vp[i].value;
                    valid_values++;
                }
            }
            if (valid_values >= MIN_NBR_OF_VALID_POINTS){
                vseries[sindx].filtered_value = result / valid_values;
                vseries[sindx].is_ready = true;
                io_led_flash(LED_YELLOW, BLINK_FAST, 80);
            }
            else {
                vseries[sindx].filtered_value =0.0;
                vseries[sindx].is_ready = false;
                io_led_flash(LED_RED, BLINK_FAST, 120);
            }
            vseries[sindx].counter = 0;
        }
    }
    // Serial.printf("sindx:%d counter:%d value:%0.2f is_ready:%d\n",
    //     sindx,
    //     vseries[sindx].counter,
    //     value,
    //     vseries[sindx].is_ready
    // );
}

void sensor_scan(void)
{
    uint8_t     address;
    uint8_t     error;
    int         devicesFound = 0;

    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.print("I2C device found at address 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            Serial.println("  !");
            devicesFound++;
        }
        else if (error == 4) {
            Serial.print("Unknown error at address 0x");
        if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
        }
    }

    if (devicesFound == 0) {
        Serial.println("No I2C devices found\n");
    } else {
        Serial.println("Scan complete\n");
    }
}


void sensor_initialize(void)
{
    uint8_t i2c_addr;
    uint8_t i2c_error;
    bool    bstatus;

    Wire.setSDA(PIN_I2C0_SDA);
    Wire.setSCL(PIN_I2C0_SCL);

    Wire.begin();
    
    sensor_scan();

    #if SENSOR_NODE == SENSOR_NODE_PIHA1
    sensor[SENSOR_TYPE_SHT31].meta.active = true;
    sensor[SENSOR_TYPE_VEML7700].meta.active = true;
    #elif ((SENSOR_NODE == SENSOR_NODE_KHH) || (SENSOR_NODE == SENSOR_NODE_TUPA1))
    sensor[SENSOR_TYPE_SHT31].meta.active = true;
    #endif


    for (uint8_t sindx = 0; sindx < SENSOR_TYPE_NBR_OF; sindx++ ){
        i2c_addr = sensor[sindx].meta.i2c_addr;
        if ((i2c_addr != 0x00) &&  (i2c_addr < 0x80)) {
            Wire.beginTransmission(i2c_addr);
            i2c_error = Wire.endTransmission();
            if (i2c_error == 0) {
                sensor[sindx].meta.active = true; 
                Serial.printf("Sensor %s active\n", sensor[sindx].meta.label);
                sensor[sindx].meta.status = SENSOR_STATUS_OK;
            } else sensor[sindx].meta.status = SENSOR_STATUS_NOT_AVAILABLE;
        }
    }
    ds18b20.begin();
    float tempC;
    for (uint8_t sindx = 0; sindx < SENSOR_TYPE_NBR_OF; sindx++ ){
        if (sensor[sindx].meta.active){
            switch (sindx){
                case SENSOR_TYPE_BMP180:
                    sensor[sindx].meta.status = bmp180.begin(I2C_ADDR_BMP180, &Wire);
                    break;
                case SENSOR_TYPE_BMP280:
                    sensor[sindx].meta.status = bmp280.begin();
                    break;
                case SENSOR_TYPE_BME680:
                    sensor[sindx].meta.status = bme680.begin();
                    if (sensor[sindx].meta.status == 0) {
                        // Set up oversampling and filter initialization
                        bme680.setTemperatureOversampling(BME680_OS_8X);
                        bme680.setHumidityOversampling(BME680_OS_2X);
                        bme680.setPressureOversampling(BME680_OS_4X);
                        bme680.setIIRFilterSize(BME680_FILTER_SIZE_3);
                        bme680.setGasHeater(320, 150); // 320*C for 150 ms
                    } 
                    break;
                case SENSOR_TYPE_AHT20:
                    sensor[sindx].meta.status = aht20.begin(&Wire);
                    break;
                case SENSOR_TYPE_SHT31:
                    bstatus = sht31.begin(sensor[SENSOR_TYPE_SHT31].meta.i2c_addr);
                    if(bstatus) sensor[sindx].meta.status = 0;
                    else sensor[sindx].meta.status = 1;
                    Serial.print("SHT31 Heater is ");
                    (sht31.isHeaterEnabled()) ? Serial.println("Enabled") : Serial.println("Disabled");
                    break;
                case SENSOR_TYPE_DS18B20:
                    ds18b20.requestTemperatures();
                    delay(1500);
                    tempC = ds18b20.getTempCByIndex(0);
                    if (tempC != DEVICE_DISCONNECTED_C){
                        sensor[sindx].meta.status = SENSOR_STATUS_OK;
                        sensor[sindx].meta.active = true;
                        ds18b20.requestTemperatures();
                    } 
                    else {
                        sensor[sindx].meta.status = SENSOR_STATUS_NOT_AVAILABLE;
                        sensor[sindx].meta.active = false;
                    }
                    break;
                case SENSOR_TYPE_PIR:
                    break;
                case SENSOR_TYPE_VEML7700:
                  if (!veml.begin()) {
                      Serial.println("VEML7700 not found");
                      while (1);
                    }
                    Serial.println("VEML7700 found");                  
                    break;
            }
        }
        if (sensor[sindx].meta.status != 0) {
            Serial.printf("Sensor failure %s\n", sensor[sindx].meta.label);
            //sensor[sindx].meta.active = false;
            sensor[sindx].meta.status = SENSOR_STATUS_READ_ERROR;
        } else sensor[sindx].meta.status = SENSOR_STATUS_OK;

    }

    uint8_t nbr_active = 0;
    for (uint8_t sindx = 0; sindx < SENSOR_TYPE_NBR_OF; sindx++ ){

        Serial.printf("!!! Sensor[%d] %s: active: %d status: %d\n", sindx, sensor[sindx].meta.label, sensor[sindx].meta.active, sensor[sindx].meta.status);
        if (sensor[sindx].meta.active){
            nbr_active++;
        }
    }
    if (nbr_active == 0){
        Serial.println("Error!!! No Active Sensors");
        main_ctrl.error.sensor = 1;
    }
    sensor_node.next_send = millis() + sensor_node.send_interval;
    atask_add_new(&sensor_handle);  
}


//   aht20.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
//   Serial.print("Temperature: "); Serial.print(temp.temperature); Serial.println(" degrees C");
//   Serial.print("Humidity: "); Serial.print(humidity.relative_humidity); Serial.println("% rH");

void sensor_print(uint8_t sindx)
{
    if((sensor[sindx].meta.show_bm & SENSOR_SHOW_BM_TEMP) != 0)
        Serial.printf("Sensor %d %s: Temperature %f\n", 
            sindx, sensor[sindx].meta.label, sensor[sindx].temperature);
    if((sensor[sindx].meta.show_bm & SENSOR_SHOW_BM_HUM) != 0)
        Serial.printf("Sensor %d %s: Humidity %f\n", 
            sindx, sensor[sindx].meta.label, sensor[sindx].humidity);
    if((sensor[sindx].meta.show_bm & SENSOR_SHOW_BM_PRESS) != 0)
        Serial.printf("Sensor %d %s: Pressure %f\n", 
            sindx, sensor[sindx].meta.label, sensor[sindx].pressure);
    if((sensor[sindx].meta.show_bm & SENSOR_SHOW_BM_VALUE) != 0)
        Serial.printf("Sensor %d %s: Value %f\n",
            sindx, sensor[sindx].meta.label, sensor[sindx].float_val);
    if((sensor[sindx].meta.show_bm & SENSOR_SHOW_BM_CNTR) != 0)
        Serial.printf("Sensor %d %s: Counter %d\n",
            sindx, sensor[sindx].meta.label, sensor[sindx].on_off);

}

void sensor_read_values(uint8_t sindx)
{
    bool read_ok = true;
    float fval;

    #if (SENSOR_NODE == SENSOR_NODE_PIHA1)
    switch(sindx) {
        case SENSOR_TYPE_SHT31:
            fval = sht31.readTemperature();
            if (isnan(fval)) read_ok = false; 
            else {
                sensor[sindx].temperature = fval;
                sensor_add_value(VALUE_INDEX_TEMPERATURE, fval);
            }
            fval = sht31.readHumidity();
            if (isnan(fval)) read_ok = false; 
            else {
                sensor[sindx].humidity = fval;
                sensor_add_value(VALUE_INDEX_HUMIDITY, fval);
            }
            break;
        case SENSOR_TYPE_VEML7700:
            sensor[sindx].float_val = veml.readLux(VEML_LUX_AUTO);
            sensor_add_value(VALUE_INDEX_LUX, sensor[sindx].float_val);
            break;
    }
    #elif ((SENSOR_NODE == SENSOR_NODE_KHH) || (SENSOR_NODE == SENSOR_NODE_TUPA1))
    switch(sindx) {
        case SENSOR_TYPE_SHT31:
            fval = sht31.readTemperature();
            if (isnan(fval)) read_ok = false; 
            else {
                sensor[sindx].temperature = fval;
                sensor_add_value(VALUE_INDEX_TEMPERATURE, fval);
            }
            fval = sht31.readHumidity();
            if (isnan(fval)) read_ok = false; 
            else {
                sensor[sindx].humidity = fval;
                sensor_add_value(VALUE_INDEX_HUMIDITY, fval);
            }
            break;
    }
    #else
    switch(sindx) {
        case SENSOR_TYPE_UNDEFINED:
            break;
        case SENSOR_TYPE_BMP180:
            sensor[sindx].temperature = bmp180.readTemperature();
            sensor[sindx].pressure = bmp180.readPressure();
            break;
        case SENSOR_TYPE_BMP280:
            sensor[sindx].temperature = bmp280.readTemperature();
            sensor[sindx].pressure = bmp280.readPressure();
            break;
        case SENSOR_TYPE_BME680:
            if (! bme680.performReading()) { 
                read_ok = false;
            }
            else {
                sensor[sindx].temperature = bme680.temperature;
                sensor[sindx].pressure = bme680.pressure / 100.0;
                sensor[sindx].humidity = bme680.humidity;
            }
            break;
        case SENSOR_TYPE_AHT20:
            sensors_event_t humidity, temp;
            aht20.getEvent(&humidity, &temp);
            sensor[sindx].humidity = humidity.relative_humidity;
            sensor[sindx].temperature = temp.temperature;
            break;
        case SENSOR_TYPE_SHT31:
            fval = sht31.readTemperature();
            if (isnan(fval)) read_ok = false; 
            else {
                sensor[sindx].temperature = fval;
                sensor_add_value(VALUE_INDEX_TEMPERATURE, fval);
            }
            fval = sht31.readHumidity();
            if (isnan(fval)) read_ok = false; 
            else {
                sensor[sindx].humidity = fval;
                sensor_add_value(VALUE_INDEX_HUMIDITY, fval);
            }
            break;
        case SENSOR_TYPE_DS18B20:
            fval = ds18b20.getTempCByIndex(0);
            if (fval != DEVICE_DISCONNECTED_C)
            {
                Serial.printf("DS18B20 Temp: %.1f\n",fval);
                sensor[sindx].temperature = fval;
            }
            else
            {
                Serial.println("Error: Could not read DS18B20");
                read_ok = false;
            }
            ds18b20.requestTemperatures();  // for next reading
            break;
        case SENSOR_TYPE_PIR:
            break;
        case SENSOR_TYPE_VEML7700:
            sensor[sindx].float_val = veml.readLux(VEML_LUX_AUTO);
            sensor_add_value(VALUE_INDEX_LUX, sensor[sindx].float_val);
            break;
    }

    #endif
    sensor[sindx].meta.next_meas = millis() + INTERVAL_READ_SENSOR;
    Serial.printf("sensor_read_values(%d) read_ok: %d\n", sindx, read_ok);
    if (read_ok) {
        sensor[sindx].meta.updated = true;
        main_ctrl.error.sensor = 0;
        // if(++sensor[sindx].meta.counter > 9999) sensor[sindx].meta.counter = 0; 
        sensor_print(sindx);  
        super_clear_cntr(SUPER_CNTR_SENSOR);
        
    }
    else {
        Serial.printf("!!! Sensor read failure: %s\n",sensor[sindx].meta.label);
        if(main_ctrl.error.sensor < 255) main_ctrl.error.sensor++;
    }

}

void sensor_pir_state_machine(void)
{
    switch (sensor_ctrl.pir_state)
    {
        case 0:
            sensor_ctrl.pir_cntr = 0;
            sensor_ctrl.pir_state = 10;
            break;
        case 10:
            if(io_pir_detected())
            sensor_ctrl.pir_state = 20;
            break;
        case 20:
            if(!io_pir_detected())
            {
                if(++sensor_ctrl.pir_cntr > 999) sensor_ctrl.pir_cntr = 0 ;
                sensor_ctrl.pir_state = 10;
                sensor_ctrl.pir_is_active = true;
            }
            break;
    }
}

bool sensor_node_send(void)
{
    bool do_send = false;
    #if(SENSOR_NODE  == SENSOR_NODE_PIHA1)
        if(millis() > sensor_node.next_send)
        {
            if(vseries[VALUE_INDEX_TEMPERATURE].is_ready &&
                vseries[VALUE_INDEX_HUMIDITY].is_ready,
                vseries[VALUE_INDEX_LUX].is_ready)
            {
                sprintf(sensor_ctrl.buff,
                    "<S;PIHA1;T;%0.1f;H;%0.0f;L;%0.0f>",
                    vseries[VALUE_INDEX_TEMPERATURE].filtered_value,
                    vseries[VALUE_INDEX_HUMIDITY].filtered_value,
                    vseries[VALUE_INDEX_LUX].filtered_value);
                do_send = true;
            }
        }
    #elif (SENSOR_NODE == SENSOR_NODE_KHH) 
        if(millis() > sensor_node.next_send)
        {
            if(vseries[VALUE_INDEX_TEMPERATURE].is_ready &&
                vseries[VALUE_INDEX_HUMIDITY].is_ready)
            {
                sprintf(sensor_ctrl.buff,
                    "<S;KHH;T;%0.1f;H;%0.0f>",
                    vseries[VALUE_INDEX_TEMPERATURE].filtered_value,
                    vseries[VALUE_INDEX_HUMIDITY].filtered_value);
                do_send = true;
            }
        }    
    #elif (SENSOR_NODE == SENSOR_NODE_TUPA1)
        if(millis() > sensor_node.next_send)
        {
            if(vseries[VALUE_INDEX_TEMPERATURE].is_ready &&
                vseries[VALUE_INDEX_HUMIDITY].is_ready)
            {
                sprintf(sensor_ctrl.buff,
                    "<S;TUPA1;T;%0.1f;H;%0.0f>",
                    vseries[VALUE_INDEX_TEMPERATURE].filtered_value,
                    vseries[VALUE_INDEX_HUMIDITY].filtered_value);
                do_send = true;
            }
        }    
    #endif

    if(do_send){
        io_led_flash(LED_BLUE, BLINK_NORMAL, 60);
        Serial.println(sensor_ctrl.buff);
        r69_send(sensor_ctrl.buff);
        sensor_node.next_send = millis() + sensor_node.send_interval;
        sensor_clear_all_values();
    }
    return do_send;
}

void sensor_task(void)
{
    // Serial.printf("State: %d, sensor_indx = %d\n", sensor_handle.state, sensor_ctrl.sensor_indx );
    switch(sensor_handle.state)
    {
        case 0:
            sensor_handle.state = 10;
            sensor_ctrl.sensor_indx = 0;
            break;
        case 10: 
            // Serial.printf("*** indx: %d - %d\n",sensor_ctrl.sensor_indx, sensor[sensor_ctrl.sensor_indx].meta.active);
            if(sensor[sensor_ctrl.sensor_indx].meta.active && (millis() > sensor[sensor_ctrl.sensor_indx].meta.next_meas)) {
                sensor_read_values(sensor_ctrl.sensor_indx);
                sensor[sensor_ctrl.sensor_indx].meta.next_meas = millis() + 10000;
            }
            sensor_handle.state = 20;
            break;
        case 20:
            if(r69_ready_to_send()){
                if (sensor_node_send());
            }
            sensor_handle.state = 100;
            break;
        case 100:
            if(sensor_ctrl.sensor_indx < SENSOR_TYPE_NBR_OF-1) sensor_ctrl.sensor_indx++;
            else sensor_ctrl.sensor_indx = 0;
            while (!sensor[sensor_ctrl.sensor_indx].meta.active){
                if(sensor_ctrl.sensor_indx < SENSOR_TYPE_NBR_OF-1) sensor_ctrl.sensor_indx++;
                 else sensor_ctrl.sensor_indx = 0;
            }
            sensor_handle.state = 10;
            break;
    }
}


void sensor_print_bmp280_data(void)
{
    // Serial.print(F("SensorID was: 0x")); Serial.println(bmp280.sensorID(),16);
    // Serial.print(F("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n"));
    // Serial.print(F("   ID of 0x56-0x58 represents a BMP 280,\n"));
    // Serial.print(F("        ID of 0x60 represents a BME 280.\n"));
    // Serial.print(F("        ID of 0x61 represents a BME 680.\n"));
}

void sensor_set_bmp280_configuration(void)
{
  /* Default settings from the datasheet. */
//   bmp280.setSampling( Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
//                       Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
//                       Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
//                       Adafruit_BMP280::FILTER_X16,      /* Filtering. */
//                       Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
}

