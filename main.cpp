/**
 * Copyright (c) 2017, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>

#include "lorawan/LoRaWANInterface.h"
#include "lorawan/system/lorawan_data_structures.h"
#include "events/EventQueue.h"

// Application helpers
#include "trace_helper.h"
#include "lora_radio_helper.h"
#include "mbed-os/platform/mbed-trace/include/mbed-trace/mbed_trace.h"
#include "agora_components.h"
#include "LSM9DS1Service.h"

using namespace events;

#define DEBUG_SENSOR_POLLING 1
#define MAX_VBAT_VOLTAGE 3.3f


// Max payload size can be LORAMAC_PHY_MAXPAYLOAD.
// This example only communicates with much shorter messages (<30 bytes).
// If longer messages are used, these buffers must be changed accordingly.
uint8_t tx_buffer[500];
uint8_t rx_buffer[500];

/*
 * Sets up an application dependent transmission timer in ms. Used only when Duty Cycling is off for testing
 */
#define TX_TIMER                        10000

/**
 * Maximum number of events for the event queue.
 * 10 is the safe number for the stack events, however, if application
 * also uses the queue for whatever purposes, this number should be increased.
 */
#define MAX_NUMBER_OF_EVENTS            10

/**
 * Maximum number of retries for CONFIRMED messages before giving up
 */
#define CONFIRMED_MSG_RETRY_COUNTER     3

/**
* This event queue is the global event queue for both the
* application and stack. To conserve memory, the stack is designed to run
* in the same thread as the application and the application is responsible for
* providing an event queue to the stack that will be used for ISR deferment as
* well as application information event queuing.
*/
static EventQueue ev_queue(MAX_NUMBER_OF_EVENTS *EVENTS_EVENT_SIZE);

/**
 * Event handler.
 *
 * This will be passed to the LoRaWAN stack to queue events for the
 * application which in turn drive the application.
 */
static void lora_event_handler(lorawan_event_t event);

/**
 * Constructing Mbed LoRaWANInterface and passing it the radio object from lora_radio_helper.
 */
static LoRaWANInterface lorawan(radio);

/**
 * Application specific callbacks
 */
static lorawan_app_callbacks_t callbacks;

