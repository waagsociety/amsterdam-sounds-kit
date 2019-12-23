/*

 LORA region configuration can be found here : Arduino/libraries/MCCI_LoRaWAN_LMIC_library/project_config 
 Currently uses ABP mode only. OTAA still needs some work.
 
*/

#include <arduino_lmic_hal_boards.h>

#define ABP_MODE
#define ARDUINO_SAMD_FEATHER_M0 1

#ifdef ABP_MODE
 
  // Device slm_6 specific keys and id,  keys all big endian (same order as in TTN console
  static const PROGMEM u1_t NWKSKEY[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  static const u1_t PROGMEM APPSKEY[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  static const u4_t DEVADDR = 0x00000000;

  // Empty callbacks for ABP
  void os_getArtEui (u1_t* buf) { }
  void os_getDevEui (u1_t* buf) { }
  void os_getDevKey (u1_t* buf) { }
#else

  
#endif

// Schedule TX every this many seconds (duty cycle limitations should be calculated depending on send data size).
const unsigned TX_INTERVAL = 120;

// Pin mapping for AdaFruit Feather M0
const lmic_pinmap *pPinMap = Arduino_LMIC::GetPinmap_FeatherM0LoRa();
