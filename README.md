# Amsterdam Sounds kit  
**Open source (hardware) sound level meter for the internet of things.**

![alt Amsterdam Sounds Kit](./images/amsterdam-sounds-kit.jpg "Amsterdam Sounds Kit")

However the code is intended for bringing a TTN sound level meter in the air. The code can easily be adapted to make a stand alone offline device by stripping out the LoRa part of de the code.

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
                | Ant.           | <------------------------
                +----------------+

</pre>
</code>

**N.B.**
* For getting the LoRa communication running make sure to also connect pin 6 and I01 since this is used by LMIC library to control the radio module.
* A solid wire can be use as an antenna. [Look up](https://learn.adafruit.com/adafruit-feather-m0-radio-with-lora-radio-module/antenna-options) which length is needed for which LoRa frequency.

## Code installation

#### Adafruit SAMD boards

* Add the following URL to **Additional Boards manager URLs** in the Arduino IDE Preferences in order to be able to install the firmware for the Feather M0.  
https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
* Install **Adafruit SAMD boards** using the **Boards manager** in the Arduino IDE. Version 1.4.1 was used for this project.

#### MCCI LoRaWAN LMIC library

* Install the **MCCI LoRaWAN LMIC library** using the **Library manager** in the Arduino IDE. Version 2.3.2 was used for this project.

#### AmsterdamSoundsKit

* Download the code and open [**AmsterdamSoundsKit.ino**]([./Arduino/AmsterdamSoundsKit/AmsterdamSoundsKit.ino]) in the  Arduino IDE.

## Configuration

#### General settings

* *SEND_AFTER*  
Defines at which rate the device sends updates to The Things Network. Keep in mind that there is a limited data rate for the TTN (see [maximum duty cycle](https://www.thethingsnetwork.org/docs/lorawan/duty-cycle.html#maximum-duty-cycle)). The devices sends a summary over the specified period (see [limitations](#limitations--known-issues)).
* *DISABLE_SERIAL*  
Setting this to 1 disables all serial communication. This is recommended to do when deploying the device to stand-alone.

Snippet from [*AmsterdamSoundsKit.ino*](./Arduino/AmsterdamSoundsKit/AmsterdamSoundsKit.ino)

<pre><code>
// Disable all serial ouput, use when device is deployed somewhere
#define DISABLE_SERIAL    1

// Send to TTN after each N seconds of measuring
#define SEND_AFTER    300
</pre></code>

#### LoRa settings

Currently only ABP is implemented. The device address and keys have to be set to the ones generated in the TTN console.

Snippet from [*LoraSettings.h*](./Arduino/AmsterdamSoundsKit/LoraSettings.h)

<pre><code>
static const PROGMEM u1_t NWKSKEY[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const u1_t PROGMEM APPSKEY[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const u4_t DEVADDR = 0x00000000;
</pre></code>

* *NWKSKEY*  
16 byte **Network Session Key** to be copied from device overview in TTN console.

* *APPSKEY*  
16 byte **App Session Key** to be copied from device overview in TTN console.

* *DEVADDR*  
4 byte **Device Address** to be copied from device overview in TTN console.

#### SLM settings

Snippet from [*SLMSettings.h*](./Arduino/AmsterdamSoundsKit/SLMSettings.h)

<pre><code>
// Current mode
#define SLM_MODE SLM_MODE_DEBUG
</pre></code>

* *SLM_MODE*  
Defines if and how the device outputs to the serial port, allowed values:
 * *SLM_MODE_NORMAL*  
 The SLM part of the code does not ouput at all. Use when deploying the device to stand-alone situation.
 * *SLM_MODE_STREAM_AUDIO*  
 The SLM streams the raw audio data from the microphone to serial port converting the 18 bit samples to 32 bit. This allows for capturing the audio to disk or listening to it to make sure it is OK.  
 <pre><code>
 # capture audio to disk (unix/linux)
 cat /dev/cu.usbmodem14111 > audio.raw
 # listen to audio (install mplayer command line utility)
cat /dev/cu.usbmodem14111 | mplayer -rawaudio rate=48000:channels=1:samplesize=4 -demuxer rawaudio -cache 1024 -
 </pre></code>
 * *SLM_MODE_STREAM_FFT*  
 The SLM outputs the raw ouput of the FFT to the serial port. Can be used in combination with the [*SpectrumPlotter*](./Arduino/AmsterdamSoundsKit/Tools/SpectrumPlotter/SpectrumPlotter.pde) Processing tool. Note, conversion from fixed point numbers to float is done in Processing.
 * *SLM_MODE_OUTPUT*  
 The SLM outputs live dBA levels in a binary format. Intended to use with [*SPLDisplay*]((./Arduino/AmsterdamSoundsKit/Tools/SpectrumPlotter/SPLDisplay.pde) Processing tool.
 * *SLM_MODE_DEBUG*  
 The SLM writes live dBA levels in readable format to the serial port.
 * *SLM_MODE_PROFILE*  
 The SLM outputs the amount of time it consumes for doing the audio analysis.


* *Other settings*  
These settings are probably good to go. They allow you to tweak how the audio is analyzed. Current settings make the device capture audio at 48000hz and do a FFT analysis over 1024 samples of audio at a rate of 32 times per second. Changing these settings could lead to performance or memory issues. For example, longer FFT requires more processing time and longer buffers. Secondly the  scaling table for correcting the frequency response of the microphone and doing dBA weighting is precalculated for a specific FFT size. Changing this would require precalculating a new table ([*EQ.h*](./Arduino/AmsterdamSoundsKit/)).

#### LMIC library settings  

Make sure to set the correct LoRa network plan in *lmic_project_config.h*. This file typically resides in the *libraries/MCCI_LoRaWAN_LMIC_library/project_config* folder in your Arduino projects folder.  
The following settings are an example for use of the European TTN plan.

<pre><code>
#define CFG_eu868
#define LMIC_DEBUG_LEVEL 0
#define CFG_sx1276_radio 1
#define ARDUINO_SAMD_FEATHER_M0
#define DISABLE_BEACONS
#define DISABLE_PING
#define DISABLE_JOIN
</pre></code>

## TTN setup

In order to be able to send data to The Things Network you have to make an account, [register](https://www.thethingsnetwork.org/docs/applications/add.html) an *application* and [add](https://www.thethingsnetwork.org/docs/devices/registration.html) *devices* to that application.
The application is your sandbox within TTN that manages all the devices registered to the application. N.B. currently the Arduino code only supports ABP devices, no OTAA yet.

#### Application setings

Apart from having to register individual devices nothing much has to be done here, except for setting the payload format. The sensor sends each frame of data in particular format to save bandwidth. The format at the sending end has to match the format at the receiving end.

**Payload format**

The following code decodes the bytes received from the sensor. It mainly converts fixed point numbers to floating point numbers.  
**The following code must be pasted** in the *decoder* tab of your application its payload formats. The other tabs (*converter*, *validator* and *encoder*) can be left empty.

<pre><code>
// For example, an uQ7.1 is an unsigned number with 7 bits for the integer part and 1 bit for the fractional part.
// As such it has a range of [0 - 127.5].
function Decoder(bytes, port) {
  var mean = bytes[0]/2.0; //convert from uQ7.1
  var min = bytes[1]/2.0; //convert from uQ7.1
  var max = bytes[2]/2.0; //convert from uQ7.1
  var stddev = bytes[3]/8.0; //convert from uQ5.3
  var n = (bytes[5] << 8) | bytes[4];
  return {
    mean: mean,
    min: min,
    max: max,
    stddev: stddev,
    n: n
  };
}
</pre></code>

#### Device settings

For each device added to the application the following settings have to be applied in the settings tab of the device:

* *Activation Method* : ABP  
 ABP is currently used since this has less overhead than OTAA which is preferable wnen the devices resets often (as the device does, see [limitations](#limitations--known-issues) ).

* *Frame Counter Checks* : Off  
 These need to be disabled. This is a security flaw, but the microcontroller can only use its flash as persistent memory, so keeping frame counts on the device is not ideal.

## Tools

The following tools can be found in the repository:

* [*precalculate_bin_scale_table*](./Arduino/AmsterdamSoundsKit/Tools/PrecalculateBinScaleTable/precalculate_bin_scale_table.rb)  
A Ruby script that generates the table in [*EQ.h*](./Arduino/AmsterdamSoundsKit/). It uses the frequency response of the microphone as input and a formula for calculating the frequency response of the dBA weighting. The microphone response was extracted from the datasheet.
* [*SPLDisplay*](./Arduino/AmsterdamSoundsKit/Tools/SPLDisplay/SPLDisplay.pde)  
A Processing sketch that reads data from the sensor in order to display it nicely. ([SLM settings](#slm-settings)).
* [*SpectrumPlotter*](./Arduino/AmsterdamSoundsKit/Tools/SPLDisplay/SPLDisplay.pde)  
A Processing sketch that reads raw FFT output data from the sensor in order to display it ([SLM settings](#slm-settings)).

## Enclosure

The bare components can be put in a custom housing for outdoor use.
Within the Amsterdam Sounds project a standard sized box (115mm x 65mm x 40mm) is used for containing the microprocessor and the antenna. The microphone placement is a little more difficult. It must be outside, but protected from the weather.

#### 3D printable microphone mount  
A custom 3D printable mount was designed for placing the microphone outside of the box and as a base for a standard sizes microphone windshield. The mount consists of [*two parts*](./Arduino/AmsterdamSoundsKit/Enclosure) that can be printed with a 3D printer or ordered via an online printing service.
![alt Amsterdam Sounds Kit microphone mount](./images/mount.jpg "Amsterdam Sounds Kit microphone mount")

#### Acoustic vent  
Sadly this is the only part that is not easily available. But..



## Code documentation

Code documentation is mainly written in the comments of the code.

## Limitations / known issues

**timing**  
LoRa timing is critical. Currently the SLM cannot read en process audio and do the LoRa communication at the same time.

**data rate**  
LoRa has limited bandwidth for data transfer. Therefore the sensor only sends averages and statistics over periods of time (default minutely).

## Considerations



### Parts list

...

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
