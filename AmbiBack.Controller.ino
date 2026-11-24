
// --- Include all libraries
#include "globalincludes.h"
#include "FastLED.h"
#include <WiFi.h>

#if defined(ENABLE_WIFI_AP)
# include <WiFiAP.h>
#endif

// Unofficial API to read Voltage VDD
extern "C" int rom_phy_get_vdd33();

// --- Include all classes 
#include "eepromhelper.h"
#if defined(ENABLE_IR)
# include "irhelper.h"
#endif 
#if defined(ENABLE_TCP)
#include "tcpserver.h"
#endif 
#include "udpserver.h"
#if defined(ENABLE_SERIAL_INPUT)
# include "serialhelper.h"
#endif 
// Main WebServer now!
#include "ambiwebserver.h"
#if defined(ENABLE_CAM)
# include "camprocessor.h"
#endif

#include <rom/rtc.h>
#include "esp_wps.h"
#include "esp_err.h"
#include <esp_wifi.h>
#include "esp_wifi_types.h"
#include "esp_heap_caps.h"
#include "esp32-hal-cpu.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#if defined(CONFIG_BT_ENABLED)
# include "esp_bt.h"
# include "esp_bt_main.h"
# if defined(DISABLE_BLUETOOTH)
# endif
#endif

#if defined(ESP32_EXTRA_HEAP_TRACING)
# include "esp_heap_trace.h"
# define NUM_RECORDS 100
static heap_trace_record_t trace_record[NUM_RECORDS]; // This buffer must be in internal RAM
#endif

// WPS Helper defines 
  static esp_wps_config_t esp32wps_config;
/*
Change the definition of the WPS mode
from WPS_TYPE_PBC to WPS_TYPE_PIN in
the case that you are using pin type
WPS
*/
#   define ESP_WPS_MODE      WPS_TYPE_PBC
#   define ESP_MANUFACTURER  "ESPRESSIF"
#   define ESP_MODEL_NUMBER  "ESP32"
#   define ESP_MODEL_NAME    "ESPRESSIF IOT"
#   define ESP_DEVICE_NAME   "ESP STATION"

#   ifdef ENABLE_SSD1306
#     include "SSD1306.h"
#     include "OLEDDisplayUi.h"
#     define SSD1306_ADDRESS 0x3c
SSD1306 oled(SSD1306_ADDRESS, I2C_SDA, I2C_SCL, SSD130_MODLE_TYPE);
OLEDDisplayUi ui(&oled);
#   endif

typedef struct {
    int err;
    const char *reason;
} wifi_reason_t;
static const wifi_reason_t wifi_reason[] =
{
    {0,                                    PSTR("other reason")},
    {WIFI_REASON_UNSPECIFIED,              PSTR("unspecified")},
    {WIFI_REASON_AUTH_EXPIRE,              PSTR("auth expire")},
    {WIFI_REASON_AUTH_LEAVE,               PSTR("auth leave")},
    {WIFI_REASON_ASSOC_EXPIRE,             PSTR("assoc expire")},
    {WIFI_REASON_ASSOC_TOOMANY,            PSTR("assoc too many")},
    {WIFI_REASON_NOT_AUTHED,               PSTR("not authed")},
    {WIFI_REASON_NOT_ASSOCED,              PSTR("not assoced")},
    {WIFI_REASON_ASSOC_LEAVE,              PSTR("assoc leave")},
    {WIFI_REASON_ASSOC_NOT_AUTHED,         PSTR("assoc not authed")},
    {WIFI_REASON_BEACON_TIMEOUT,           PSTR("beacon timeout")},
    {WIFI_REASON_NO_AP_FOUND,              PSTR("no ap found")},
    {WIFI_REASON_AUTH_FAIL,                PSTR("auth fail")},
    {WIFI_REASON_ASSOC_FAIL,               PSTR("assoc fail")},
    {WIFI_REASON_HANDSHAKE_TIMEOUT,        PSTR("hanshake timeout")},
    {WIFI_REASON_DISASSOC_PWRCAP_BAD,      PSTR("bad Power Capability, disassoc")},
    {WIFI_REASON_DISASSOC_SUPCHAN_BAD,     PSTR("bad Supported Channels, disassoc")},
    {WIFI_REASON_IE_INVALID,               PSTR("invalid IE")},
    {WIFI_REASON_MIC_FAILURE,              PSTR("MIC failure")},
    {WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT,   PSTR("4-way keying handshake timeout")},
    {WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT, PSTR("Group key handshake")},
    {WIFI_REASON_IE_IN_4WAY_DIFFERS,       PSTR("IE in 4-way differs")},
    {WIFI_REASON_GROUP_CIPHER_INVALID,     PSTR("invalid group cipher")},
    {WIFI_REASON_PAIRWISE_CIPHER_INVALID,  PSTR("invalid pairwise cipher")},
    {WIFI_REASON_AKMP_INVALID,             PSTR("invalid AKMP")},
    {WIFI_REASON_UNSUPP_RSN_IE_VERSION,    PSTR("unsupported RSN IE version")},
    {WIFI_REASON_INVALID_RSN_IE_CAP,       PSTR("invalid RSN IE capability")},
    {WIFI_REASON_802_1X_AUTH_FAILED,       PSTR("802.1x auth failed")},
    {WIFI_REASON_CIPHER_SUITE_REJECTED,    PSTR("cipher suite rejected")}
};

static const char* wifi_disconnect_reason_to_str(int err)
{
    for (int i=0; i< sizeof(wifi_reason)/sizeof(wifi_reason[0]); i++){
        if (err == wifi_reason[i].err){
            return wifi_reason[i].reason;
        }
    }
    return wifi_reason[0].reason;
}

// --- Create static pointers to objects
EEPROMHelper* pEepromHelper = NULL; // Main EEPROM Helper
LEDHelper* pLedhelper = NULL;       // Main LED Helper

// --- Create all static class objects
#ifdef ENABLE_IR
IRHelper irhelper;          // Main IR Receiver  
#endif 
UdpServer udpServer(UDP_PORT);  // Default UdpServer for Broadcasts
#if defined(ENABLE_TCP)
TcpServer tcpServer(TCP_PORT);  // Default TcpServer for LED sync
#endif 
#if defined(ENABLE_SERIAL_INPUT)
SerialHelper serialHelper;    // Default SerialHelper
#endif 

AmbiWebServer webServer(HTTP_PORT); // Default WebServer
// ---

// --- prototypes defined 
#ifdef ENABLE_IR
void HandleIR();
#endif
#if defined(ESP32)
void wpsInitConfig();
String wpsPin2string(uint8_t a[]);