//APB:
// static uint32_t DEV_ADDR = 0x;
// static uint8_t NWK_SKEY[] = {};
// static uint8_t APP_SKEY[] = {};
//OTAA: 
static uint8_t DEV_EUI[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t APP_EUI[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t APP_KEY[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  };


void init_sensors(void) {

	// Enable sensor power domain
	sensor_power_en = 1;

ThisThread::sleep_for(100ms);
	printf("Initializing sensors...\r\n");
	printf("\t BME680: ");

	if(bme680->init(&sensor_i2c)) {
		printf("OK\r\n");
	} else {
		printf("FAILED\r\n");
	}

	// No way to check this really...
	printf("\t MAX44009: OK\r\n");

	printf("\t Si7021: ");
	if(si7021.check() == 1) {
		printf("OK\r\n");
	} else {
		printf("FAILED\r\n");
	}

	printf("\t VL53L0X: ");
	if(vl53l0x.init_sensor(DEFAULT_DEVICE_ADDRESS) == 0) {
		printf("OK\r\n");
	} else {
		printf("FAILED\r\n");
	}

	printf("\t LSM9DS1: ");
	if(lsm9ds1.begin() != 0) {
		lsm9ds1.calibrate();
		printf("OK\r\n");
	} else {
		printf("FAILED\r\n");
	}

	printf("\t ICM20602: ");
	icm20602.init();
	if(icm20602.isOnline()) {
		printf("OK\r\n");
	} else {
		printf("FAILED\r\n");
	}
}

void poll_sensors(float &temperature_bme, float &pressure_bme, float &humidity_bme, float &gas_res_bme, float &co2_eq_bme, float &breath_voc_eq_bme, float &iaq_score_bme,
                    uint8_t &iaq_acc_bme,uint32_t &humidity_si, uint32_t &temp_si, float &x_accel, float &y_accel, float &z_accel, 
                    float &x_mag, float &y_mag, float &z_mag,float &x_gyro, float &y_gyro, float &z_gyro,uint32_t &distance, float &vbat) {
#if DEBUG_SENSOR_POLLING
	printf("Polling sensors...\n");
#endif

	/** Poll BME680 */
	 temperature_bme 	= bme680->get_temperature();
	 pressure_bme		= bme680->get_pressure();
	 humidity_bme		= bme680->get_humidity();
	 gas_res_bme		= bme680->get_gas_resistance();
	 co2_eq_bme		= bme680->get_co2_equivalent();
	 breath_voc_eq_bme	= bme680->get_breath_voc_equivalent();
	 iaq_score_bme		= bme680->get_iaq_score();
	 iaq_acc_bme		= bme680->get_iaq_accuracy();

#if DEBUG_SENSOR_POLLING
	printf("BME680:\n");
	printf("\ttemperature: %0.2F\n", temperature_bme);
	printf("\tpressure: %.2f\n", pressure_bme);
	printf("\thumidity: %.2f\n", humidity_bme);
	printf("\tgas resistance: %.2f\n", gas_res_bme);
	printf("\tco2 equivalent: %.2f\n", co2_eq_bme);
	printf("\tbreath voc eq: %.2f\n", breath_voc_eq_bme);
	printf("\tiaq score: %.2f\n", iaq_score_bme);
	printf("\tiaq accuracy: %i\n", iaq_acc_bme);
#endif



	/** Poll Si7021 */
	si7021.measure();
	 humidity_si = si7021.get_humidity();
	 temp_si	 = si7021.get_temperature();


#if DEBUG_SENSOR_POLLING
	printf("Si7021:\n");
	printf("\ttemperature: %lu\n", temp_si);
	printf("\thumidity: %lu\n", humidity_si);
#endif

	/** Poll VL53L0X */
	 distance = 0;
	vl53l0x.get_distance(&distance);
	if(distance == 0) {
		// 0 means the distance is too far
		// Set to infinity
		distance = 0xFFFF;
	}

#if DEBUG_SENSOR_POLLING
	printf("VL53L0X:\n");
	printf("\tdistance: %lu\n", distance);
#endif

	/** Poll LSM9DS1 */
	LSM9DS1Service::tri_axis_reading_t reading;
	lsm9ds1.readAccel();
	reading.x = lsm9ds1.calcAccel(lsm9ds1.ax);
	reading.y = lsm9ds1.calcAccel(lsm9ds1.ay);
	reading.z = lsm9ds1.calcAccel(lsm9ds1.az);
     x_accel = reading.x;
     y_accel = reading.y;
     z_accel = reading.z;

#if DEBUG_SENSOR_POLLING
	printf("LSM9DS1:\n");
	printf("\taccel: (%0.2f, %0.2f, %0.2f)\n", reading.x, reading.y, reading.z);
#endif


	lsm9ds1.readGyro();
	reading.x = lsm9ds1.calcGyro(lsm9ds1.gx);
	reading.y = lsm9ds1.calcGyro(lsm9ds1.gy);
	reading.z = lsm9ds1.calcGyro(lsm9ds1.gz);
     x_gyro = reading.x;
     y_gyro = reading.y;
     z_gyro = reading.z;

	lsm9ds1.readMag();
	reading.x = lsm9ds1.calcMag(lsm9ds1.mx);
	reading.y = lsm9ds1.calcMag(lsm9ds1.my);
	reading.z = lsm9ds1.calcMag(lsm9ds1.mz);
     x_mag = reading.x;
     y_mag = reading.y;
     z_mag = reading.z;

#if DEBUG_SENSOR_POLLING
	printf("\tgyro:  (%0.2f, %0.2f, %0.2f)\n", reading.x, reading.y, reading.z);
	printf("\tmag:   (%0.2f, %0.2f, %0.2f)\n", reading.x, reading.y, reading.z);
#endif


	/** Poll ICM20602 */
	// TODO - support for ICM20602
	//icm20602.getAccXvalue();


	/** Check battery voltage */
	battery_mon_en = 1;
	ThisThread::sleep_for(10ms);
	 vbat = battery_voltage_in.read() * MAX_VBAT_VOLTAGE * 2.0f;

#if DEBUG_SENSOR_POLLING
	printf("Battery Voltage:\n");
	printf("\tVbat: %.2f V\n", vbat);
#endif

	battery_mon_en = 0;

#if DEBUG_SENSOR_POLLING
	printf("\n");
#endif

}


/**
 * Entry point for application
 */
int main(void)
{
    init_sensors();

    // stores the status of a call to LoRaWAN protocol
    lorawan_status_t retcode;

    // Initialize LoRaWAN stack
    if (lorawan.initialize(&ev_queue) != LORAWAN_STATUS_OK) {
        printf("\r\n LoRa initialization failed! \r\n");
        return -1;
    }

    printf("\r\n Mbed LoRaWANStack initialized \r\n");

    // prepare application callbacks
    callbacks.events = mbed::callback(lora_event_handler);
    lorawan.add_app_callbacks(&callbacks);

    // Set number of retries in case of CONFIRMED messages
    if (lorawan.set_confirmed_msg_retries(CONFIRMED_MSG_RETRY_COUNTER)
            != LORAWAN_STATUS_OK) {
        printf("\r\n set_confirmed_msg_retries failed! \r\n\r\n");
        return -1;
    }

    printf("\r\n CONFIRMED message retries : %d \r\n",
           CONFIRMED_MSG_RETRY_COUNTER);

    // Enable adaptive data rate
    if (lorawan.disable_adaptive_datarate() != LORAWAN_STATUS_OK) {
        printf("\r\n disable_adaptive_datarate failed! \r\n");
        return -1;
    }

    printf("\r\n Adaptive data  rate (ADR) - Disabled \r\n");

    lorawan_connect_t connect_params;

        //OTAA:
    connect_params.connect_type = LORAWAN_CONNECTION_OTAA;
    connect_params.connection_u.otaa.dev_eui = DEV_EUI;
    connect_params.connection_u.otaa.app_eui = APP_EUI;
    connect_params.connection_u.otaa.app_key = APP_KEY;
    connect_params.connection_u.otaa.nb_trials = 3;

    //ABP:
    // connect_params.connect_type = LORAWAN_CONNECTION_ABP;
    // connect_params.connection_u.abp.dev_addr = DEV_ADDR;
    // connect_params.connection_u.abp.nwk_skey = NWK_SKEY;
    // connect_params.connection_u.abp.app_skey = APP_SKEY;

    retcode = lorawan.connect(connect_params);

    if (retcode == LORAWAN_STATUS_OK ||
            retcode == LORAWAN_STATUS_CONNECT_IN_PROGRESS) {
    } else {
        printf("\r\n Connection error, code = %d \r\n", retcode);
        return -1;
    }

    printf("\r\n Connection - In Progress ...\r\n");

    // make your event queue dispatching events forever
    ev_queue.dispatch_forever();

    return 0;
}

/**
 * Sends a message to the Network Server
 */
static void send_message()
{
    static uint8_t packet_toggle = 0;
    uint16_t packet_len;
    int16_t retcode;


    float temperature_bme = 0;
    float pressure_bme = 0;
    float humidity_bme = 0;
    float gas_res_bme = 0;
    float co2_eq_bme = 0;
    float breath_voc_eq_bme = 0;
    float iaq_score_bme = 0;
    uint8_t iaq_acc_bme = 0;
    uint32_t humidity_si = 0;
    uint32_t temp_si = 0;
    float x_accel = 0;
    float y_accel = 0;
    float z_accel = 0;
    float x_mag = 0;
    float y_mag  = 0;
    float z_mag = 0;
    float x_gyro = 0;
    float y_gyro = 0;
    float z_gyro = 0;
    uint32_t distance = 0;
    float vbat = 0;

    poll_sensors(temperature_bme, pressure_bme, humidity_bme, gas_res_bme, co2_eq_bme, breath_voc_eq_bme, iaq_score_bme,
                    iaq_acc_bme,humidity_si, temp_si, x_accel, y_accel, z_accel, 
                    x_mag, y_mag, z_mag,x_gyro, y_gyro, z_gyro,distance, vbat);

    if(!packet_toggle)
    packet_len = sprintf((char *) tx_buffer, "{\"Data_Msg_0\":{\"temp_bme\":%3.3f,\"pres_bme\":%3.3f,\"hum_bme\":%3.3f,\"gas_res_bme\":%3.3f,\"co2_eq_bme\":%3.3f,\"breath_voc_eq_bme\":%3.3f,"
                                                "\"iaq_score_bme\":%3.3f,\"iaq_acc_bme\":%d}}",temperature_bme, pressure_bme, humidity_bme, gas_res_bme, co2_eq_bme, breath_voc_eq_bme, iaq_score_bme,
                                                iaq_acc_bme);
    if(packet_toggle)
    packet_len = sprintf((char *) tx_buffer, "{\"Data_Msg_1\":{\"hum_si\":%lu,\"temp_si\":%lu,\"x_accel\":%3.3f,\"y_accel\":%3.3f,"
                                                "\"z_accel\":%3.3f,\"x_mag\":%3.3f,\"y_mag\":%3.3f,\"z_mag\":%3.3f,\"x_gyro\":%3.3f,\"y_gyro\":%3.3f,\"z_gyro\":%3.3f,"
                                                "\"dist\":%lu,\"vbat\":%3.3f}}",humidity_si, temp_si, x_accel, y_accel, z_accel, 
                                                x_mag, y_mag, z_mag,x_gyro, y_gyro, z_gyro,distance, vbat);

    packet_toggle ^= 1;

    retcode = lorawan.send(MBED_CONF_LORA_APP_PORT, tx_buffer, packet_len,
                           MSG_UNCONFIRMED_FLAG);

    if (retcode < 0) {
        retcode == LORAWAN_STATUS_WOULD_BLOCK ? printf("send - WOULD BLOCK\r\n")
        : printf("\r\n send() - Error code %d \r\n", retcode);

        if (retcode == LORAWAN_STATUS_WOULD_BLOCK) {
            //retry in 3 seconds
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                ev_queue.call_in(3000, send_message);
            }
        }
        return;
    }

    printf("\r\n %d bytes scheduled for transmission \r\n", retcode);
    memset(tx_buffer, 0, sizeof(tx_buffer));
}

/**
 * Receive a message from the Network Server
 */
static void receive_message()
{
    uint8_t port;
    int flags;
    int16_t retcode = lorawan.receive(rx_buffer, sizeof(rx_buffer), port, flags);

    if (retcode < 0) {
        printf("\r\n receive() - Error code %d \r\n", retcode);
        return;
    }

    printf(" RX Data on port %u (%d bytes): ", port, retcode);
    for (uint8_t i = 0; i < retcode; i++) {
        printf("%02x ", rx_buffer[i]);
    }
    printf("\r\n");
    
    memset(rx_buffer, 0, sizeof(rx_buffer));
}

/**
 * Event handler
 */
static void lora_event_handler(lorawan_event_t event)
{
    switch (event) {
        case CONNECTED:
            printf("\r\n Connection - Successful \r\n");
            lorawan.set_datarate(4);
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                send_message();
            } else {
                ev_queue.call_every(TX_TIMER, send_message);
            }

            break;
        case DISCONNECTED:
            ev_queue.break_dispatch();
            printf("\r\n Disconnected Successfully \r\n");
            break;
        case TX_DONE:
            printf("\r\n Message Sent to Network Server \r\n");
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                send_message();
            }
            break;
        case TX_TIMEOUT:
        case TX_ERROR:
        case TX_CRYPTO_ERROR:
        case TX_SCHEDULING_ERROR:
            printf("\r\n Transmission Error - EventCode = %d \r\n", event);
            // try again
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                send_message();
            }
            break;
        case RX_DONE:
            printf("\r\n Received message from Network Server \r\n");
            receive_message();
            break;
        case RX_TIMEOUT:
        case RX_ERROR:
            printf("\r\n Error in reception - Code = %d \r\n", event);
            break;
        case JOIN_FAILURE:
            printf("\r\n OTAA Failed - Check Keys \r\n");
            break;
        case UPLINK_REQUIRED:
            printf("\r\n Uplink required by NS \r\n");
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                send_message();
            }
            break;
        default:
            MBED_ASSERT("Unknown Event");
    }
}

// EOF
