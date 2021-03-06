#include "lora_demo.h"
#include "RHF76.h"
#include <Math.h>
#include "bsp.h"

/*
 ==================================================================================
 data template:

 Type       Name            Token       DataType    RW          Attribute
 property   temperature     temperature integer     readonly    range: [-100, 155]
                                                                initial: 0
                                                                step: 1
                                                                unit: centigrade

 property   humidity        humidity    integer     readonly    range: [-0, 100]
                                                                initial: 0
                                                                step: 1
                                                                unit: %

 property   report_period   period      integer     read-write  range: [0, 3600]
                                                                initial: 0
                                                                step: 1
                                                                unit: second

 ==================================================================================
 up-link parser javascript:

 function RawToProtocol(fPort, bytes) {
  var data = {
      "method": "report",
      "clientToken" : new Date(),
      "params" : {}
  };
  data.params.temperature = bytes[0];
  data.params.humidity = bytes[1];
  data.params.period = bytes[2] | (bytes[3] << 8);
  return data;
 }

 ==================================================================================
 down-link parser javascript:

 function ProtocolToRaw(obj) {
     var data = new Array();
     data[0] = 5;// fport=5
     data[1] = 0;// unconfirmed mode
     data[2] = obj.params.period & 0x00FF;
     data[3] = (obj.params.period >> 8) & 0x00FF;
     return data;
 }

 */

uint16_t report_period = 10;

typedef struct device_data_st {
    uint8_t     magn_fullscale;                // fullscale of magnetometer
    uint8_t     temp_sensitivity;
    uint16_t    humi_sensitivity;
    uint16_t    press_sensitivity;
    uint16_t    magn_sensitivity;              // sensitivity per fullscale
    int16_t     temperature;
    int16_t     humidity;  
    int16_t     magn_x;                       // X-magnetic value in LSB
    int16_t     magn_y;                       // Y-magnetic value in LSB
    int16_t     magn_z;                       // Z-magnetic value in LSB
    uint16_t    period;
    uint32_t    pressure;
} __PACKED__ dev_data_t;

typedef struct device_data_wrapper_st {
    union {
        dev_data_t  dev_data;
        uint8_t     serialize[sizeof(dev_data_t)];
    } u;
} dev_data_wrapper_t;

dev_data_wrapper_t dev_data_wrapper;

void recv_callback(uint8_t *data, uint8_t len)
{
    int i = 0;

    printf("len: %d\n", len);

    for (i = 0; i < len; ++i) {
        printf("data[%d]: %d\n", i, data[i]);
    }

    if (len == 1) {
        report_period = data[0];
    } else if (len >= 2) {
        report_period = data[0] | (data[1] << 8);
    }
    printf("report_period: %d\n", report_period);
}

void print_to_screen(sensor_data_t sensor_data)
{
  float pressure = sensor_data.sensor_press.pressure*1.0/sensor_data.sensor_press.sensitivity;
  float altitude = (pow(1013.25/pressure,1.0/5.257) - 1)*((int16_t)sensor_data.sensor_tempnhumi.temperature/10.0+273.15)/0.0065;
  printf("temperature: %2.2f\n", (int16_t)sensor_data.sensor_tempnhumi.temperature / 10.0);
  printf("humidity   : %2.2f\n", sensor_data.sensor_tempnhumi.humidity / 10.0);
  printf("pressure   : %2.2f,\t altitude: %2.2f\n", pressure, altitude);
  printf("magn       : %2.3f, %2.3f, %2.3f\n", 
               (int16_t)sensor_data.sensor_magn.magn_x*1.0/sensor_data.sensor_magn.sensitivity, 
               (int16_t)sensor_data.sensor_magn.magn_y*1.0/sensor_data.sensor_magn.sensitivity, 
               (int16_t)sensor_data.sensor_magn.magn_z*1.0/sensor_data.sensor_magn.sensitivity);
}