# if defined(ARDUINO_ESP32_RELEASE_2_0_0) || (defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR == 2)
void handleWiFiEvents(arduino_event_id_t event, arduino_event_info_t info);
# elif defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void handleWiFiEvents(arduino_event_id_t event, arduino_event_info_t info);
# else
void handleWiFiEvents(WiFiEvent_t event, system_event_info_t info);
# endif
const char* esp32GetRebootRsnStr(byte bCore, RESET_REASON reason);

#endif
// ---

// --- Main SETUP 
void setup() {
  // Error Handler return value
  esp_err_t ret;

#if defined(ESP32_DEBUG_FREEMEM)
  global::printEspMemUsage("setup", "begin"); 
#endif

  // Begin Serial for debugging right now
  Serial.end(); 
  Serial.begin(DEFAULT_SERIAL_RATE);
  Serial.setRxBufferSize(2048);

  // at the very first step, we init the WIFI Mode!
  global::HandleWifiMode();

  // Init previous static objects dynamic now!
  pEepromHelper = new EEPROMHelper(); 
  pLedhelper = new LEDHelper();

#if defined(ESP32_DEBUG_FREEMEM)
  global::printEspMemUsage("setup", "LEDHelper"); 
#endif
#if defined(ESP32_DISABLED_BROWNOUT)
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // !Important, disable brownout detector!
#endif

    // Some first debug output
  _DBGPRINTF(PSTR(PSTR_SETUP "ENTER on Core %d\n"), xPortGetCoreID());
  _DBGPRINTF(PSTR(PSTR_SETUP "Reset Reason Core 0 %s\n"), esp32GetRebootRsnStr(0, rtc_get_reset_reason(0)));
  _DBGPRINTF(PSTR(PSTR_SETUP "Reset Reason Core 1 %s\n"), esp32GetRebootRsnStr(1, rtc_get_reset_reason(1)));
  _DBGPRINTF(PSTR(PSTR_SETUP "SetCpuFrequency to 240 Mhz = %s\n"), (setCpuFrequencyMhz(240) ? PSTR_SUCCESS : PSTR_FAILED) );
  _DBGPRINTF(PSTR(PSTR_SETUP "Serialrate: %d\n"), DEFAULT_SERIAL_RATE);
  _DBGPRINTF(PSTR(PSTR_SETUP "EEPROM Length: %d\n"), pEepromHelper->GetEepromSize());

    // Handle WIFI Stuff
# if defined(ARDUINO_ESP32_RELEASE_2_0_0) || (defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR == 2)
  WiFi.onEvent([](arduino_event_t *event) {
    handleWiFiEvents(event->event_id, event->event_info);
  }); // Handle WIFI events (ESP32 2.0.x - uses WiFiEventSysCb)
# elif defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t info) {
    handleWiFiEvents(event, info);
  }); // Handle WIFI events (ESP32 3.x)
# else
  WiFi.onEvent(handleWiFiEvents); // Handle WIFI events (ESP32 1.x)
# endif
  WiFi.setHostname(NETWORK_NAME); // Set hostname
  wpsInitConfig();                // Init WPS Config object

  // Disabled Power LED
  global::TogglePwrLedOff(); 

  // Init Global Object first and assign helpers!
  global::Init();
#if defined(ESP32_DEBUG_FREEMEM)
  global::printEspMemUsage("setup", "init"); 
#endif  

  // Set references back
  global::SetLedHelper(pLedhelper);
  global::SetEepromHelper(pEepromHelper);
  global::SetWebServerHelper(&webServer);
#if defined(ENABLE_CAM)
  global::m_pCamProcessor = new CamProcessor();
#endif 
  
#define ENABLE_ESP32_DETAILS
#if defined(ENABLE_ESP32_DETAILS)
# define ESP32_DETAIL_PRINTFSTR ", Chip: %s, Rev: %d"
#else
# define ESP32_DETAIL_PRINTFSTR ""
#endif
  uint32_t chipId = 0;
  for(int i=0; i<17; i=i+8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  _DBGPRINTF(PSTR(PSTR_SETUP "ESP32 EfuseMac: %#04x, FlashChipSize: %d, FreeSketchSpace: %d, SdkVersion: %d" ESP32_DETAIL_PRINTFSTR "\n" ),
    chipId,
    ESP.getFlashChipSize(),
    ESP.getFreeSketchSpace(), 
    ESP.getSdkVersion() 
#if defined(ENABLE_ESP32_DETAILS)
    ,ESP.getChipModel()
    ,ESP.getChipRevision()
#endif    
  );
  _DBGPRINTF(PSTR(PSTR_SETUP "debug: PWRLEDPIN %d, CTLLEDPIN %d, BTNPIN %d, IRRECVPIN %d, LEDPIN1 %d, LEDPIN2 %d\n"), PWRLED_PIN, CTLLED_PIN, BTN_PIN, IRRECV_PIN, LEDCTRL_PIN, LEDCTRL_PIN_SEC);

  // Init Settings from EEPROM
  pEepromHelper->LoadSettings(); 
  _DBGPRINTF(PSTR(PSTR_SETUP "debug: DeviceID: '%s', DeviceName: '%s', Version: '%s'\n"), global::GetDeviceID(), global::GetDeviceName(), AMBIBACK_VERSION);

#if defined(ENABLE_WIFI_AP)
  // Open the setup window only when the device needs it or the user asked for it.
  global::HandleProvisioningBoot();
  if (global::m_pWebServer != NULL) {
    global::m_pWebServer->StartServer();
  }
#endif

  // Init FastLed and stuff in helper
  pLedhelper->InitLeds();
  // Initial LED Test
  pLedhelper->LedDemoMode(255, LED_DEMO_DEFAULT, 500);

#ifdef ENABLE_IR
  // Enable IR Receiver!
  irhelper.enableIrIn();
#endif

#if defined(ESP32_DEBUG_FREEMEM)
  global::printEspMemUsage("setup", "init led"); 
#endif

#if defined(ENABLE_HUE)
  global::InitHue();
# if defined(ESP32_DEBUG_FREEMEM)
  global::printEspMemUsage("setup", "init hue"); 
# endif
#endif
#if defined(ENABLE_DREAMSCREEN)
  global::InitDreamScreen(); 
# if defined(ESP32_DEBUG_FREEMEM)
  global::printEspMemUsage("setup", "init dreamscreen"); 
# endif
#endif

#if defined(CONFIG_BT_ENABLED)
# if defined(DISABLE_BLUETOOTH)
#   if defined(ENABLE_RELAY) 
  // More al: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/controller_vhci.html
  if (global::GetAmbibackRelayMode() != RELAY_BT) {
#   endif
    _DBGPRINTF(PSTR("setup: FreeMem=%dKB before Bluetooth DISABLE Status=%d\n"), heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL) / 1024, esp_bt_controller_get_status());
#   if !defined(ARDUINO_ESP32_RELEASE_2_0_0)
    ret = esp_bluedroid_disable();              if (ret != ESP_OK) { _DBGPRINTF(PSTR("setup: Bluetooth esp_bluedroid_disable ESP_BT_MODE_BLE failed: %s\n"), esp_err_to_name(ret)); } 
    ret = esp_bluedroid_deinit();               if (ret != ESP_OK) { _DBGPRINTF(PSTR("setup: Bluetooth esp_bluedroid_deinit ESP_BT_MODE_BLE failed: %s\n"), esp_err_to_name(ret)); } 
    ret = esp_bt_controller_disable();          if (ret != ESP_OK) { _DBGPRINTF(PSTR("setup: Bluetooth esp_bt_controller_disable ESP_BT_MODE_BLE failed: %s\n"), esp_err_to_name(ret)); } 
    ret = esp_bt_controller_deinit();           if (ret != ESP_OK) { _DBGPRINTF(PSTR("setup: Bluetooth esp_bt_controller_deinit ESP_BT_MODE_BLE failed: %s\n"), esp_err_to_name(ret)); } 
#   endif
    ret = esp_bt_mem_release(ESP_BT_MODE_BTDM); if (ret != ESP_OK) { _DBGPRINTF(PSTR("setup: Bluetooth esp_bt_mem_release ESP_BT_MODE_BTDM failed: %s\n"), esp_err_to_name(ret)); } 
    ret = esp_bt_mem_release(ESP_BT_MODE_BLE); if (ret != ESP_OK) { _DBGPRINTF(PSTR("setup: Bluetooth esp_bt_mem_release ESP_BT_MODE_BLE failed: %s\n"), esp_err_to_name(ret)); } 
    _DBGPRINTF(PSTR("setup: FreeMem=%dKB after Bluetooth DISABLE\n"), heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL) / 1024);
