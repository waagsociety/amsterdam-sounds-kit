# Amsterdam Sounds kit  
**Open source (hardware) sound level meter for the internet of things.**

![alt Amsterdam Sounds Kit](./images/amsterdam-sounds-kit.jpg "Amsterdam Sounds Kit")

However the code is intended for bringing a TTN sound level meter in the air. The code can easily be adapter to make a stand alone offline device by stripping out the LoRa part of de the code.

## Electronic components assembly

* SPH0645LM4H-B on breakout board (Adafruit)
* Adafruit Feather M0 RFM95 LoRa

The least that needs to be done to get the sound level measurement part running is connecting the microphone (SPH0645LM4H) to the microcontroller. The schematic diagram assumes a Adafruit Feather M0 RFM95 LoRa is used. With other pins a Arduino Zero will also work and any other ATSAMD21 Cortex M0 based board.

<pre>
<code>
+---------+     +----------------+  
| SPH0645 |     | Feather M0     |  
+---------+     +----------------+  
| 3V      | <-> | 3V             |
| GND     | <-> | GND            |
| BCLK    | <-> | 1 / I2SCK / TX |
| DOUT    | <-> | 9 / I2SD0      |
| LRCL    | <-> | 0 / I2SF0 / RX |
| SEL     |     |                |
+---------+     | IO1            | <-+
                |                |   |
                | 6              | <-+
                +----------------+

</pre>
</code>

**N.B.**
For getting the LoRa communication running make sure to also connect pin 6 and I01 since this is used by LMIC library to control the radio module.

## Code installation

#### Adafruit SAMD boards

* Add the following URL to **Additional Boards manager URLs** in the Arduino IDE Preferences in order to be able to install the firmware for the Feather M0.  
https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
* Install **Adafruit SAMD boards** using the **Boards manager** in the Arduino IDE. Version 1.4.1 was used for this project.

#### MCCI LoRaWAN LMIC library

* Install the **MCCI LoRaWAN LMIC library** using the **Library manager** in the Arduino IDE. Version 2.3.2 was used for this project.

#### AmsterdamSoundsKit

* Download the code and open [**AmsterdamSoundsKit.ino**](./Arduino/AmsterdamSoundsKit/AmsterdamSoundsKit.ino) in the  Arduino IDE.

## Configuration

**LMIC library settings**  
lmic_project_config.h

**lora settings**

**slm settings**

**serial output**

## TTN setup

* Register

* ABP is currently used since this works quicker than OTAA when the devices resets often (as the device does, see [limitations](#limitations) ).

* Frame counter check needs to be disabled. This is a security flaw, but the microcontroller can only use its flash as persistent memory, so keeping frame counts on the device is not ideal.

## Enclosure

**3D printable microphone mount**  
...

**Acoustic vent**  
...


## Code documentation

Code documentation is mainly written in the comments of the code.

## Limitations / known issues

**timing**  
LoRa timing is critical. Currently the SLM cannot read en process audio and do the LoRa communication at the same time.

**data rate**  
LoRa has limited bandwidth for data transfer. Therefore the sensor only sends averages and statistics over periods of time (default minutely).

### Tools

**bin scale table**  
...

### Parts list

### References

1. Project homepage  
https://amsterdamsounds.waag.org
1. Design of a Sound Level Meter, Douglas Lanman  
https://alumni.media.mit.edu/~dlanman/courses/decibel_meter.pdf  
1. CMSIS-DSP library reference  
https://arm-software.github.io/CMSIS_5/DSP/html/modules.html
1. Arduino IBM LMIC (LoRaWAN-MAC-in-C) library  
https://github.com/mcci-catena/arduino-lmic
1. Datasheet of digital MEMS microphone (SPH0645LM4H)  
https://cdn-shop.adafruit.com/product-files/3421/i2S+Datasheet.PDF
1. Microphone on breakout board  
https://www.adafruit.com/product/3421
1. Feather M0 with integrated LoRa radio for European frequency plan (EU863-870)  
https://www.adafruit.com/product/3178
1. Acoustic vents for sealing the microphone hole  
https://www.alibaba.com/product-detail/Black-Custom-Size-Waterproof-IP-67_62167248114.html
1. Standard size enclosure (115mm x 65mm x 40mm)  
https://www.reichelt.nl/kunststof-behuizing-115-x-65-x-40-lichtgrijs-rnd-455-00182-p193396.html
