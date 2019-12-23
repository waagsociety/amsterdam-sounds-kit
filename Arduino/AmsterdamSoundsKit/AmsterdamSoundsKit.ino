/**
 
 Amsterdam Sounds Kit dBA sound level meter for TTN.

*/

#include <lmic.h>
#include <hal/hal.h>
#include "SLM.h"
#include "LoraSettings.h"
#include "SerialDummy.h"

// Disable all serial ouput, use when device is deployed somewhere
#define DISABLE_SERIAL    1

// Send to TTN after each N seconds of measuring
#define SEND_AFTER    300

#if DISABLE_SERIAL
#define Serial      dummy
#endif

// Sound level meter
SLM slm(Serial);
static int recording = false;

// LMIC jobs to be called from os run loop.
static osjob_t sendjob;
static osjob_t restartjob;

// Event from the LMIC library, we only watch out for EV_TXCOMPLETE and then we resart. A future version should have OTAA and should not restart after every data transmission. 
void onEvent(ev_t ev)
{
  Serial.print(os_getTime());
  Serial.print(F(": "));
  switch (ev)
  {
    case EV_TXCOMPLETE:
      // Currently we just send and expect nothing back 
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
      {
        Serial.println(F("Received ack"));
      }
      if (LMIC.dataLen)
      {
        Serial.println(F("Received "));
        Serial.println(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));
      }
      // Reboot to restart measurement.
      restart(&restartjob);
      break;
  }
}

// Send data to TTN in format that matches the payload format defined in the TTN endpoint
// Transform floating point numbers from stats to q numbers to save bytes
// uQ7.1 has .5 precision and a maximum value of 127.5
// uQ5.3 has .125 precision and a maximum value of 31.875
void do_send(osjob_t *j)
{
  // Payload format
  struct {
    uint8_t mean;   //uQ7.1
    uint8_t min;    //uQ7.1
    uint8_t max;    //uQ7.1
    uint8_t stddev; //uQ5.3
    uint16_t  n;    //
  }
  data;

  // Put stats in payload format
  data.mean = (uint8_t)(slm.stats.Mean()*2);
  data.min = (uint8_t)(slm.stats.Min()*2);
  data.max = (uint8_t)(slm.stats.Max()*2);
  data.stddev = (uint8_t)(slm.stats.StdDev(true)*8);
  data.n = (uint16_t)(slm.stats.N());

  // Check if there is not a current TX/RX job running.
  if (LMIC.opmode & OP_TXRXPEND)
  {
    Serial.println(F("OP_TXRXPEND, not sending"));
  }
  else
  {
    // Prepare upstream data transmission at the next possible time.
    // Port helps us distinquish payload formats
    // Next TX is scheduled after TX_COMPLETE event.
    LMIC_setTxData2(1, (unsigned char *)&data, sizeof(data), 0);
    Serial.println(F("Packet queued"));
  }

  // Restart when request timed out
  os_setTimedCallback(&restartjob, os_getTime()+sec2osticks(120), restart);
}

// Reboot the device. After each transmission.
// This is not ideal but the I2S interrupt seems to interfere with LoRa transmission. Therefore we do: measure, send, restart .
void restart(osjob_t *j)
{
  Serial.println(F("Restart"));
  NVIC_SystemReset();
}

// Setup 
void setup()
{
  while (!Serial)
  {
  }
  Serial.begin(9600);

#ifndef CFG_eu868
  Serial.println(F("Not configured with CFG_eu86"));
#endif

  // While not using radio
  pinMode(8, OUTPUT);
  pinMode(8, HIGH);   
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Start the sound measurements
  slm.setup();
  slm.start();
  recording = true;
}

// Start up the LoRa transmission and put the send job in the LoRa os loop
void connect()
{
  // LMIC init or use os_init with global pinmap;
  os_init_ex(pPinMap);  

  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  // Next code only needed for ABP
#ifdef ABP_MODE
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession(0x01, DEVADDR, nwkskey, appskey);
  LMIC.dn2Dr = DR_SF9;
  do_send(&sendjob);
#else
  Serial.println(F("Currently only ABP_MODE supported"));
#endif
  
}

// Keep hold of current amount of seconds running
static int secs = 0;
void loop()
{
  // On every second 
  uint32_t n_secs = millis()/1000;
  if (secs != n_secs)
  {
    secs = n_secs;
    if (secs == SEND_AFTER)
    {
      slm.stop();
      recording = false;
      connect();
    }
    // Blink quickly while recording, slowly while sending
    if (recording)
    {
      digitalWrite(LED_BUILTIN, (n_secs % 2) == 0 ? HIGH : LOW);   
    }
    else
    {
      digitalWrite(LED_BUILTIN, (n_secs % 4) == 0 ? HIGH : LOW);
    }
  }
  // Call update as often as possible while recording, else call the runloop of LMIC
  if (recording)
  {
    slm.update();
  }
  else
  {
    os_runloop_once();
  }
}