#   if defined(ENABLE_RELAY) 
  } else {
    _DBGPRINTF(PSTR("setup: FreeMem=%dKB Bluetooth stays ENABLED Status=%d\n"), esp_bt_controller_get_status());
  }
#   endif
# else
    // TODO START BT
# endif
#endif

  // Check for FIRMWARE RESET
  int buttonState = digitalRead(BTN_PIN); 
  if (buttonState == BTN_STATE_ON) {
    _DBGPRINTF(PSTR("setup: Button pressed on STARTUP, wait for 10 seconds until doing actual FIRMWARE RESET\n"));  
    // Buttonpress started
    global::g_timeButtonPressed = millis(); 
    for(int iWait = 0; iWait < 10; iWait++) {
      global::TogglePwrLed(255, 0, 0);
      global::ToggleCtlLed(255, 0, 0);
      _delay(500); 
      _DBGPRINTF(PSTR("setup: firmware reset LOOP %d\n"), iWait);

      // Get new ButtonState
      buttonState = digitalRead(BTN_PIN); 
      if (buttonState == BTN_STATE_OFF) {
        // Abort Firmware Reset
        _DBGPRINTF(PSTR("setup: firmware reset ABORTED %d\n"), iWait);
        break;  
      }
    }

    if (buttonState == BTN_STATE_ON) {
      _DBGPRINTF(PSTR("setup: perform firmware reset in 1 second!\n"));
      global::TogglePwrLed(255, 255, 0);
      global::ToggleCtlLed(255, 255, 0);
      _delay(1000); 
      
      /* WE DO A EMERGENCY Factory reset! */ 
      pEepromHelper->FactoryReset(); 
      // actually ERASE AP with second param
      WiFi.disconnect(false,true); 
      global::RebootController(); // reset(); 
      _delay(1000); 
    }
  }

#ifdef ENABLE_AS312
    pinMode(AS312_PIN, INPUT);
#endif
#if defined(ENABLE_SSD1306)
    oled.init();
    // Wire.setClock(100000);  //! Reduce the speed and prevent the speed from being too high, causing the screen
    oled.setFont(ArialMT_Plain_16);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.flipScreenVertically(); 

    delay(50);

    global::oledSetObj(&oled); 
    global::uiSetObj(&ui); 
    global::uiInit();
//    global::oledDrawString("AmbiBack - Camera"); 
#endif

# if defined(ESP32_EXTRA_HEAP_TRACING)
  heap_trace_init_standalone(trace_record, NUM_RECORDS);
  heap_trace_start(HEAP_TRACE_ALL);
# endif

  // Handle WIFI Startup stuff
  global::HandleWifiAPSetup();
  global::HandleWifiConnectSetup();

# if defined(ESP32_DEBUG_FREEMEM)
  global::printEspMemUsage("setup", "end"); 
# endif
  _DBGPRINTF(PSTR(PSTR_SETUP "EXIT\n"));  
}
// --- 