/**
 * @brief     application entry
 * @modified  by jieranzhi 2020/03/31
 * @note      following javascript code snippet demonstrate how to correctly 
 *            decode the data passed to Tencent cloud
 ****************************** CODE START *****************************  
              function RawToProtocol(fPort, bytes) 
              {
                var data ={
                        "method": "report",
                        "clientToken": new Date(),
                        "params": {}
                }
                var magnFullscale   = bytes[0];
                switch(magnFullscale)
                {
                        case 0:
                                data.params.magnFullscale = 4; 
                        break;
                        case 1:
                                data.params.magnFullscale = 8; 
                        break;
                        case 2:
                                data.params.magnFullscale = 12; 
                        break;
                        case 3:
                                data.params.magnFullscale = 16; 
                        break;
                }
                var tempSensitivity = bytes[1];
                var humiSensitivity = bytes[2] | (bytes[3]<<8);
                var presSensitivity = bytes[4] | (bytes[5]<<8);
                var magnSensitivity = bytes[6] | (bytes[7]<<8);

                data.params.temperature   = (convertToInt16(bytes[8] | (bytes[9]<<8))*1.0/10).toFixed(2);
                data.params.humidity      = ((bytes[10] | (bytes[11]<<8))*1.0/10).toFixed(2);
                data.params.magnX         = (convertToInt16(bytes[12] | (bytes[13]<<8))*1.0/magnSensitivity).toFixed(2);
                data.params.magnY         = (convertToInt16(bytes[14] | (bytes[15]<<8))*1.0/magnSensitivity).toFixed(2);
                data.params.magnZ         = (convertToInt16(bytes[16] | (bytes[17]<<8))*1.0/magnSensitivity).toFixed(2);
                data.params.period        = bytes[18] | (bytes[19]<<8);
                data.params.pressure      = ((bytes[20] | (bytes[21]<<8) | (bytes[22]<<16) | (bytes[23]<<24))*1.0/presSensitivity).toFixed(2);
                data.params.altitude      = ((Math.pow(1017.92/data.params.pressure,1.0/5.257) - 1)*(data.params.temperature/10.0+273.15)/0.0065).toFixed(2);
                data.params.fPort         = fPort;
                return data;
             } 

            function convertToInt16(num)
            {
                    var intNum = num;
                    if ((num & 0x8000) > 0) {
                            intNum = num - 0x10000;
                    }
                    return intNum;
            }
 ****************************** CODE END ***************************** 
 */
void application_entry(void *arg)
{
    sensor_data_t sensor_data;

    // initialization of sensors
    BSP_Sensor_Init();

    rhf76_lora_init(HAL_UART_PORT_1);
    tos_lora_module_recvcb_register(recv_callback);
    tos_lora_module_join_otaa("8cf957200000f53c", "8cf957200000f52c6d09aaaaad205a72");

    while (1) {
        BSP_Sensor_Read(&sensor_data);
        print_to_screen(sensor_data);
        // generate data frame
        dev_data_wrapper.u.dev_data.magn_fullscale    = (uint8_t)(sensor_data.sensor_magn.fullscale);
        dev_data_wrapper.u.dev_data.temp_sensitivity  = (uint8_t)(sensor_data.sensor_tempnhumi.temp_sensitivity);
        dev_data_wrapper.u.dev_data.humi_sensitivity  = (uint16_t)(sensor_data.sensor_tempnhumi.humi_sensitivity);
        dev_data_wrapper.u.dev_data.press_sensitivity = (uint16_t)(sensor_data.sensor_press.sensitivity);
        dev_data_wrapper.u.dev_data.magn_sensitivity  = (uint16_t)(sensor_data.sensor_magn.sensitivity);
        dev_data_wrapper.u.dev_data.temperature       = (int16_t)(sensor_data.sensor_tempnhumi.temperature);
        dev_data_wrapper.u.dev_data.humidity          = (int16_t)(sensor_data.sensor_tempnhumi.humidity);
        dev_data_wrapper.u.dev_data.magn_x            = (int16_t)(sensor_data.sensor_magn.magn_x);
        dev_data_wrapper.u.dev_data.magn_y            = (int16_t)(sensor_data.sensor_magn.magn_y);
        dev_data_wrapper.u.dev_data.magn_z            = (int16_t)(sensor_data.sensor_magn.magn_z);
        dev_data_wrapper.u.dev_data.pressure          = (uint32_t)(sensor_data.sensor_press.pressure);
        dev_data_wrapper.u.dev_data.period            = report_period;
        // send data to the server (via gateway)
        tos_lora_module_send(dev_data_wrapper.u.serialize, sizeof(dev_data_t));
        tos_task_delay(report_period * 1000);
    }
}

