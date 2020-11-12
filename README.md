
# Example LoRaWAN application for Agora running Mbed-OS

This is an example application based on `Mbed-OS` LoRaWAN protocol APIs. The Mbed-OS LoRaWAN stack implementation is compliant with LoRaWAN v1.0.2 specification.  See this [link](https://os.mbed.com/blog/entry/Introducing-LoRaWAN-11-support/) for information on support for other LoRaWAN spec versions. This application can work with any Network Server if you have correct credentials for the said Network Server. 

## Getting Started

### Supported Hardware
[Agora](https://os.mbed.com/platforms/AGORA-DEV/)

### Example configuration and radio selection

Set channel plan and frequency in mbed_app.json.  Default channel plan works with Multitech Conduit gateway default channel setting of subband 2.

### Add network credentials
Set keys in main.cpp.  Search 'OTAA' or 'ABP' and fill in accordingly.

#### For OTAA

Please add `Device EUI`, `Application EUI` and `Application Key` needed for Over-the-air-activation(OTAA). For example:

```c
static uint8_t DEV_EUI[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t APP_EUI[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t APP_KEY[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  };

```

#### For ABP

For Activation-By-Personalization (ABP) connection method, modify the `mbed_app.json` to enable ABP. You can do it by simply turning off OTAA. For example:

```json
"lora.over-the-air-activation": false,
```

In addition to that, you need to provide `Application Session Key`, `Network Session Key` and `Device Address`. For example in main.cpp:

```c
static uint32_t DEV_ADDR = 0x;
static uint8_t NWK_SKEY[] = {};
static uint8_t APP_SKEY[] = {}; 
```

## Configuring the application

The Mbed OS LoRaWAN stack provides a lot of configuration controls to the application through the Mbed OS configuration system. The previous section discusses some of these controls. This section highlights some useful features that you can configure.

### Selecting a PHY

The LoRaWAN protocol is subject to various country specific regulations concerning radio emissions. That's why the Mbed OS LoRaWAN stack provides a `LoRaPHY` class that you can use to implement any region specific PHY layer. Currently, the Mbed OS LoRaWAN stack provides 10 different country specific implementations of `LoRaPHY` class. Selection of a specific PHY layer happens at compile time. By default, the Mbed OS LoRaWAN stack uses `EU 868 MHz` PHY. An example of selecting a PHY can be:

```josn
        "phy": {
            "help": "LoRa PHY region. 0 = EU868 (default), 1 = AS923, 2 = AU915, 3 = CN470, 4 = CN779, 5 = EU433, 6 = IN865, 7 = KR920, 8 = US915, 9 = US915_HYBRID",
            "value": "0"
        },
```

### Duty cycling

LoRaWAN v1.0.2 specifcation is exclusively duty cycle based. This application comes with duty cycle enabled by default. In other words, the Mbed OS LoRaWAN stack enforces duty cycle. The stack keeps track of transmissions on the channels in use and schedules transmissions on channels that become available in the shortest time possible. We recommend you keep duty cycle on for compliance with your country specific regulations. 

However, you can define a timer value in the application, which you can use to perform a periodic uplink when the duty cycle is turned off. Such a setup should be used only for testing or with a large enough timer value. For example:

```josn 
"target_overrides": {
	"*": {
		"lora.duty-cycle-on": false
		},
	}
}
```


## Compiling the application

Use Mbed CLI commands to generate a binary for the application.
For example:

```sh
$ mbed compile -m YOUR_TARGET -t ARM
```

## Running the application

Drag and drop the application binary from `BUILD/YOUR_TARGET/ARM/mbed-os-example-lora.bin` to your Mbed enabled target hardware, which appears as a USB device on your host machine. 

Attach a serial console emulator of your choice (for example, PuTTY, Minicom or screen) to your USB device. Set the baudrate to 115200 bit/s, and reset your board by pressing the reset button.

You should see an output similar to this:

```
Mbed LoRaWANStack initialized 

 CONFIRMED message retries : 3 

 Adaptive data  rate (ADR) - Disabled 

 Connection - In Progress ...

 Connection - Successful
Polling sensors...
BME680:
        temperature: 28.93
        pressure: 99860.00
        humidity: 32.45
        gas resistance: 13021.00
        co2 equivalent: 500.00
        breath voc eq: 0.50
        iaq score: 25.00
        iaq accuracy: 0
Si7021:
        temperature: 28837
        humidity: 33665
VL53L0X:
        distance: 305
LSM9DS1:
        accel: (0.00, -0.01, 1.04)
        gyro:  (0.05, 0.64, -0.20)
        mag:   (0.05, 0.64, -0.20)
Battery Voltage:
        Vbat: 3.31 V


 182 bytes scheduled for transmission

 Message Sent to Network Server
Polling sensors...
BME680:
        temperature: 28.94
        pressure: 99858.00
        humidity: 31.57
        gas resistance: 17542.00
        co2 equivalent: 500.00
        breath voc eq: 0.50
        iaq score: 25.00
        iaq accuracy: 0
Si7021:
        temperature: 28794
        humidity: 33176
VL53L0X:
        distance: 286
LSM9DS1:
        accel: (0.00, -0.01, 1.04)
        gyro:  (0.05, 0.66, -0.19)
        mag:   (0.05, 0.66, -0.19)
Battery Voltage:
        Vbat: 3.31 V


 210 bytes scheduled for transmission

 Message Sent to Network Server

```


### License and contributions

The software is provided under Apache-2.0 license. Contributions to this project are accepted under the same license. Please see [contributing.md](CONTRIBUTING.md) for more info.

This project contains code from other projects. The original license text is included in those source files. They must comply with our license guide.