// --- Main LOOP
void loop() {
# if defined(ESP32_DEBUG_FREEMEM)
  global::printEspMemUsage("loop", "begin"); 
# endif
  /* Loop variables */
  bool bButtonPressed = false;
#ifdef ENABLE_AS312
  bool bMovementDetected = false;
  unsigned long timeMovementDetected = 0;
#endif

#if defined(ENABLE_DBLRBT_WPS)
  int iLastMillis = global::m_pEepromHelper->GetSettingInt("syslastmillis", 0xFFFFFFFF, true); 
  if (iLastMillis == 0xFFFFFFFF) {
    _DBGPRINTF(PSTR(PSTR_LOOP "syslastmillis 0xFFFFFFFF - not written yet\n")); 
  } else if (iLastMillis == 0x7FFFFFFF) {
    iLastMillis = millis(); 
    _DBGPRINTF(PSTR(PSTR_LOOP "syslastmillis 0x7FFFFFFF - not written during last reboot - setting to %d now!\n"), iLastMillis); 
    global::m_pEepromHelper->SetSettingInt("syslastmillis", iLastMillis);
  } else if (iLastMillis < DEFAULT_WPS_REBOOT_CHECK_TIMOUT) {
    _DBGPRINTF(PSTR(PSTR_LOOP "syslastmillis below %d (%d), DOUBLE REBOOT detected - forcing WPS Mode\n"), DEFAULT_WPS_REBOOT_CHECK_TIMOUT, iLastMillis); 
    global::SetWifiTryWPS(true);
    global::SetWifiTryConnect(false);
    global::TogglePwrLedMsg();
    global::ToggleCtlLedOff();
    // Set Last Millis to highest possible value
    global::m_pEepromHelper->SetSettingInt("syslastmillis", 0xFFFFFFFF);
    // Make sure LED Mode is set to OFF 
    global::m_pEepromHelper->SetSettingByte("LedMode", LED_MODE_OFF);
  } else {
    _DBGPRINTF(PSTR(PSTR_LOOP "syslastmillis is %d, setting back to millis()\n"), iLastMillis);
    iLastMillis = millis();
  }
#endif

  // Init went so far, Activate Power LED
  global::TogglePwrLedOn(); 

#if defined(ENABLE_CAM)
  if (!global::m_bCameraOK) 
    global::InitCam();
    
# if defined(ESP32_DEBUG_FREEMEM)
  global::printEspMemUsage("loop", "init cam"); 
# endif
#endif

  _DBGPRINTF(PSTR(PSTR_LOOP "ENTER MAIN LOOP\n"));
beginLoop:
  // -- Get Current Millis first time 
  global::g_timeNow = millis(); 

#if defined(ENABLE_SERIAL_INPUT)
  serialHelper.checkSerial();
#endif

  /* --- Check if new LED needs to be processed --- */
  pLedhelper->HandleLeds(); 
#ifdef ENABLE_IR
  HandleIR();   /* Perform IR Data check */
#endif
#if defined(ENABLE_SSD1306)
  // Handle own Display Queue 
  if (global::oledProcessQueue()) {
    global::g_timeSS1306Wait = millis();
  }
  // Reset TimeWait!
  if (millis() > (global::g_timeSS1306Wait+5000))
    global::g_timeSS1306Wait = 0; 
  
  // Handle UI Queue on idle
  if (global::g_timeSS1306Wait == 0) {   
    ui.update(); 
  }
#endif


#ifdef ENABLE_AS312
  /* --- Check Movement State once every second! --- */
  int movementState = digitalRead(AS312_PIN);
  if (bMovementDetected == false && movementState) {
      // Movement started!
      timeMovementDetected = millis(); 
      bMovementDetected = true; 
      _DBGPRINTF(PSTR(PSTR_LOOP "Movement detected started\n"));  
  } else if (bMovementDetected == true && !movementState) {
      // Movement released
      unsigned long timePassed = millis() - timeMovementDetected; 
      bMovementDetected = false; 
      global::dbgprintf(PSTR(PSTR_LOOP "Movement detected end (Duration %d ms)\n"), timePassed);

       /* Handle Movement length */
      if (timePassed < 3000) {
        _DBGPRINTF(PSTR(PSTR_LOOP "Movement less than 3 seconds\n")); 
        // TODO MAKE CONFIGUREABLE!
        // global::HandleButtonFunction();
      } else if (timePassed < 10000) {
        _DBGPRINTF(PSTR(PSTR_LOOP "Movement between 3 and 10 seconds\n")); 
      } else {
        _DBGPRINTF(PSTR(PSTR_LOOP "Movement more than 10 seconds\n")); 
      }
  }
#endif

/* --- Check Button State  --- */
  int buttonState = digitalRead(BTN_PIN); 
  if (bButtonPressed == false && buttonState == BTN_STATE_ON) {
      // Buttonpress started
      global::g_timeButtonPressed = millis(); 
      bButtonPressed = true;
      global::SetButtonPressed(true);
      global::SetButtonPressedStart(global::g_timeButtonPressed);
      global::TogglePwrLedOff(); 
      global::ToggleCtlLedOff(); 
      _DBGPRINTF(PSTR(PSTR_LOOP "Button pushed\n"));  
  } else if (bButtonPressed == true && buttonState == BTN_STATE_OFF) {
      // Buttonpress released
      unsigned long timePassed = millis() - global::g_timeButtonPressed; 
      bButtonPressed = false;
      global::SetButtonPressed(false);
      global::SetButtonPressedStart(0);
      global::dbgprintf(PSTR(PSTR_LOOP "Button released, restoring LEDs\n"));
      global::TogglePwrLedOn(); 
      if (WiFi.status() == WL_CONNECTED) {
        global::ToggleCtlLedOn(); 
      } else {
        global::ToggleCtlLedOff(); 
      }

       /* Handle Button press length */
      if (timePassed < 1500) {
        _DBGPRINTF(PSTR(PSTR_LOOP "Button <1.5 sec press\n")); 
        global::HandleButtonFunction();
      } else if (timePassed < 7500) {
        _DBGPRINTF(PSTR(PSTR_LOOP "Button >1.5 && <7.5 sec press\n")); 

        if (strlen(global::GetWifiSSID()) == 0 && timePassed > 3500) {
          _DBGPRINTF(PSTR(PSTR_LOOP "FACTORY DEFAULT Button >3.5 && <7.5 sec press, setting WifiTryWPS\n"));  
          global::TogglePwrLedOn(255, 32, 0, true);
          global::ToggleCtlLedOn(255, 32, 0, true);
          
          global::SetWifiTryWPS(true);
          global::SetWifiTryConnect(false);
          global::SetWifiTryReConnect(false);
          global::g_timeWIFITryConnectCount = 0;
          
          WiFi.disconnect(true); /* We need to disconnect the WIFI first */
          _delay(1000);
#if defined(ENABLE_WIFI_AP)
        } else if (timePassed > 3500) {
          _DBGPRINTF(PSTR(PSTR_LOOP "Button >3.5 && <7.5 sec press, opening setup window\n"));
          global::TogglePwrLedOn(0, 0, 255, true);
          global::OpenProvisioningByUser();
#endif
        } else {
          /* By default, we perform a REBOOT */
          global::SetShouldReboot(true); 
        }
      } else if (timePassed < 15000) {
        _DBGPRINTF(PSTR(PSTR_LOOP "Button >7.5 && <15 sec press, setting WifiTryWPS\n"));  
        global::TogglePwrLedOn(255, 32, 0, true);
        global::ToggleCtlLedOn(255, 32, 0, true);
        
        global::SetWifiTryWPS(true);
        global::SetWifiTryConnect(false);
        global::SetWifiTryReConnect(false);
        global::g_timeWIFITryConnectCount = 0;
        
        WiFi.disconnect(true); /* We need to disconnect the WIFI first */
        _delay(1000);
      } else {
        _DBGPRINTF(PSTR(PSTR_LOOP "Button >15 sec press, doing FACTORY RESET!\n"));  

        /* WE DO A Factory reset! */ 
        pEepromHelper->FactoryReset(); 
        // actually ERASE AP with second param
        WiFi.disconnect(false,true); 
        global::RebootController(); // reset(); 
        _delay(1000); 
      }
  } else if (buttonState == BTN_STATE_ON) {
    /* Do fancy LED blink stuff */
    unsigned long timePassed = (millis() - global::g_timeButtonPressed); 
    if (timePassed > 0) {
      if (timePassed > 15000) {
        if (timePassed % 300 == 0 ) {
          _DBGPRINTF(PSTR(PSTR_LOOP "Button toggle1 led\n"));  
          global::TogglePwrLed(255, 0, 0);
          global::ToggleCtlLed(255, 0, 0);
        }
      } else  if (timePassed > 7500) {
        if (timePassed % 300 == 0 ) {
          _DBGPRINTF(PSTR(PSTR_LOOP "Button toggle2 led\n"));  
          global::TogglePwrLed(255, 255, 0);
          global::ToggleCtlLed(255, 255, 0);
        }
      } else {
        if (timePassed % 300 == 0 ) {
          _DBGPRINTF(PSTR(PSTR_LOOP "Button toggle3 led\n"));  
          global::TogglePwrLed(0, 0, 255);
          global::ToggleCtlLed(0, 0, 255);
        }
      }
    }
  }
/* --- --- --- */

  // -- Get Current Millis SECOND time
  global::g_timeNow = millis(); 

# if defined(ENABLE_DBLRBT_WPS)
/* --- Setting Last Millis state --- */
  if (iLastMillis < DEFAULT_WPS_REBOOT_CHECK_TIMOUT && global::g_timeNow > DEFAULT_WPS_REBOOT_CHECK_TIMOUT) {
      _DBGPRINTF(PSTR(PSTR_LOOP "iLastMillis below %d (%d) and timeNow above %d, resetting syslastmillis back to 0x7FFFFFFF\n"), DEFAULT_WPS_REBOOT_CHECK_TIMOUT, iLastMillis, global::g_timeNow); 
      // Set Last Millis to highest possible value
      iLastMillis = 0x7FFFFFFF; 
      global::m_pEepromHelper->SetSettingInt("syslastmillis", iLastMillis);
  }
/* --- --- --- */
# endif

/* --- Run Checks every second --- */
  if (  millis() > DEFAULT_WIFI_STARTUP_CHECK_WAIT && 
      ( global::g_timeNow - global::g_timeWIFICheck) > 1000 ) {
    /* CHeck WPS / WIFI */
    global::HandleWifiWPS(&esp32wps_config);
    global::HandleWifiCheck();

    /* Flush Serial messages every second only */
    global::flushserial(); 
  }
/* --- --- --- */

/* --- Checks every 5 seconds --- */
  if ( (global::g_timeNow - global::g_timeFreeMemCheck) > SYS_FREEMEM_CHECK_TIME ) {
    /* FreeMemory Check --- */
    global::g_timeFreeMemCheck = millis();
    int iFreeKB = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL) / 1024;
    if (iFreeKB < SYS_FREEMEM_MIN) {
      global::errprintf_P(PSTR("loop: LOW MEMORY %dKB\n"), iFreeKB);

      /* If Wifi Status is connected but signal is 0, it most likely means we lost connection due low memory! 
       *  In this case we perform an emergency reboot!
       */
      if (WiFi.status() == WL_CONNECTED && WiFi.RSSI() == 0) {
        global::errprintf_P(PSTR("loop: Wifi Status CONNECTED but SIGNAL lost, EMERGENCY REBOOT!\n"));
        global::RebootController();
        _delay(1000); 
      }
// TODO: EMERGENCY REBOOT?
    }
  }
/* --- --- --- */

/* --- DEBUG Output every 5 seconds --- */
  if ( (global::g_timeNow - global::g_timeDebugCheck) > DIAG_DEBUG_OUTPUT_TIME ) {
    /* Update timeDebugCheck */
    global::g_timeDebugCheck = millis();
#if defined(ENABLE_CAM)
# define CAM_PRINTFSTR " CamFPS=%d/s BandMode=%s"
#else
# define CAM_PRINTFSTR ""
#endif      
#if defined(ENABLE_HUE)
# define HUE_PRINTFSTR " HueFPS=%d/s"
#else
# define HUE_PRINTFSTR ""
#endif
#if defined(ESP32_RUNTIMESTATS)
# define RTS_PRINTFSTR " CPU=%dMhz XTal=%dMhz Tasks=%d"
#else
# define RTS_PRINTFSTR ""
#endif
    _DBGPRINTF(PSTR(PSTR_LOOP "WiFi=%s(%s) Sig=%ddBm VDD=%d INT/HEAP/SPIRAM=%dKB/%dKB/%dKB Serial=%d Mode=%s LedFPS=%d/s Delay=%dms NetState=%s%s NetPkts=%d/%d/s" CAM_PRINTFSTR HUE_PRINTFSTR RTS_PRINTFSTR "\n"), 
      global::GetWifiStatusTxt(WiFi.status()), 
      WiFi.SSID().c_str(),
      WiFi.RSSI(),
      rom_phy_get_vdd33(),
      heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL) / 1024,
      ESP.getFreeHeap() / 1024,
      (psramFound() ? heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM) / 1024 : 0), 
      Serial.baudRate(),
      pLedhelper->GetLedModeStr(), 
      pLedhelper->GetCurrentFps(),
      pLedhelper->GetDelayLength(),
      (global::GetClientConnected() ? PSTR("Connected to ") : PSTR("Waiting") ), 
      (global::GetClientConnected() ? global::GetClientConnectedIpStr() : PSTR("...") ), 
      global::GetCurrentPkts(), 
      global::GetCurrentPktsDropped()
#if defined(ENABLE_CAM)
      , global::m_pCamProcessor->GetCurrentCamFps()
      , (global::GetCamBandMode() == 0 ? "60Hz" : "50Hz")
#endif
#if defined(ENABLE_HUE)
      , global::m_pHueHelper->GetCurrentFps()
#endif
#if defined(ESP32_RUNTIMESTATS)
      , getCpuFrequencyMhz()
      , getXtalFrequencyMhz()
      , uxTaskGetNumberOfTasks()
#endif
    );

    // Extra Online Checks
    if (WiFi.status() == WL_CONNECTED) {
      // Make sure we reset WifiTry counter
      global::g_timeWIFITryConnectCount = 0;
      // Keep Power LED ON
      global::TogglePwrLedOn(true);
    }
#ifdef ENABLE_SSD1306
//  oledDrawString(PSTR("WiFi=%s, FreeMem=%dKB FreePSRAM:%dKB"));
#endif

#if defined(ESP32_EXTRA_DIAG)
    _DBGPRINTF(PSTR("loop: heap debug, free_size=%d bytes, largest_free_block=%d bytes, minimum_free_size=%d bytes HEAPCHECK=%s \n"), 
      heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
      heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL), 
      heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
      (heap_caps_check_integrity_all(true) ? "OK" : "CORRUPT")
    );
# if defined(ESP32_EXTRA_HEAP_TRACING)
    heap_trace_stop();
    heap_trace_dump();
    heap_trace_start(HEAP_TRACE_ALL);
# endif
#endif

/* FUCKING vtasklist is missng!
    char sztasklist[512];
    memset(&sztasklist, 0, 512);
    vTaskList(sztasklist);
    _DBGPRINTF(PSTR("Task  State   Prio    Stack    Num\n%s"), sztasklist);
*/
    // Reset Counters
    global::SetCurrentPkts(0);
    global::SetCurrentPktsDropped(0);
#if defined(ENABLE_HUE)
    global::m_pHueHelper->SetCurrentFps(0); 
    global::m_pHueHelper->SetCurrentFpsSkip(0); 
#endif
  }
/* --- --- --- */
/* --- Check for EEProm Save after period of time --- */
  if (global::GetSettingsChanged() ) {
    if ( global::g_timeNow > (pEepromHelper->GetLastUpdate() + DEFAULT_EEPROM_SAVE_TIMEOUT)) {
      // Force Write Changes to EEPROM
      pEepromHelper->CommitChanges();   
      _DBGPRINTF_ULTRA(PSTR("loop: EEPRom AutoSave %d ms - %d(%d) hit - changes commited\n"), DEFAULT_EEPROM_SAVE_TIMEOUT, global::g_timeNow, pEepromHelper->GetLastUpdate());  
    } 
  }
/* --- --- --- */
/* --- Handle other stuff --- */
  // Handle WebServer requests again here!
  if (webServer.GetInitDone()) {
    webServer.handleClient();
  }
/* --- --- --- */
/* --- Check for StartWps here! --- */
  if(global::GetShouldStartWps()) {
    _DBGPRINTF(PSTR("loop: shouldStartWps ENABLED, peforming WPS Mode in next loop\n"));
    global::SetWifiTryWPS(true);
    global::SetWifiTryConnect(false);
    global::SetWifiTryReConnect(false);
    global::g_timeWIFITryConnectCount = 0;
  }  
/* --- --- --- */
/* --- Check for HTTP FirmwareUpdate here! --- */

  if(global::GetShouldStartFirmwareUpdate() && global::GetFirmwareRunning() == false ){
#if defined(OTA_UPDATE_TASK)
    int iFreeOTAKB = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL) / 1024;
    if (iFreeOTAKB < SYS_FREEMEM_OTA_MIN) {
      global::errprintf_P(PSTR("loop: http FirmwareUpdate triggered with (%dKB) LOW MEMORY\n"), iFreeOTAKB);
#else
      _DBGPRINTF(PSTR("loop: http FirmwareUpdate triggered with (%dKB) memory\n"), iFreeOTAKB);
#endif
      global::SetShouldStartFirmwareUpdate(false);
      // Trigger Update on Loop TASK!
      global::StartHttpUpdate();

      global::SetWifiTryWPS(true);
      global::SetWifiTryConnect(false);
      global::g_timeWIFITryConnectCount = 0;
#if defined(OTA_UPDATE_TASK)
    }  
#endif
  }  
/* --- --- --- */

/* --- Check for reboot here! --- */
  if(global::GetShouldReboot() && (global::GetRebootAt() == 0 || millis() >= global::GetRebootAt())){
    _DBGPRINTF(PSTR("loop: shouldReboot ENABLED, peforming reboot now\n"));
    global::RebootController();
    _delay(1000); 
  }  
/* --- --- --- */

/* --- Check for action here! --- */
  if(global::m_doAction > 0){
    _DBGPRINTF(PSTR("loop: doAction triggered!\n") );

    switch(global::m_doAction){
#ifdef ENABLE_DREAMSCREEN
      case ACTION_AddDreamScreenToNetwork:
        _delay(500); 
        global::m_pDreamScreen->AddDreamScreenToNetwork();
        break;
#endif
      default:
        _DBGPRINTF(PSTR("loop: doAction triggered UNKNOWN=%d!\n"), global::m_doAction);
        break;  
    }
    // Disable
    global::SetDoAction( ACTION_off ); 
  }  
/* --- --- --- */

  // Give other tasks a chance to execute
  _yield();

  // Repeat our LOOP 
  goto beginLoop;
}


// --- Helper functions
#ifdef ENABLE_IR
void HandleIR() {
  /* --- Perform IR Data check --- */
  irhelper.checkIRdata(); 
  if (irhelper.GetIRReceived()) {
    if ( irhelper.GetLastReceivedIR()  != 0xFFFFFFFF ) /* Repeat last command */
      irhelper.SetLastIR(irhelper.GetLastReceivedIR()); 
    /* enable receive next IR data */
    irhelper.SetIRReceived(false);
    /* Process IR command */
    pLedhelper->processIRCommand(irhelper.GetLastIR()); 
  }
  /* --- --- --- */
}
#endif

void wpsInitConfig() {
  /* not anymore in ESP_IDF 4.4 
  esp32wps_config.crypto_funcs = &g_wifi_default_wps_crypto_funcs;
  */
  esp32wps_config.wps_type = ESP_WPS_MODE;
  strcpy(esp32wps_config.factory_info.manufacturer, ESP_MANUFACTURER);
  strcpy(esp32wps_config.factory_info.model_number, ESP_MODEL_NUMBER);
  strcpy(esp32wps_config.factory_info.model_name, ESP_MODEL_NAME);
  strcpy(esp32wps_config.factory_info.device_name, ESP_DEVICE_NAME);
}
String wpsPin2string(uint8_t a[]){
  char wps_pin[9];
  for(int i=0;i<8;i++){
    wps_pin[i] = a[i];
  }
  wps_pin[8] = '\0';
  return (String)wps_pin;
}

# if defined(ARDUINO_ESP32_RELEASE_2_0_0) || (defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR == 2)
void handleWiFiEvents(arduino_event_id_t event, arduino_event_info_t info) {
# elif defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void handleWiFiEvents(arduino_event_id_t event, arduino_event_info_t info) {
# else
void handleWiFiEvents(WiFiEvent_t event, system_event_info_t info) {
# endif
  switch(event){
    case ARDUINO_EVENT_WIFI_READY:
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "Ready\n"));
      break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "SCAN Finished - SSID %s RSSI %d Ch %d\n"),
        (WiFi.SSID() != NULL ? WiFi.SSID().c_str() : "%INVALID%"), 
        WiFi.RSSI(), 
        WiFi.channel());

# if defined(ARDUINO_ESP32_RELEASE_2_0_0) || (defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 2)
      if (info.wifi_scan_done.status == 0) {
        _DBGPRINTF(PSTR(PSTR_WIFI_EV "SCAN found %d networks\n"), info.wifi_scan_done.number);
        for(uint8_t iIndex=0; iIndex < info.wifi_scan_done.number; iIndex++) {
# else
      if (info.wifi_scan_done.status == 0) {
        _DBGPRINTF(PSTR(PSTR_WIFI_EV "SCAN found %d networks\n"), info.wifi_scan_done.number);
        for(uint8_t iIndex=0; iIndex < info.wifi_scan_done.number; iIndex++) {
# endif
          _DBGPRINTF(PSTR(PSTR_WIFI_EV "%d SSID %s RSSI %d Enc %s Ch %d\n"),
            iIndex, 
            (WiFi.SSID(iIndex) != NULL ? WiFi.SSID(iIndex).c_str() : "%INVALID%"), 
            WiFi.RSSI(iIndex), 
            global::GetWifiAuthTxt(WiFi.encryptionType(iIndex)),
            WiFi.channel((uint8_t)iIndex));
        }
      } else {
        _DBGPRINTF(PSTR(PSTR_WIFI_EV "SCAN FAILED"));
      }
      break;
    case ARDUINO_EVENT_WIFI_STA_START:
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "STA Started\n"));
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "STA Stopped\n"));
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      global::SetWifiTryReConnect(false);
# if defined(ARDUINO_ESP32_RELEASE_2_0_0) || (defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 2)
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "STA Connected: %s\n"), info.wifi_sta_connected.ssid);
# else
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "STA Connected: %s\n"), WiFi.SSID().c_str());
# endif
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP : /*IP_EVENT_STA_GOT_IP: */
# if defined(ARDUINO_ESP32_RELEASE_2_0_0) || (defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 2)
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "Got IP: %d.%d.%d.%d\n"), 
      info.got_ip.ip_info.ip.addr & 0xFF, 
      (info.got_ip.ip_info.ip.addr >> 8) & 0xFF, 
      (info.got_ip.ip_info.ip.addr >> 16) & 0xFF, 
      (info.got_ip.ip_info.ip.addr >> 24) & 0xFF
      );
# else
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "Got IP: %s\n"), WiFi.localIP().toString().c_str());
# endif
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      if (global::GetWifiConnectCheck()) {
        if (!global::GetWifiTryReConnect()) {
          _DBGPRINTF(PSTR(PSTR_WIFI_EV "Lost IP, reconnect...\n")  );
          WiFi.disconnect();
          WiFi.reconnect();
  #if defined(WIFI_SETSLEEP)
          // Do not use
          WiFi.setSleep(false);
  #endif
        } else {
          _DBGPRINTF(PSTR(PSTR_WIFI_EV "Lost IP, reconnect already running\n")  );
        }
      }
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      if (global::GetWifiTryWPS()) {
        global::dbgprintf(PSTR(PSTR_WIFI_EV "WPS Mode: Disconnected\n"));
      } else {
        if ( !global::GetWifiConnectCheck() || 
             millis() < DEFAULT_WIFI_STARTUP_CHECK_WAIT ) {
          global::dbgprintf(PSTR(PSTR_WIFI_EV "Disconnected, Check DISABLED\n"));
          break;
        }
        global::dbgprintf( PSTR(PSTR_WIFI_EV "Disconnected reason: %d (%s), reconnect Mode %d...\n"), 
# if defined(ARDUINO_ESP32_RELEASE_2_0_0) || (defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 2)
          info.wifi_sta_disconnected.reason, wifi_disconnect_reason_to_str(info.wifi_sta_disconnected.reason), global::GetWifiFailHandling() );
# else
          info.disconnected.reason, wifi_disconnect_reason_to_str(info.disconnected.reason), global::GetWifiFailHandling() );
# endif

        // --- REBOOT HANDLING
        // DISCONNECTED, perform REBOOT
        if ( global::GetWifiFailHandling() == WIFI_FAIL_REBOOT) {
          global::errprintf_P(PSTR(PSTR_WIFI_EV "Disconnected: REBOOT\n"));
          global::RebootController();
          _delay(1000);
        }

        // Check for timeout
        if ( (millis() - global::g_timeWIFITryConnect) > DEFAULT_WIFI_CONNECT_TIMEOUT) {
          _DBGPRINTF(PSTR(PSTR_WIFI_EV "Timeout connecting to %s, retry %d...\n"), global::GetWifiSSID(), global::g_timeWIFITryConnectCount);
          global::SetWifiTryReConnect(false);
          global::g_timeWIFITryConnectCount++;

          // RECONNECT failed a few times, perform REBOOT
          if (global::g_timeWIFITryConnectCount > DEFAULT_WIFI_REBOOT_AFTER_FAILS) {
            if ( global::GetWifiFailHandling() == WIFI_FAIL_RECONNECTREBOOT) {
              global::errprintf_P(PSTR(PSTR_WIFI_EV "Reconnect failed %d times, REBOOT\n"), global::g_timeWIFITryConnectCount);
              global::RebootController();
              _delay(1000);
            }
          }
        }

        // RECONNECT failed for too many times, perform REBOOT
        if (global::g_timeWIFITryConnectCount > DEFAULT_WIFI_FORCE_REBOOT_AFTER_FAILS) {
          global::errprintf_P(PSTR(PSTR_WIFI_EV "Reconnect failed %d times, FORCE REBOOT\n"), global::g_timeWIFITryConnectCount);
          global::RebootController();
          _delay(1000);
        }
        // ---
        
        // --- Do Reconnect Handling!
        if (!global::GetWifiTryReConnect()) {
          global::SetWifiTryReConnect(true);
          global::g_timeWIFITryConnect = millis();

          // Use this function to restore own WIFI
          global::ConnectWifiConfigured(global::GetWifiSSID(), global::GetWifiPassword(), true); 
        } else {
          global::dbgprintf(PSTR(PSTR_WIFI_EV "Disconnected, reconnect already running\n"));
        }
        // ---
      }
      break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
# if defined(ARDUINO_ESP32_RELEASE_2_0_0) || (defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 2)
      char* pszWifiSSID;
      char* pszWifiPSK;
      wifi_ap_record_t infoAp;
      if(!esp_wifi_sta_get_ap_info(&infoAp)) {
        pszWifiSSID = (char*)infoAp.ssid;
        pszWifiPSK = (char*)WiFi.psk().c_str();
        _DBGPRINTF(PSTR(PSTR_WIFI_EV "WPS OK, using get_ap_info %s\n"), pszWifiSSID);
      } else {
        wifi_config_t confWifi;
        esp_wifi_get_config(WIFI_IF_STA, &confWifi);
        pszWifiSSID = (char*)confWifi.sta.ssid;
        pszWifiPSK = (char*)confWifi.sta.password;
        _DBGPRINTF(PSTR(PSTR_WIFI_EV "WPS OK, using get_config %s\n"), pszWifiSSID);
      }
      // Save WPS results   
      if(pszWifiSSID != NULL && pszWifiPSK != NULL ) {
        _DBGPRINTF(PSTR(PSTR_WIFI_EV "WPS OK, connecting to: %s\n"), pszWifiSSID);

        // Save Settings to EEPROM before doing other stuff!
        global::SetWifiSSID(pszWifiSSID); 
        global::SetWifiPassword(pszWifiPSK); 
        // Force Write Changes to EEPROM
        pEepromHelper->CommitChanges();
        _yield(); 
          
        esp_wifi_wps_disable();
        _delay(1000);

        // Use this function to restore own WIFI
        global::ConnectWifiConfigured(global::GetWifiSSID(), global::GetWifiPassword(), true); 
      } else {
        _DBGPRINTF(PSTR(PSTR_WIFI_EV "WPS OK but empty SSID %s PSK %s\n"), 
          (pszWifiSSID == NULL ? "NULL" : pszWifiSSID), 
          (pszWifiPSK == NULL ? "NULL" : pszWifiPSK));
      }
# else
      if(WiFi.SSID().length() > 0 && WiFi.psk().length() > 0 ) {
        _DBGPRINTF(PSTR(PSTR_WIFI_EV "WPS OK, connecting to: %s\n"), WiFi.SSID().c_str());
  
        // Save Settings to EEPROM before doing other stuff!
        global::SetWifiSSID((char*) WiFi.SSID().c_str()); 
        global::SetWifiPassword((char*) WiFi.psk().c_str()); 
        // Force Write Changes to EEPROM
        pEepromHelper->CommitChanges();
        _yield(); 
          
        esp_wifi_wps_disable();
        _delay(10);
        WiFi.begin();
#   if defined(WIFI_SETSLEEP)
        // Do not use
        WiFi.setSleep(false);
#   endif
      } else {
        _DBGPRINTF(PSTR(PSTR_WIFI_EV "WPS OK but empty SSID %d PSK %d\n"), WiFi.SSID().length(), WiFi.psk().length());
      }
# endif    

      break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
# if defined(ARDUINO_ESP32_RELEASE_2_0_0) || (defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 2)
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "WPS Failed reason %d\n"), info.wps_fail_reason );
# else
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "WPS Failed\n"));
# endif
      esp_wifi_wps_disable();

      // Set Mode if needed
      global::HandleWifiMode();

      // Renenable WPS Mode
      esp_wifi_wps_enable(&esp32wps_config);
      esp_wifi_wps_start(0);
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "WPS re-enabled\n"));
      break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "WPS Timeout\n"));
      esp_wifi_wps_disable();
      esp_wifi_wps_enable(&esp32wps_config);
      esp_wifi_wps_start(0);
      break;
    case ARDUINO_EVENT_WPS_ER_PIN:
# if defined(ARDUINO_ESP32_RELEASE_2_0_0) || (defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 2)
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "WPS_PIN = %d\n"), wpsPin2string(info.wps_er_pin.pin_code));
# else
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "WPS_PIN = %d\n"), wpsPin2string(info.sta_er_pin.pin_code))
# endif
      break;
    case ARDUINO_EVENT_WIFI_AP_START:
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "AP started\n"));
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "AP stopped\n"));
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "AP Client Connected\n"));
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "AP Client Disconnected\n"));
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
# if defined(ARDUINO_ESP32_RELEASE_2_0_0) || (defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 2)
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "AP Client IP: %d.%d.%d.%d\n"), 
        info.wifi_ap_staipassigned.ip.addr & 0xFF, 
        (info.wifi_ap_staipassigned.ip.addr >> 8) & 0xFF, 
        (info.wifi_ap_staipassigned.ip.addr >> 16) & 0xFF, 
        (info.wifi_ap_staipassigned.ip.addr >> 24) & 0xFF
      );
# else
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "AP Client IP assigned\n"));
# endif    
      break;
    default:
      _DBGPRINTF(PSTR(PSTR_WIFI_EV "Unhandled EVENT %d\n"), event);
      break;
  }
}
const char* esp32GetRebootRsnStr(byte bCore, RESET_REASON reason){
  switch ( reason){
    case POWERON_RESET : return PSTR("Vbat power on reset");
    case SW_RESET : return PSTR("Software reset digital core");         
    case OWDT_RESET : return PSTR("Legacy watch dog reset digital core");
    case DEEPSLEEP_RESET : return PSTR("Deep Sleep reset digital core");    
    case SDIO_RESET : return PSTR("Reset by SLC module, reset digital core");        
    case TG0WDT_SYS_RESET : return PSTR("Timer Group0 Watch dog reset digital core");    
    case TG1WDT_SYS_RESET : return PSTR("Timer Group1 Watch dog reset digital core");    
    case RTCWDT_SYS_RESET : return PSTR("RTC Watch dog Reset digital core");      
    case INTRUSION_RESET : return PSTR("Instrusion tested to reset CPU");  
    case TGWDT_CPU_RESET : return PSTR("Time Group reset CPU"); 
    case SW_CPU_RESET : return PSTR("Software reset CPU");    
    case RTCWDT_CPU_RESET : return PSTR("RTC Watch dog Reset CPU");   
    case EXT_CPU_RESET : return PSTR("for APP CPU, reseted by PRO CPU");     
    case RTCWDT_BROWN_OUT_RESET : return PSTR("Reset when the vdd voltage is not stable");  
    case RTCWDT_RTC_RESET : return PSTR("RTC Watch dog reset digital core and rtc module");  
    default : return PSTR("Unknown reboot reason");
  }  
}

// ---
