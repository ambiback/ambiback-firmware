// Include all needed files
#include "globalincludes.h"
#include "Arduino.h"
#include "global.h"
#include "debughelper.h"
#if defined(ENABLE_CAM)
#include "camprocessor.h"
#endif
// Include libs that are required in code here
#include "udpserver.h"
#if defined(ENABLE_TCP)
#include "tcpserver.h"
#endif 

#include <stdarg.h>
#include <new>  // For std::nothrow
#include <HTTPClient.h>
#include "ambiHTTPUpdate.h"

// ESP32 Includes
#include "esp_crc.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include <esp_task_wdt.h>
#include "esp_wps.h"
#include <GeneralUtils.h>

# if defined(ENABLE_WIFI_NAT)
# define DHCP_DNS_IP_ADDR 0x08080808 // 8.8.8.8 Google

// NAT functions are not available in ESP32 2.0.x Arduino framework (missing from precompiled lwip library)
// Only include NAT headers if NOT ESP32 2.0.x
# if !(defined(ARDUINO_ESP32_RELEASE_2_0_0) || (defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR == 2))
// Include required for IP NAT 
# include "lwip/lwip_napt.h"
# include "lwip/ip.h"
# include "dhcpserver/dhcpserver.h"
# include "dhcpserver/dhcpserver_options.h"
# if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
#   include "esp_netif.h"  // ESP32 3.x: esp_netif API
# else
#   include "tcpip_adapter.h"  // ESP32 1.x: legacy tcpip_adapter API
# endif

// SAMPLE CODE
//  https://github.com/paclema/esp32_lwip_nat_example/blob/a73440ecca8eda81671f8a5165df3c04de9100c1/src/main.cpp
# endif
# endif

#if defined(ENABLE_CAM)
# include "src/esp32cam/esp32cam.h"
  // Create Camera config
  esp32cam::Config m_CamCfg;
#endif

#if defined(ENABLE_RELAY)
# include <cJSON.h>
#endif

#ifdef _DEBUG_UDP
Syslog* global::m_pSyslog = NULL;
#endif

LEDHelper* global::m_pLedHelper = NULL;
EEPROMHelper* global::m_pEepromHelper = NULL;
AmbiWebServer* global::m_pWebServer = NULL;
#if defined(ENABLE_CAM)
# include "camprocessor.h"
::CamProcessor* global::m_pCamProcessor = NULL;
#endif

// Init Variables
bool global::m_shouldReboot = false;
unsigned long global::m_lRebootAt = 0;
bool global::m_shouldStartWps = false;
bool global::m_firmwareRunning;          // Firmware Update Running Helper
bool global::m_shouldStartFirmwareUpdate;// Trigger Firmware Update Helper
bool global::m_bSettingsChanged = false;
bool global::m_bNetworkStarted = false;
bool global::m_bClientConnected = false;
bool global::m_bClientConnectedIsNat = false;
bool global::m_bSerialConnected = false;
char global::m_szCurClientIP[STR_IP_MAX];
u32_t global::m_CurClientAddr = 0;

bool global::m_bSerialEnabled = _DEBUG_SERIAL;
bool global::m_bSyslogEnabled = _DEBUG_UDP;
bool global::m_bUltraDebugEnabled = ESP32_ULTRA_DEBUG_ENABLED;

// Frame Package Counter variables
byte global::m_bCurrentPkts = 0;            // Received Pkts/s
byte global::m_bCurrentPktsDropped = 0;     // Dropped Pkts/s
byte global::m_bCurrentFramesCtr = 0;       // Counter for received packets
byte global::m_bCurrentDroppedCtr = 0;      // Counter for dropped packets
int global::m_doAction = ACTION_off;
unsigned long global::g_timeNow = 0;                //  
unsigned long global::g_timeButtonPressed = 0;      // 
unsigned long global::g_timeWIFICheck = 0;          // 
unsigned long global::g_timeDebugCheck = 0;         // 
unsigned long global::g_timeFreeMemCheck = 0;       // 
unsigned long global::g_timeWIFITryConnect = 0;     //
unsigned long global::g_timeWIFITryConnectCount = 0;//
#if defined(ENABLE_SSD1306)
unsigned long global::g_timeSS1306Wait = 0;         //
#endif
unsigned long global::g_timeNowLastNetworkErr = 0;  //

int global::g_httpUpdateCheck = 0;    // Helper for OTA

int global::m_iColorPreset = 1;       // Normal Mode
int global::m_iBrightnessPreset = 3;  // Normal Mode
int global::m_iMotionPreset = 2;      // Normal Mode
int global::m_iAmbientPreset = 0;     // Solid; 1-11 scenes; 12=40Hz Gamma; 13=10Hz Alpha; 14=34Hz Gamma Red; 15=Rotlicht Abend
int global::m_iAmbientSpeed = 5;      // 1-10
int global::m_iAmbientIntensity = 80; // 1-100
int global::m_iAmbientHealthIntensity = 80; // 1-100
#if defined(ENABLE_CAM)
int global::m_iCamPreset = 1;         // Default WhiteBalance Mode
int global::m_iExposurePreset = 2;    // Dark Mode
int global::m_iCamInnerPreset = 3;    // Default Mode
#endif
char global::m_wifi_ssid[WLAN_SSID_MAX];
char global::m_wifi_password[WLAN_KEY_MAX];
char global::m_devicename[DEVICE_NAME_MAX];
char global::m_deviceid[DEVICE_ID_MAX];
char global::m_deviceidmac[DEVICE_MAC_MAX];
bool global::m_wifi_ap_enabled = true;
char global::m_wifi_ap_ssid[WLAN_SSID_MAX];
char global::m_wifi_ap_password[WLAN_KEY_MAX];
bool global::m_bWifiConnectCheck = true;
bool global::m_bWifiTryWPS = false;
bool global::m_bWifiConnected = false;
bool global::m_bWifiDirectConnected = false;
bool global::m_bWifiTryConnect = false;
bool global::m_bWifiTryReConnect = false;
bool global::m_bForceWifiTryWPS = FORCE_WIFIWPS_FIRSTTIME;
bool global::m_bRestoreLastLedMode = false;
#if defined(ENABLE_WIFI_AP)
bool global::m_bWifiProvisioningEnabled = true;
byte global::m_bWifiProvisioningTimeoutMinutes = WIFI_PROVISIONING_BOOT_WINDOW_DEFAULT_MINUTES;
bool global::m_bWifiProvisioningActive = false;
bool global::m_bWifiProvisioningPending = false;
unsigned long global::m_lWifiProvisioningStarted = 0;
unsigned long global::m_lWifiProvisioningDeadline = 0;
char global::m_wifi_provisioning_token[WIFI_PROVISIONING_TOKEN_MAX];
char global::m_wifi_provisioning_ssid[WLAN_SSID_MAX];
char global::m_wifi_provisioning_password[WLAN_KEY_MAX];
bool global::m_bWifiProvisioningPhysical = false;
char global::m_szClaimKey[DEVICE_CLAIM_KEY_LEN + 1] = "";
#endif

char global::m_DebugSyslogServer[STR_IPNAME_MAX];
char global::m_DebugSyslogAppName[STR_IPNAME_MAX];
int global::m_iDebugsyslogport = SYSLOG_PORT;

# if ESP32_DEVICE_TYPE == ESP32_BRIDGE
// other default for bridge
byte global::m_bButtonFunction = BTN_MODE_SMART_HUE_TOGGLE;
# else
#   if defined(ENABLE_CAM)
byte global::m_bButtonFunction = BTN_MODE_LED_CAM;
#   else
byte global::m_bButtonFunction = BTN_MODE_OFF;
#   endif
# endif
bool global::m_bButtonPressed = false;
unsigned long global::m_lButtonPressedStart = 0;

byte global::m_bWifiFailHandling = WIFI_FAIL_RECONNECT;

#if defined(ENABLE_CAM)
bool global::m_bCameraPrepareOK = false;
bool global::m_bCameraOK = false;
int global::m_iCamPerformance = FPS_35;        // Default = 35FPS
int global::m_iCamIndex = 0;              // Default = 160x120
# if defined(CAM_XCLK_FREQ)
int global::m_iCamFreq = CAM_XCLK_FREQ;   // Set propper default based on CAM module!
# else
int global::m_iCamFreq = FREQ_10;         // Default = 10 MHZ
# endif
int global::m_iCamFrameBuffer = CAM_BUFFER_COUNT;
int global::m_iCamFlicker = FLICKER_AUTO;
# if CAM_SENSOR == CAM_SENSOR_OV3660
byte global::m_bCamOverclock = 0;
# endif
byte global::m_bCamColorCorRed = 100;
byte global::m_bCamColorCorGreen = 100;
byte global::m_bCamColorCorBlue = 100;
byte global::m_bCamActivationThreshold = BLACKACTTHRESHOLD_DEFAULT;
byte global::m_bCamMinPixelAmount = MIN_DEFAULT_PIXEL_AMOUNT;
byte global::m_bCamBlackRemoveThreshold = 5;
int global::m_iCamContrast = 0;
int global::m_iCamBrightness = CAM_ADPT_BRIGHTNESS;
int global::m_iCamSaturation = 0;
int global::m_iCamSpecialEffect = 0;
int global::m_iCamWbMode = 2;                         // "Cloudy" as default
int global::m_iCamQuality = (psramFound() ? 6 : 12);  // Default quality 5 is fine
int global::m_iCamAutoWhitebalance = 1;               // Enable AutoWhitebalance Control
int global::m_iCamAwbGainControl = 1;         // Enable Autowhitebalance Gain!
int global::m_iCamVFlip = 0;                  // Disable VVlip
int global::m_iCamHMirror = 0;                // Disable HMireror
int global::m_bCamHighQuality = 0;            // High Quality Mode
int global::m_iCamBPC = 1;                    // Black Pixel Correction
int global::m_iCamWPC = 1;                    // White Pixel Correction
int global::m_iCamDenoise = 1;                // Denoise Control
int global::m_iCamLenc = 0;                   // Lens Correction
int global::m_iCamRawGMA = 1;                 // Raw Gamma Enable
int global::m_iCamDcw = 1;                    // Downsize
int global::m_iCamGaincontrol = 1;            // Auto Gain Control
int global::m_iCamAgcGain = CAM_ADPT_GAIN_DEFAULT;  // Default Manual Gain Level
int global::m_iCamAutoExposurecontrol = 0;    // Auto exposure control
int global::m_iCamAec2 = 0;                   // Disable Aec2!
int global::m_iCamAecValue = 1200;            // Set AEC Value default
int global::m_iCamAeLevels = 0;               // "-2" as default
int global::m_iCamAdaptiveExposurecontrol = 0;// Internal Exposure Control handling!
int global::m_iCamAecMinValue = CAM_ADPT_AEC_MIN;                       // Minimal Aec Value when Internal Exposure handling is used
int global::m_iCamAecMaxValue = CAM_ADPT_AEC_MAX;                       // Maximal Aec Value when Internal Exposure handling is used
int global::m_iCamAdaptiveChangePercent = CAM_WHITE_AEC_CHANGEPERC;     // Aec change Value when Internal Exposure handling is used
int global::m_iCamAdaptiveChangeDelay = CAM_ADPT_CHANGE_DELAY;          // Change Delay before going UP/down with AEC!
int global::m_iCamAdaptiveGaincontrol = 0;                              // Internal Exposure Control handling!
byte global::m_bCamWhiteActivationTreshold = WHITEACTTHRESHOLD_DEFAULT; // HIGH ColorLight Threshold Value that triggers AEC DOWNSCALE
byte global::m_bCamBeforeWhiteActivationTreshold = CAM_ADPT_BEFORE_ACT; // LOW ColorLight Threshold Value that triggers AEC UPSCALE
byte global::m_bCamWhiteMinimumAmount = CAM_ADPT_WHITE_MIN;             // HIGH ColorLight minimum Amount in percent of Sectors that have to be above Threshold value
byte global::m_bCamAdaptiveGainDown = CAM_ADPT_GAIN_DOWN;               // Threshold value for outside pixels, if outside pixels fall below this value, Gainlevel is reduced by ONE.
byte global::m_bCamAdaptiveGainUp = CAM_ADPT_GAIN_UP;                   // Threshold value for inside pixels, if inside pixels fall below this value AND outside pixels are below CamAdaptiveGainDown
byte global::m_bCamAdaptiveGainMax = CAM_ADPT_GAIN_MAX;                 // Max Gain Level for adaptive gain control mode

byte global::m_bCamAutoOnOff = 0;                                       // Default OFF
int global::m_iCamAutoOnOffDelay = BLACK_AUTOONOFFSECONDS;              // Auto on off delay!
byte global::m_bCamJpegCalcDirect = 1;// 1 = On the fly CALC - FASTER! 0 = Convert to BMP Data first then process, needs more memory and is slower!

#endif
#if defined(ENABLE_HUE)
HueHelper* global::m_pHueHelper = NULL;
#endif
#if defined(ENABLE_DREAMSCREEN)
DreamScreen* global::m_pDreamScreen = NULL;
#endif
#if defined(ENABLE_RELAY)
char global::m_ambiback_devices_name[AMBIBACK_MAX_DEVICES][DEVICE_NAME_MAX];
char global::m_ambiback_devices_id[AMBIBACK_MAX_DEVICES][AMBIBACK_MAX_IDLEN];
char global::m_ambiback_devices_mac[AMBIBACK_MAX_DEVICES][AMBIBACK_MAX_IDLEN];
char global::m_ambiback_devices_ip[AMBIBACK_MAX_DEVICES][AMBIBACK_MAX_IDLEN];
byte global::m_ambiback_devices_count = 0;
char global::m_ambiback_selected_device[AMBIBACK_MAX_IDLEN];
bool global::m_ambiback_relaymode_enabled = false;
bool global::m_ambiback_relaymode_initialized = false;
byte global::m_ambiback_relaymode_direct = 1;
byte global::m_ambiback_relay_mode = RELAY_UDP;
byte global::m_ambiback_relay_wifidirect_enabled = 0;
int global::m_ambiback_relay_failed = 0;
bool global::m_ambiback_discovery_send = false;
IPAddress global::m_ambiback_relaymode_ipaddr;
char global::m_ambiback_relay_ssid[WLAN_SSID_MAX];
char global::m_ambiback_relay_password[WLAN_KEY_MAX];
#endif

// STATIC HTTP CLIENT for URL Calls
HTTPClient mHttp;

// --- PUBLIC
// Task related helpers
bool g_bPwrLedState = false;
byte g_bPwrRed = 0;
byte g_bPwrGreen = 0;
byte g_bPwrBlue = 0;
bool g_bCtlLedState = false;
byte g_bCtlRed = 0;
byte g_bCtlGreen = 0;
byte g_bCtlBlue = 0;
// ---

// --- CHECKSUM Helper
#define    CRC_START_8   0x00
/*
   static uint8_t sht75_crc_table[];

   The SHT75 humidity sensor is capable of calculating an 8 bit CRC checksum to
   ensure data integrity. The lookup table crc_table[] is used to recalculate
   the CRC.
*/
static uint8_t sht75_crc_table[] = {
  0,   49,  98,  83,  196, 245, 166, 151, 185, 136, 219, 234, 125, 76,  31,  46,
  67,  114, 33,  16,  135, 182, 229, 212, 250, 203, 152, 169, 62,  15,  92,  109,
  134, 183, 228, 213, 66,  115, 32,  17,  63,  14,  93,  108, 251, 202, 153, 168,
  197, 244, 167, 150, 1,   48,  99,  82,  124, 77,  30,  47,  184, 137, 218, 235,
  61,  12,  95,  110, 249, 200, 155, 170, 132, 181, 230, 215, 64,  113, 34,  19,
  126, 79,  28,  45,  186, 139, 216, 233, 199, 246, 165, 148, 3,   50,  97,  80,
  187, 138, 217, 232, 127, 78,  29,  44,  2,   51,  96,  81,  198, 247, 164, 149,
  248, 201, 154, 171, 60,  13,  94,  111, 65,  112, 35,  18,  133, 180, 231, 214,
  122, 75,  24,  41,  190, 143, 220, 237, 195, 242, 161, 144, 7,   54,  101, 84,
  57,  8,   91,  106, 253, 204, 159, 174, 128, 177, 226, 211, 68,  117, 38,  23,
  252, 205, 158, 175, 56,  9,   90,  107, 69,  116, 39,  22,  129, 176, 227, 210,
  191, 142, 221, 236, 123, 74,  25,  40,  6,   55,  100, 85,  194, 243, 160, 145,
  71,  118, 37,  20,  131, 178, 225, 208, 254, 207, 156, 173, 58,  11,  88,  105,
  4,   53,  102, 87,  192, 241, 162, 147, 189, 140, 223, 238, 121, 72,  27,  42,
  193, 240, 163, 146, 5,   52,  103, 86,  120, 73,  26,  43,  188, 141, 222, 239,
  130, 179, 224, 209, 70,  119, 36,  21,  59,  10,  89,  104, 255, 206, 157, 172
};
/*
   uint8_t crc_8( const unsigned char *input_str, size_t num_bytes );

   The function crc_8() calculates the 8 bit wide CRC of an input string of a
   given length.
*/
uint8_t global::crc_8( byte* input_bytes, size_t num_bytes ) {
  uint8_t crc = CRC_START_8;
  byte* ptr = input_bytes;

  if ( ptr != NULL ) {
    for (size_t a = 0; a < num_bytes; a++) {
      crc = sht75_crc_table[(*ptr) ^ crc];
      ptr++;
    }
    //    _DBGPRINTF(PSTR("global:crc_8: Buffer %p, bytes=%d, checksum=0x%02X\n"), input_bytes, num_bytes, crc);
  } else {
    global::errprintf_P(PSTR(PSTR_GLOBAL "crc_8: INVALID Buffer %p, expected bytes=%d\n"), input_bytes, num_bytes);
  }
  return crc;
}  /* crc_8 */

/*
   uint8_t update_crc_8( unsigned char crc, unsigned char val );

   Given a databyte and the previous value of the CRC value, the function
   update_crc_8() calculates and returns the new actual CRC value of the data
   comming in.
*/
uint8_t global::update_crc_8( unsigned char crc, unsigned char val ) {
  return sht75_crc_table[val ^ crc];
}  /* update_crc_8 */
// ---

#if defined(ESP32)
void global::printEspPartitions() {
  const esp_partition_t* _partition = NULL;

  /* Print Boot Partition */
  _partition = esp_ota_get_boot_partition();
  if (_partition) {
    _DBGPRINTF_ULTRA(PSTR("global: printEspPartitions: Boot Partition: %s, Start Address=%d, Size=%d\n"),
          _partition->label,
          _partition->size,
          _partition->address);
    _partition = esp_ota_get_next_update_partition(_partition);
  }

  /* Print Running Partition */
  _partition = esp_ota_get_running_partition();
  if (_partition) {
    _DBGPRINTF_ULTRA(PSTR("global: printEspPartitions: Running Partition: %s, Start Address=%d, Size=%d\n"),
          _partition->label,
          _partition->size,
          _partition->address);
    _partition = esp_ota_get_next_update_partition(_partition);
  }

  /* Print OTA Partition(s) */
  _partition = esp_ota_get_next_update_partition(_partition);
  if (_partition) {
    _DBGPRINTF_ULTRA(PSTR("global: printEspPartitions: OTA Partition 1: %s, Start Address=%d, Size=%d\n"),
          _partition->label,
          _partition->size,
          _partition->address);
    _partition = esp_ota_get_next_update_partition(_partition);
    if (_partition) {
      _DBGPRINTF_ULTRA(PSTR("global: printEspPartitions: OTA Partition 2: %s, Start Address=%d, Size=%d\n"),
            _partition->label,
            _partition->size,
            _partition->address);
    }
  }

}

int global::getFreeSketchSpace() {
  const esp_partition_t* _partition = esp_ota_get_next_update_partition(_partition);
  int partsize = 0;
  if (_partition) {
    _DBGPRINTF_ULTRA(PSTR("global: getFreeSketchSpace: Partition: %s, Start Address=%d, Size=%d\n"),
          _partition->label,
          _partition->size,
          _partition->address);
  }
  return partsize;
}
void global::esp32Stophandler(void) {
  _DBGPRINTF(PSTR(PSTR_GLOBAL "esp32Stophandler: REBOOT!\n"));
}

void global::printEspMemUsage(const char* pszSource, const char* pszFunc) {
  _DBGPRINTF(PSTR("%s: %s: Free Memory INT/HEAP/SPIRAM=%dKB/%dKB/%dKB\n"),
    pszSource,
    pszFunc,
    heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL) / 1024,
    ESP.getFreeHeap() / 1024,
    (psramFound() ? heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM) / 1024 : 0)
  );
}

void global::Init() {
  // Init Random number generator
  esp_random();
  
  // Configure Power/control LED
#ifndef USE_WS2812_CONTROL_LEDS
  // Use simple LED Output
  if (PWRLED_PIN >= 0) pinMode(PWRLED_PIN, OUTPUT);
  if (CTLLED_PIN >= 0) pinMode(CTLLED_PIN, OUTPUT);
#endif
  pinMode(BTN_PIN, INPUT);

  // Check for PSRAM
  // Disable default-malloc PSRAM redirect: some library/free paths can then free()
  // a pointer not recognized by heap_caps_free and trigger "target pointer is outside heap areas".
  // Our own PSRAM use is via _malloc_psram() and is unchanged.
  // if (psramFound()) {
  //   heap_caps_malloc_extmem_enable(SYS_ALLOC_PSRAM_MIN);
  // }

  // Init Helpers
  global::m_pSyslog = NULL;
  global::m_pLedHelper = NULL;
  global::m_pEepromHelper = NULL;
  global::m_pWebServer = NULL;

  global::m_bSettingsChanged = false;
  global::m_bNetworkStarted = false;

  // Init String variables
  global::m_wifi_ssid[0] = '\0';
  global::m_wifi_password[0] = '\0';
  global::m_bRestoreLastLedMode = false;
#if defined(ENABLE_WIFI_AP)
  global::m_wifi_provisioning_token[0] = '\0';
  global::m_wifi_provisioning_ssid[0] = '\0';
  global::m_wifi_provisioning_password[0] = '\0';
  global::m_bWifiProvisioningEnabled = true;
  global::m_bWifiProvisioningTimeoutMinutes = WIFI_PROVISIONING_BOOT_WINDOW_DEFAULT_MINUTES;
  global::m_lWifiProvisioningStarted = 0;
  global::m_lWifiProvisioningDeadline = 0;
  global::m_bWifiProvisioningActive = false;
  global::m_bWifiProvisioningPending = false;
#endif
  global::m_deviceid[0] = '\0';
  global::m_deviceidmac[0] = '\0';
  memcpy(global::m_devicename, DEVICE_NAME, strlen(DEVICE_NAME));
#if defined(ENABLE_RELAY)
  global::m_ambiback_selected_device[0] = '\0';
#endif
  global::m_szCurClientIP[0] = '\0';
  global::m_CurClientAddr = 0;
  
  // Debug Parameters
  //  global::m_DebugSyslogServer[0] = '\0';
  snprintf_P(global::m_DebugSyslogServer, STR_IPNAME_MAX, PSTR("%s"), SYSLOG_SERVER); // Copy default Syslog Server

  // Get DeviceID using ESP API directly - is UNIQUE
  uint8_t chipid[8] = {0};
# if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  // ESP32 3.x uses Arduino API
  uint64_t mac = ESP.getEfuseMac();
  chipid[0] = (uint8_t)(mac & 0xFF);
  chipid[1] = (uint8_t)((mac >> 8) & 0xFF);
  chipid[2] = (uint8_t)((mac >> 16) & 0xFF);
  chipid[3] = (uint8_t)((mac >> 24) & 0xFF);
  chipid[4] = (uint8_t)((mac >> 32) & 0xFF);
  chipid[5] = (uint8_t)((mac >> 40) & 0xFF);
# else
  esp_efuse_mac_get_default((uint8_t*)(&chipid));
# endif

  // Correctly use the first 4 bytes in reverse ORDER like done before!
  snprintf_P(global::m_deviceid, DEVICE_ID_MAX, PSTR("0XFF00%02X%02X%02X%02X"), chipid[3], chipid[2], chipid[1], chipid[0]);
  
  // Get Full MAC ID!
  uint8_t derived_mac_addr[6] = {0};
# if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  // ESP32 3.x - use Arduino API to get WiFi MAC
  uint64_t wifiMac = ESP.getEfuseMac();
  // WiFi STA MAC is typically the base MAC (no offset in default config)
  for (int i = 0; i < 6; i++) {
    derived_mac_addr[i] = (uint8_t)((wifiMac >> (i * 8)) & 0xFF);
  }
# else
  esp_read_mac(derived_mac_addr, ESP_MAC_WIFI_STA);
# endif
  snprintf_P(global::m_deviceidmac, DEVICE_MAC_MAX, PSTR("%02X%02X%02X%02X%02X%02X"), derived_mac_addr[0], derived_mac_addr[1], derived_mac_addr[2],
           derived_mac_addr[3], derived_mac_addr[4], derived_mac_addr[5]);

  _DBGPRINTF(PSTR(PSTR_GLOBAL "Init InternalID='0x%s' Mac='0x%s' PSRAM: %s\n"),
                      global::m_deviceid, global::m_deviceidmac, psramFound() ? "YES" : "NO");

  // Init Syslog APP Name
  snprintf_P(m_DebugSyslogAppName, STR_IPNAME_MAX, PSTR("AMBIBACK_%s"), global::GetDeviceID() );

#if defined(ENABLE_CAM)
  /* Cam geometry buffers are owned/initialized by CamProcessor. */
#endif

#if defined(ENABLE_HUE)
  if (global::m_pHueHelper == NULL)
    global::m_pHueHelper = new HueHelper();
#endif

#if defined(ENABLE_DREAMSCREEN)
  if (global::m_pDreamScreen == NULL)
    global::m_pDreamScreen = new DreamScreen(DREAMSCREEN_UDP_PORT);
#endif

#if defined(ESP32) && defined(ESP32_REDIR_DEBUG)
  esp_log_level_set("*", ESP32_DEBUG_LEVEL);

  // Verbose cmaera loggging
  esp_log_level_set("camera", ESP_LOG_INFO/*ESP_LOG_VERBOSE*/);
  esp_log_level_set("camera_xclk", ESP_LOG_INFO/*ESP_LOG_VERBOSE*/);
  esp_log_level_set("esp_jpg_decode", ESP_LOG_VERBOSE);
# if CAM_SENSOR == CAM_SENSOR_OV3660
  esp_log_level_set("ov3660", ESP_LOG_INFO/*ESP_LOG_VERBOSE*/);
# else
  esp_log_level_set("ov2640", ESP_LOG_INFO/*ESP_LOG_VERBOSE*/);
# endif
  // Redirect ESP Debug
  log_v("AMBIBACK", "BEFORE REDIRECT\n");
  esp_log_set_vprintf(global::dbgprintf_esp);
  log_v("AMBIBACK", "AFTER REDIRECT\n");
#endif

#if defined(ESP32) && defined(ESP32_EXTRA_DIAG)
  //  Only ESP-IDF 4.x
  //  heap_caps_register_failed_alloc_callback(global::heap_caps_alloc_failed_hook);
#endif

  // Set serial Debugging default
  global::SetSerialEnabled(_DEBUG_SERIAL);

  // Set UDP debugging default
  global::SetSyslogEnabled(_DEBUG_UDP);

  esp_err_t err = esp_register_shutdown_handler((shutdown_handler_t)esp32Stophandler);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    _DBGPRINTF(PSTR(PSTR_GLOBAL "Init shutdown_handler FAILED: %d\n"), err);
  } else {
    _DBGPRINTF(PSTR(PSTR_GLOBAL "Init shutdown_handler SUCCESS\n"));
  }
}

void global::CalcFramesCtr() {
  /* Update paket counters */
  _DBGPRINTF_ULTRA(PSTR("global:CalcFramesCtr PKTs: %d/%d /s \n"), global::GetCurrentFramesCtr(), global::GetCurrentDroppedCtr() );
  global::SetCurrentPkts(global::GetCurrentFramesCtr());
  global::SetCurrentPktsDropped(global::GetCurrentDroppedCtr());

  // Reset back to 0
  global::SetCurrentFramesCtr(0);
  global::SetCurrentDroppedCtr(0);
}

void global::SetClientConnected(bool bNew) { 
  global::m_bClientConnected = bNew;
# if defined(ENABLE_WIFI_NAT)
  if (!m_bClientConnected) {
    // Remove Port Mapping
    global::RemovePortMappingWifiAPNAT(HTTP_PORT_FORWARD);
  }
# endif
}

void global::SetClientConnectedIpStr(IPAddress* pIPClient) {
  if (pIPClient == NULL) {
    global::m_szCurClientIP[0] = '\0';
    global::m_CurClientAddr = 0;
  } else {
    // Copy IP Address over!
    strncpy(&global::m_szCurClientIP[0], (const char *) pIPClient->toString().c_str(), STR_IP_MAX);
    global::m_CurClientAddr = *pIPClient; // Auto converts to u32_t
# if defined(ENABLE_WIFI_NAT)
    if (global::GetClientConnected()) {
      // Add Portmapping for HTTP_PORT_FORWARD
      global::AddPortMappingWifiAPNAT(HTTP_PORT_FORWARD, global::m_CurClientAddr, HTTP_PORT);
    }
# endif    
  }
}

size_t global::GetDeviceInfo(char* pszIn, size_t stLen) {
  IPAddress ip = WiFi.localIP();
  stLen = snprintf_P(pszIn, stLen, PSTR("{\n"
                                        "\t\"DeviceType\": \"%s\",\n"
                                        "\t\"DeviceVersion\": \"%s\",\n"
                                        "\t\"DeviceID\": \"%s\",\n"
                                        "\t\"DeviceMAC\": \"%s\",\n"
                                        "\t\"DeviceName\": \"%s\",\n"
                                        "\t\"DeviceIP\": \"%d.%d.%d.%d\",\n"
                                        "\t\"LedStatus\": \"%d\",\n"
# if defined(ENABLE_RELAY)
                                        "\t\"WifiDirectConnected\": \"%d\",\n" 
                                        "\t\"WifiDirectAP\": \"%s\",\n" 
# endif
                                        "\t\"Finished\": \"1\"\n"
                                        "}\n"),
                     AMBIBACK_NAME,
                     AMBIBACK_VERSION,
                     global::GetDeviceID(),
                     global::GetDeviceIDMac(),
                     global::GetDeviceName(),
                     ip[0], ip[1], ip[2], ip[3],  // Format IP directly without String
                     global::m_pLedHelper->GetLedMode()
# if defined(ENABLE_RELAY)
                     , global::GetWifiDirectConnected()
                     , (global::GetWifiDirectConnected() == 1 ? global::m_ambiback_selected_device : "")
# endif
                    );
  return stLen;
}

void global::OTAUpdateStarted() {
  _DBGPRINTF(PSTR(PSTR_GLOBAL "OTAUpdateStarted: ENTER\n"));
  global::g_httpUpdateCheck = 102400; // 100KB Chunks
  global::TogglePwrLed(0, 127, 255);
}

void global::OTAUpdateFinished() {
  _DBGPRINTF(PSTR(PSTR_GLOBAL "OTAUpdateFinished\n"));
  global::TogglePwrLed(0, 255, 0);
}

void global::OTAUpdateProgress(int cur, int total) {
  if (cur > global::g_httpUpdateCheck) {
    _DBGPRINTF(PSTR(PSTR_GLOBAL "OTAUpdate: %d/%d bytes\n"), cur, total);
    global::TogglePwrLed(0, 127, 255);
    global::g_httpUpdateCheck += 102400; // Next 100KB
  }
}

void global::OTAUpdateError(int err) {
  _DBGPRINTF(PSTR(PSTR_GLOBAL "OTAUpdate ERROR: %d\n"), err);
}

void global::StartHttpUpdate() {
  _DBGPRINTF(PSTR(PSTR_GLOBAL "StartHttpUpdate ENTER\n"));
  // --- ENABLE DEBUG!
  // esp_log_level_set("*", ESP_LOG_VERBOSE);
  // --- ENABLE DEBUG!

  // Prepare HTTPUpdate
  ambihttpUpdate.onStart(global::OTAUpdateStarted);
  ambihttpUpdate.onEnd(global::OTAUpdateFinished);
  ambihttpUpdate.onProgress(global::OTAUpdateProgress);
  ambihttpUpdate.onError(global::OTAUpdateError);
  ambihttpUpdate.rebootOnUpdate(false); // We reboot ourselfes
# if !defined(USE_WS2812_CONTROL_LEDS)
  if (PWRLED_PIN > 0) {
    ambihttpUpdate.setLedPin(PWRLED_PIN, HIGH);
  }
# endif

  char updarteurl[MAX_URL_SIZE];
  snprintf_P(updarteurl, MAX_URL_SIZE, PSTR("%s/%s/update.bin"), HTTP_BASE_URL, global::m_pWebServer->GetUpdateVersion() );
  _DBGPRINTF(PSTR(PSTR_GLOBAL "StartHttpUpdate: %s\n"), updarteurl);

  // Create Wifi Client
  WiFiClient updateclient;
  updateclient.setTimeout(12000 / 1000); // 5 seconds timeout only!:) 
  
  /*    // Create Secure Wifi Client
    WiFiClientSecure updateclient;
    updateclient.setCACert(rootCACertificate);
    updateclient.setTimeout(12000 / 1000); // timeout argument is defined in seconds for setTimeout */

  // Firmware update running!
  global::SetFirmwareRunning( true );
  global::DisableArduinoCoreWatchDog();

  _DBGPRINTF(PSTR(PSTR_GLOBAL "StartHttpUpdate: FreeMem=%dKB FreePSRAM:%dKB Heap:%dB Free:%dB Max:%dB\n"),
                      heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL) / 1024,
                      (psramFound() ? heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024 : 0),
                      ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMaxAllocHeap() );

  // Perform Update now
  t_ambihttpUpdate_return ret = ambihttpUpdate.update(updateclient, updarteurl);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      _DBGPRINTF(PSTR(PSTR_GLOBAL "StartHttpUpdate ERROR (%d): %s\n"), ambihttpUpdate.getLastError(), ambihttpUpdate.getLastErrorString().c_str());
      global::m_pWebServer->SetUpdateError("Firmware Update ERROR"); // + ambihttpUpdate.getLastError() + ": " + ambihttpUpdate.getLastErrorString();

      global::SetFirmwareRunning(false);
      break;
    case HTTP_UPDATE_NO_UPDATES:
      _DBGPRINTF(PSTR(PSTR_GLOBAL "StartHttpUpdate: NO UPDATE\n"));
      global::m_pWebServer->SetUpdateError("Firmware Update ERROR NO UPDATE");

      global::SetFirmwareRunning(false);
      break;
    case HTTP_UPDATE_OK:
      _DBGPRINTF(PSTR(PSTR_GLOBAL "StartHttpUpdate: Update OK!\n"));
      global::m_pWebServer->SetUpdateError("");

      global::SetFirmwareRunning(false);
      global::SetShouldReboot(true);
      break;
  }

  _DBGPRINTF(PSTR(PSTR_GLOBAL "StartHttpUpdate EXIT\n"));
}

void global::DisableArduinoCoreWatchDog() {
  _DBGPRINTF_ULTRA(PSTR("global:DisableArduinoCoreWatchDog Disable WDT on CORE1\n"));

  // Disable fucking Watchdog!
  disableCore1WDT();
}

void global::ScheduleReboot(unsigned long lDelayMs) {
  global::m_shouldReboot = true;
  global::m_lRebootAt = millis() + lDelayMs;
  _DBGPRINTF(PSTR("global:ScheduleReboot: reboot scheduled in %lu ms\n"), lDelayMs);
}

void global::EnableArduinoCoreWatchDog() {
  _DBGPRINTF_ULTRA(PSTR("global:EnableArduinoCoreWatchDog Disable WDT on CORE1\n"));

  // Enable fucking Watchdog!
  enableCore1WDT();
}

void global::DisableSysCoreWatchDog() {
  /* Disable Watchdog if we run on Syslog Core */
  if (xPortGetCoreID() == SYSTEM_RUNNING_CORE) {
    _DBGPRINTF_ULTRA(PSTR("global:DisableSysCoreWatchDog Disable WDT on CORE0\n"));

    // Disable fucking Watchdog!
    disableCore0WDT();
  }
}

void global::EnableSysCoreWatchDog() {
  if (xPortGetCoreID() == SYSTEM_RUNNING_CORE) {
    _DBGPRINTF_ULTRA(PSTR("global:EnableSysCoreWatchDog Disable WDT on CORE0\n"));

    // Enable fucking Watchdog!
    enableCore0WDT();
  }
}

void global::CalcDynamicDelayLength(unsigned long lLastUpdate) {
  // --- Calc Dynamic DelayLength
  unsigned long lTimeDifference = millis() - lLastUpdate;
  int iDelayLength = int((lTimeDifference / DEFAULT_FRAME_LENGTH) * DEFAULT_FRAME_LENGTH); // Round DelayLength to frame length

  // Max Limit Delay to MAX_RELAY_DELAY_LENGTH !
  /*
    #if defined(ENABLE_RELAY)
    // Lets use at least 40ms delay in relay mode for now!
    if (global::GetAmbibackRelaymodeEnabled()) {
      if (iDelayLength < (DEFAULT_RELAY_FRAME_LENGTH*4))
        iDelayLength = (DEFAULT_RELAY_FRAME_LENGTH*3);
      if (global::m_pLedHelper->GetFrameLength() != DEFAULT_RELAY_FRAME_LENGTH)
        global::m_pLedHelper->SetFrameLength(DEFAULT_RELAY_FRAME_LENGTH, false);
    } else if (iDelayLength > MAX_RELAY_DELAY_LENGTH) {
    #else
    // Limit Delay to 100ms!
    if (iDelayLength > MAX_RELAY_DELAY_LENGTH) {
    #endif
  */
  // Limit Delay to MAX_RELAY_DELAY_LENGTH!
  if (iDelayLength > MAX_DELAY_LENGTH) {
    iDelayLength = MAX_DELAY_LENGTH;
  } else if (iDelayLength <= DEFAULT_FRAME_LENGTH * 2) {
    iDelayLength = DEFAULT_FRAME_LENGTH * 3;
  } else if (iDelayLength > global::m_pLedHelper->GetDelayLength()) {
    // Smooth Delay change
    iDelayLength += int((((iDelayLength - global::m_pLedHelper->GetDelayLength()) / 2) / DEFAULT_FRAME_LENGTH) * DEFAULT_FRAME_LENGTH);
  }
  // Set new dynamic Delay Length if necessary, but do NOT save in EEPROM
  if (iDelayLength != global::m_pLedHelper->GetDelayLength()) {
    global::m_pLedHelper->SetDelayLength(iDelayLength, false);
  }
  // ---
}

void global::SetSyslogEnabled(bool bNew) {
  m_bSyslogEnabled = bNew;
  if ( m_bSyslogEnabled == false ) {
    // Stop Syslog Debugging
    global::DeInitSyslogClient();
  } else {
    // Start Syslog Debugging
    global::InitSyslogClient();
  }
}




#if defined(ENABLE_HUE)
void global::SetHuePreset(int iIndex, bool bEpprom) {
  if (m_pHueHelper) m_pHueHelper->SetPreset(iIndex, bEpprom);
}
#endif

#if defined(ENABLE_CAM)
bool global::PrepareInitCam() {
  using namespace esp32cam;
  // Prepare Init Cam
  _DBGPRINTF(PSTR("global:PrepareInitCam:ENTER\n"));

  if (global::m_bCameraPrepareOK) {
    // Init already done
    return true;
  }

# if ESP32_DEVICE_TYPE == ESP32_WROOVER_CAM // ESP32 Wroover CAM Module! 
  _DBGPRINTF(PSTR("global:PrepareInitCam:ESP32_WROOVER_CAM PINOUT\n"));
  m_CamCfg.setPins(pins::AiThinker);
# elif defined(ESP32_CUSTOM_CAM)
  _DBGPRINTF(PSTR("global:PrepareInitCam:ESP32_CUSTOM_CAM PINOUT %d\n"), ESP32_CUSTOM_CAM);
  // Custom ESP32 CAM Module!
  Pins PinsCustomCam{
    D0: Y2_GPIO_NUM,
    D1: Y3_GPIO_NUM,
    D2: Y4_GPIO_NUM,
    D3: Y5_GPIO_NUM,
    D4: Y6_GPIO_NUM,
    D5: Y7_GPIO_NUM,
    D6: Y8_GPIO_NUM,
    D7: Y9_GPIO_NUM,
    XCLK: XCLK_GPIO_NUM,
    PCLK: PCLK_GPIO_NUM,
    VSYNC: VSYNC_GPIO_NUM,
    HREF: HREF_GPIO_NUM,
    SDA: SIOD_GPIO_NUM,
    SCL: SIOC_GPIO_NUM,
    RESET: RESET_GPIO_NUM,
    PWDN: PWDN_GPIO_NUM,
  };
  m_CamCfg.setPins(PinsCustomCam);

#   if ESP32_CUSTOM_CAM == TTGO_T_CAMERA_MIC_V16
  /* IO13, IO14 is designed for JTAG by default,
     to use it as generalized input,
     firstly declair it as pullup input */
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#   endif

# else
  global::m_bCameraPrepareOK = false;
  global::m_bCameraOK = false;
  global::errprintf_P(PSTR("global:PrepareInitCam:UNKNOWN BOARD TYPE\n"));
  return false;
# endif

  // Use multi buffering!
  m_CamCfg.setBufferCount(global::m_iCamFrameBuffer); /* Init Framebuffer Count and preserve memory! > CAM_BUFFER_COUNT */

  // Set Camera frequence!
  m_CamCfg.setFreqHz( (global::GetCamFreqMhz() * 1000000U) ); // Set to configure Mhz

  /*
      FRAMESIZE_QQVGA,    // 160x120
      FRAMESIZE_QQVGA2,   // 128x160
      FRAMESIZE_QCIF,     // 176x144
      FRAMESIZE_HQVGA,    // 240x176
      FRAMESIZE_QVGA,     // 320x240
      FRAMESIZE_CIF,      // 400x296
      FRAMESIZE_VGA,      // 640x480
      FRAMESIZE_SVGA,     // 800x600
      FRAMESIZE_XGA,      // 1024x768
      FRAMESIZE_SXGA,     // 1280x1024
      FRAMESIZE_UXGA,     // 1600x1200
      FRAMESIZE_QXGA,     // 2048*1536
      FRAMESIZE_INVALID
  */

  // Init with higher resolution
  if (psramFound()) {
    // Force memory on PSRAM!
    m_CamCfg.setJpeg(98);
    //    cfg.setResolution(Resolution::find(2048, 1536)); // MAX
    m_CamCfg.setResolution(Resolution::find(800, 600));
  } else {
    // Limit on small resolution without PSRAM!
    m_CamCfg.setJpeg(90);
    m_CamCfg.setResolution(Resolution::find(160, 120));
  }

# if defined(ESP32_EXTRA_DIAG)
/*  // DEBUG Output Resolutions
  Resolution res;
  for (int i = 0; i < FRAMESIZE_INVALID; res.m_frameSize++) {
    _DBGPRINTF(PSTR("global:PrepareInitCam:Found Resolution %d/%d \n"), res.getWidth(), res.getHeight());
  }
  */
# endif  

  global::m_bCameraPrepareOK = true;
  return true;
}

void global::DeInitCam(){
  using namespace esp32cam;
  _DBGPRINTF(PSTR("global:DeInitCam:ENTER\n"));
  
  // DeInit Device Cam
  Camera.end();
  _DBGPRINTF(PSTR("global:DeInitCam:Camera deinit Status=%#08x(%s) ESP32_DEVICE_TYPE=%d\n"), Camera.lastEspErr, (global::m_bCameraOK ? "OK" : esp_err_to_name(Camera.lastEspErr)), ESP32_DEVICE_TYPE);

  // needs to be initalized again!
  global::m_bCameraOK = false;
}

void global::InitCam() {
  using namespace esp32cam;
  _DBGPRINTF(PSTR("global:InitCam:ENTER\n"));
  
  // Prepare Init Cam
  if (global::PrepareInitCam()) {
    // Init Camera
    global::m_bCameraOK = Camera.begin(m_CamCfg);
    _DBGPRINTF(PSTR("global:InitCam:Camera Status=%#08x(%s) ESP32_DEVICE_TYPE=%d\n"), Camera.lastEspErr, (global::m_bCameraOK ? "OK" : esp_err_to_name(Camera.lastEspErr)), ESP32_DEVICE_TYPE);
  
    // Set configured properties!
    global::SetCamResolution(global::m_iCamIndex, false);
    global::SetCamFreq(global::m_iCamFreq, false);
  # if CAM_SENSOR == CAM_SENSOR_OV3660
    global::SetCamFlicker(global::m_iCamFlicker, false);
  # endif
    global::SetCamQuality(global::m_iCamQuality, false);
    global::SetCamDcw(global::m_iCamDcw, false);
    global::SetCamHMirror(global::m_iCamHMirror, false);
    global::SetCamVFlip(global::m_iCamVFlip, false);
    global::SetCamHighQuality(global::m_bCamHighQuality, false);
  
    global::SetCamBPC(global::m_iCamBPC, false);
    global::SetCamWPC(global::m_iCamWPC, false);
    global::SetCamDenoise(global::m_iCamDenoise, false);
    global::SetCamSpecialEffect(global::m_iCamSpecialEffect, false);

  /*
    New - set our own Exposure control settings!
    global::SetCamAdaptiveExposurecontrol(global::m_iCamAdaptiveExposurecontrol, false);
    global::SetCamAecMinValue(global::m_iCamAecMinValue, false);
    global::SetCamAecMaxValue(global::m_iCamAecMaxValue, false);
    global::SetCamAdaptiveChangePercent(global::m_iCamAdaptiveChangePercent, false);
    global::SetCamAdaptiveChangeDelay(global::m_iCamAdaptiveChangeDelay, false);
    global::SetCamWhiteActivationTreshold(global::m_bCamWhiteActivationTreshold, false);
    global::SetCamAdaptiveGaincontrol(global::m_iCamAdaptiveGaincontrol, false);
    global::SetCamAdaptiveGainDown(global::m_bCamAdaptiveGainDown, false);
    global::SetCamAdaptiveGainUp(global::m_bCamAdaptiveGainUp, false);
  */

  // Finally set Campreset Settings
  global::SetCamPreset(global::m_iCamPreset, false);
  // called in SetCamPreset  global::SetExposurePreset(global::m_iExposurePreset, false);
  }    
}


void global::InitCamClck(int iMul, int iPre, int iPclk) {
  if (esp32cam::Camera.changePll(iMul, iPre, iPclk)) {
    _DBGPRINTF(PSTR("global:changePll:Change iMul=%d, iPre=%d, iPclk=%d SUCCESS !!!!!!\n"), iMul, iPre, iPclk);
  } else {
    _DBGPRINTF(PSTR("global:changePll:Change iMul=%d, iPre=%d, iPclk=%d FAIL !!!!!!\n"), iMul, iPre, iPclk);
  }
}

void global::SetCamPerformance(int iNew, bool bEpprom) {
  if (iNew >= CAMPERF_MINIDX && iNew <= CAMPERF_MAXIDX) {
    global::m_iCamPerformance = iNew;
    // Write to EEPROM
    if (bEpprom)
      global::m_pEepromHelper->SetSettingInt("CamPerf", iNew);
    _DBGPRINTF(PSTR("global:SetCamPerformance to %d\n"), global::m_iCamPerformance);
  } else {
    global::errprintf_P(PSTR("global:SetCamPerformance INVALID NUMBER %d\n"), iNew);
  }
}
int global::GetCamPerformanceFrameTime(void) {
  int iFrameTime = 30;
  switch (global::m_iCamPerformance) {
    case FPS_50:
      iFrameTime = 20;
      break;
    case FPS_45:
      iFrameTime = 22;
      break;
    case FPS_40:
      iFrameTime = 25;
      break;
    case FPS_35:
      iFrameTime = 28;
      break;
    case FPS_30:
      iFrameTime = 33;
      break;
    case FPS_25:
      iFrameTime = 40;
      break;
    case FPS_20:
      iFrameTime = 50;
      break;
  }
  return iFrameTime;
}

void global::SetCamResolution(int iIndex, bool bEpprom, bool bStoreVal) {
  if (iIndex >= RESOLUTION_MINIDX && iIndex <= RESOLUTION_MAXIDX ) {
    if (bStoreVal)
      global::m_iCamIndex = iIndex;
    if (!global::m_bCameraOK) {
      _DBGPRINTF_VERBOSE(PSTR("global:SetCamResolution:Value set but CAMERA NOT INITIATED YET.\n"));
      return;
    }
    auto newRes = esp32cam::Resolution::find(FRAMESIZE_WIDTH[iIndex], FRAMESIZE_HEIGHT[iIndex]);
    if (esp32cam::Camera.changeResolution(newRes, 0)) {
      _DBGPRINTF(PSTR("global:SetCamResolution:changeResolution SUCCESS INDEX = %d\n"), iIndex);

# if CAM_SENSOR == CAM_SENSOR_OV3660
      // Correct CLOCK | Increases FPS about 50% !
      if (global::m_iCamFreq == FREQ_20) {
        global::InitCamClck(15 /*iMul*/, 3 /*iPre*/, 30 /*iPclk*/);
      } else if (global::m_iCamFreq == FREQ_10) {
        if (global::m_bCamOverclock == 1) {
          global::InitCamClck(30 /*iMul*/, 3 /*iPre*/, 30 /*iPclk*/);
        } else {
          global::InitCamClck(25 /*iMul*/, 3 /*iPre*/, 30 /*iPclk*/);
        }
      }
# endif
      // Reset Cam HMirror and VFLIP!
      global::SetCamHMirror(global::m_iCamHMirror, false);
      global::SetCamVFlip(global::m_iCamVFlip, false);

      // Write to EEPROM
      if (bEpprom)
        global::m_pEepromHelper->SetSettingInt(CAM_INDEX_START, iIndex);
    } else {
      _DBGPRINTF(PSTR("global:SetCamResolution:Change FAIL\n"));
    }
  } else {
    _DBGPRINTF(PSTR("global:SetCamResolution:INVALID INDEX %d\n"), iIndex);
  }
}
int global::GetCamFreqMhz(void) {
  int iMhz = 20;
  switch (global::m_iCamFreq) {
    case FREQ_10:
      iMhz = 10;
      break;
    case FREQ_12:
      iMhz = 12;
      break;
    case FREQ_14:
      iMhz = 14;
      break;
    case FREQ_16:
      iMhz = 16;
      break;
    case FREQ_18:
      iMhz = 18;
      iMhz;
    case FREQ_20:
      iMhz = 20;
      break;
    case FREQ_22:
      iMhz = 22;
      break;
    case FREQ_24:
      iMhz = 24;
      break;
    case FREQ_25:
      iMhz = 25;
      break;
    case FREQ_26:
      iMhz = 26;
      break;
    case FREQ_27:
      iMhz = 27;
      break;
  }
  return iMhz;
}
void global::SetCamFreq(int iNew, bool bEpprom) {
  global::m_iCamFreq = iNew;
  // Write to EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingInt("CamFreq", iNew);

  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamFreq:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }

  int iMhz = global::GetCamFreqMhz();
  if (esp32cam::Camera.changeFrequence(iMhz)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamFreq:Change SUCCESS\n"));
  } else {
    _DBGPRINTF(PSTR("global:SetCamFreq:Change FAIL\n"));
  }
}
void global::SetCamFrameBuffer(int iNew, bool bEpprom) {
  if (iNew > 0 && iNew < (psramFound() ? CAM_BUFFER_COUNT_MAX : CAM_BUFFER_COUNT)) {
    global::m_iCamFrameBuffer = iNew;

    // Write to EEPROM
    if (bEpprom)
      global::m_pEepromHelper->SetSettingInt("CamFbBuf", iNew);

    // Init Gain helper memory
//    global::m_pLedHelper->SetCamFrameSmoothingMem((iNew == 0 ? false : true));
  } else {
    global::errprintf_P(PSTR("global:SetCamFrameBuffer INVALID FRAMEBUFFERCOUNT %d\n"), iNew);
  }
}

int global::GetCamBandMode() {
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:GetCamBandMode:Value cannot be obtained CAMERA NOT INITIATED YET.\n"));
    return 0;
  }
  return esp32cam::Camera.getBandMode();
}

void global::SetCamFlicker(int iNew, bool bEpprom) {
  global::m_iCamFlicker = iNew;
  // Write to EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingInt("CamFlicker", iNew);

  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamFlicker:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }

  if (esp32cam::Camera.changeBandMode(global::m_iCamFlicker)) {
    _yield();
    _DBGPRINTF(PSTR("global:SetCamFlicker:Change SUCCESS to %d - BandMode=%s\n"), global::m_iCamFlicker,  (global::GetCamBandMode() == 0 ? "60Hz" : "50Hz"));
  } else {
    _DBGPRINTF(PSTR("global:SetCamFlicker:Change FAIL to %d\n"), global::m_iCamFlicker);
  }
}

# if CAM_SENSOR == CAM_SENSOR_OV3660
void global::SetCamOverclock(byte bNew, bool bEpprom) {
  global::m_bCamOverclock = bNew;
  // Write to EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("CamOverclock", bNew);

  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamOverclock:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }

  // Correct CLOCK | Increases FPS about 50% !
  if (global::m_iCamFreq == FREQ_20) {
    global::InitCamClck(15 /*iMul*/, 3 /*iPre*/, 30 /*iPclk*/);
  } else if (global::m_iCamFreq == FREQ_10) {
    if (global::m_bCamOverclock == 1) {
      global::InitCamClck(30 /*iMul*/, 3 /*iPre*/, 30 /*iPclk*/);
    } else {
      global::InitCamClck(25 /*iMul*/, 3 /*iPre*/, 30 /*iPclk*/);
    }
  }
}
# endif

void global::SetCamColorCorRed(byte bNew, bool bEpprom) {
  global::m_bCamColorCorRed = bNew;
  // Write to EEPROM
  if (bEpprom) {
    // Save custom Color calibrations
    if (global::m_iCamPreset != CAM_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamColCrRed%d"), global::m_iCamPreset);
      global::m_pEepromHelper->SetSettingByte(&szSettingID[0], bNew);
    } else {
      global::m_pEepromHelper->SetSettingByte(CAM_COLOR_RED_START, bNew);
    }
  }
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamColorCorRed to %d\n"), global::m_bCamColorCorRed);
}
void global::SetCamColorCorGreen(byte bNew, bool bEpprom) {
  global::m_bCamColorCorGreen = bNew;
  // Write to EEPROM
  if (bEpprom) {
    // Save custom Color calibrations
    if (global::m_iCamPreset != CAM_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamColCrGreen%d"), global::m_iCamPreset);
      global::m_pEepromHelper->SetSettingByte(&szSettingID[0], bNew);
    } else {
      global::m_pEepromHelper->SetSettingByte(CAM_COLOR_GREEN_START, bNew);
    }
  }
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamColorCorGreen to %d\n"), global::m_bCamColorCorGreen);
}
void global::SetCamColorCorBlue(byte bNew, bool bEpprom) {
  global::m_bCamColorCorBlue = bNew;
  // Write to EEPROM
  if (bEpprom) {
    // Save custom Color calibrations
    if (global::m_iCamPreset != CAM_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamColCrBlue%d"), global::m_iCamPreset);
      global::m_pEepromHelper->SetSettingByte(&szSettingID[0], bNew);
    } else {
      global::m_pEepromHelper->SetSettingByte(CAM_COLOR_BLUE_START, bNew);
    }
  }
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamColorCorBlue to %d\n"), global::m_bCamColorCorBlue);
}
void global::SetCamActivationThreshold(byte bNew, bool bEpprom) {
  global::m_bCamActivationThreshold = bNew;
  // Write to EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte(CAM_COLOR_ACTRSHLD_START, bNew);
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamActivationThreshold to %d\n"), global::m_bCamActivationThreshold);
}
void global::SetCamMinPixelAmount(byte bNew, bool bEpprom) {
  global::m_bCamMinPixelAmount = bNew;
  // Write to EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("CamMinPixelAmnt", bNew);
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamMinPixelAmount to %d\n"), global::m_bCamMinPixelAmount);
}
void global::SetCamBlackRemoveThreshold(byte bNew, bool bEpprom) {
  global::m_bCamBlackRemoveThreshold = bNew;
  // Write to EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("CamBlackRmThres", bNew);
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamBlackRemoveThreshold to %d\n"), global::m_bCamBlackRemoveThreshold);
}

void global::SetCamAutoOnOff(byte bNew, bool bEpprom) {
  global::m_bCamAutoOnOff = bNew;
  // Write to EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("CamAutoOnOff", bNew);
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamAutoOnOff to %d\n"), global::m_bCamAutoOnOff);
}
void global::SetCamAutoOnOffDelay(int iNew, bool bEpprom) {
  if (iNew > 0 && iNew < 3600) {
    global::m_iCamAutoOnOffDelay = iNew;
    // Write to EEPROM
    if (bEpprom)
      global::m_pEepromHelper->SetSettingInt("CamAutoOODelay", iNew);
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamAutoOnOffDelay to %d\n"), global::m_iCamAutoOnOffDelay);
  } else {
    global::errprintf_P(PSTR("global:SetCamAutoOnOffDelay INVALID NUMBER %d\n"), iNew);
  }
}
void global::SetCamContrast(int iNew, bool bEpprom) {
  global::m_iCamContrast = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamContrast:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changeContrast(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamContrast:Change SUCCESS\n"));

    // Write to EEPROM
    if (bEpprom) {
      // Save custom Color calibrations
      if (global::m_iCamPreset != CAM_custom) {
        char szSettingID[ESP32_PREF_KEYMAX];
        snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamContrast%d"), global::m_iCamPreset);
        global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
      } else {
        global::m_pEepromHelper->SetSettingInt("CamContrast", iNew);
      }
    }
  } else {
    _DBGPRINTF(PSTR("global:SetCamContrast:Change FAIL\n"));
  }
}
void global::SetCamBrightness(int iNew, bool bEpprom) {
  global::m_iCamBrightness = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamBrightness:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changeBrightness(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamBrightness:Change SUCCESS\n"));
    // Write to EEPROM
    if (bEpprom) {
      // Save custom Color calibrations
      if (global::m_iCamPreset != CAM_custom) {
        char szSettingID[ESP32_PREF_KEYMAX];
        snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamBrightness%d"), global::m_iCamPreset);
        global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
      } else {
        global::m_pEepromHelper->SetSettingInt("CamBrightness", iNew);
      }
    }
  } else {
    _DBGPRINTF(PSTR("global:SetCamBrightness:Change FAIL\n"));
  }
}
void global::SetCamSaturation(int iNew, bool bEpprom) {
  global::m_iCamSaturation = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamSaturation:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changeSaturation(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamSaturation:Change SUCCESS\n"));
    // Write to EEPROM
    if (bEpprom) {
      // Save custom Color calibrations
      if (global::m_iCamPreset != CAM_custom) {
        char szSettingID[ESP32_PREF_KEYMAX];
        snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamSaturation%d"), global::m_iCamPreset);
        global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
      } else {
        global::m_pEepromHelper->SetSettingInt(CAM_SATURATION_START, iNew);
      }
    }
  } else {
    _DBGPRINTF(PSTR("global:SetCamSaturation:Change FAIL\n"));
  }
}
void global::SetCamSpecialEffect(int iNew, bool bEpprom) {
  global::m_iCamSpecialEffect = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamSpecialEffect:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changeSpecialEffect(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamSpecialEffect:Change SUCCESS\n"));
    // Write to EEPROM
    if (bEpprom)
      global::m_pEepromHelper->SetSettingInt(CAM_SPECIALEFFECT_START, iNew);
  } else {
    _DBGPRINTF(PSTR("global:SetCamSpecialEffect:Change FAIL\n"));
  }
}
void global::SetCamWbMode(int iNew, bool bEpprom) {
  global::m_iCamWbMode = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamWbMode:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changeWbMode(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamWbMode:Change SUCCESS\n"));
    // Write to EEPROM
    if (bEpprom)
      global::m_pEepromHelper->SetSettingInt(CAM_WBMODE_START, iNew);
  } else {
    _DBGPRINTF(PSTR("global:SetCamWbMode:Change FAIL\n"));
  }
}
void global::SetCamAeLevels(int iNew, bool bEpprom) {
  if (iNew >= -2 && iNew <= 2) {
    global::m_iCamAeLevels = iNew;
    if (!global::m_bCameraOK) {
      _DBGPRINTF_VERBOSE(PSTR("global:SetCamAeLevels:Value set but CAMERA NOT INITIATED YET.\n"));
      return;
    }

    if (esp32cam::Camera.changeAeLevels(iNew)) {
      _DBGPRINTF_VERBOSE(PSTR("global:SetCamAeLevels:Change SUCCESS\n"));

      // Write to EEPROM
      if (bEpprom) {
        // Save custom Color calibrations
        if (global::m_iExposurePreset != CAM_exp_custom) {
          char szSettingID[ESP32_PREF_KEYMAX];
          snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAelevels%d"), global::m_iExposurePreset);
          global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
        } else {
          global::m_pEepromHelper->SetSettingInt("CamAelevels", iNew);
        }
      }
    } else {
      _DBGPRINTF(PSTR("global:SetCamAeLevels:Change FAIL\n"));
    }
  } else {
    _DBGPRINTF(PSTR("global:SetCamAeLevels:INVALID VALUE %d\n"), iNew);
  }
}
void global::SetCamAwbGainControl(int iNew, bool bEpprom) {
  global::m_iCamAwbGainControl = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamAwbGainControl:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changeAwbGainControl(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamAwbGainControl:Change SUCCESS\n"));

    // Write to EEPROM
    if (bEpprom) {
      // Save custom Color calibrations
      if (global::m_iCamPreset != CAM_custom) {
        char szSettingID[ESP32_PREF_KEYMAX];
        snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAwbGain%d"), global::m_iCamPreset);
        global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
      } else {
        global::m_pEepromHelper->SetSettingInt("CamAwbGain", iNew);
      }
    }
  } else {
    _DBGPRINTF(PSTR("global:SetCamAwbGainControl:Change FAIL\n"));
  }
}
void global::SetCamAgcGain(int iNew, bool bEpprom) {
  global::m_iCamAgcGain = iNew;

  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamAgcGain:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }

  // Only set if CamGaincontrol is disabled!
  bool bSensorSettingChanged = true;
  if (global::GetCamGaincontrol() == 0) {
    bSensorSettingChanged = esp32cam::Camera.changAgcGain(iNew);
  }

  if (bSensorSettingChanged) {
    // Write to EEPROM
    if (bEpprom) {
      // Save custom Color calibrations
      if (global::m_iExposurePreset != CAM_exp_custom) {
        char szSettingID[ESP32_PREF_KEYMAX];
        snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAgcGain%d"), global::m_iExposurePreset);
        global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
      } else {
        global::m_pEepromHelper->SetSettingInt("CamAgcGain", iNew);
      }
    }
  } else {
    _DBGPRINTF(PSTR("global:SetCamAgcGain:Change FAIL\n"));
  }
}
void global::SetCamGainceilingSensor(int iNew) {
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamGainceilingSensor:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changGainceilingSensor(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamGainceilingSensor:Change SUCCESS\n"));
  } else {
    _DBGPRINTF(PSTR("global:SetCamGainceilingSensor:Change FAIL\n"));
  }
}
void global::SetCamGaincontrol(int iNew, bool bEpprom) {
  global::m_iCamGaincontrol = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamGaincontrol:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changeGaincontrol(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamGaincontrol:Change SUCCESS\n"));

    // Write to EEPROM
    if (bEpprom) {
      // Save custom Color calibrations
      if (global::m_iExposurePreset != CAM_exp_custom) {
        char szSettingID[ESP32_PREF_KEYMAX];
        snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamGain%d"), global::m_iExposurePreset);
        global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
      } else {
        global::m_pEepromHelper->SetSettingInt("CamGain", iNew);
      }
    }
  } else {
    _DBGPRINTF(PSTR("global:SetCamGaincontrol:Change FAIL\n"));
  }
}
void global::SetCamWhitebalance(int iNew, bool bEpprom) {
  global::m_iCamAutoWhitebalance = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamWhitebalance:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changeWhitebalance(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamWhitebalance:Change SUCCESS\n"));

    // Write to EEPROM
    if (bEpprom) {
      // Save custom Color calibrations
      if (global::m_iCamPreset != CAM_custom) {
        char szSettingID[ESP32_PREF_KEYMAX];
        snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAwb%d"), global::m_iCamPreset);
        global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
      } else {
        global::m_pEepromHelper->SetSettingInt("CamAwb", iNew);
      }
    }
  } else {
    _DBGPRINTF(PSTR("global:SetCamWhitebalance:Change FAIL\n"));
  }
}
void global::SetCamColorbar(int iNew) {
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamColorbar:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changeColorbar(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamColorbar:Change SUCCESS\n"));
  } else {
    _DBGPRINTF(PSTR("global:SetCamColorbar:Change FAIL\n"));
  }
}
void global::SetCamQuality(int iNew, bool bEpprom) {
  global::m_iCamQuality = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamQuality:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changeQuality(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamQuality:Change SUCCESS\n"));
    // Write to EEPROM
    if (bEpprom)
      global::m_pEepromHelper->SetSettingInt(CAM_QUALITY_START, iNew);
  } else {
    _DBGPRINTF(PSTR("global:SetCamQuality:Change FAIL\n"));
  }
}
void global::SetCamAec2(int iNew, bool bEpprom) {
  global::m_iCamAec2 = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamAec2:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }

  // Only set to CAM sensor if CamAutoExposurecontrol is enabled!
  bool bSensorSettingChanged = true;
  if (global::GetCamAutoExposurecontrol() == 1) {
    bSensorSettingChanged = esp32cam::Camera.changAec2(iNew);
  } else {
    // Disable if Expo Control is off
    bSensorSettingChanged = esp32cam::Camera.changAec2(0);
  }

  if (bSensorSettingChanged) {
    _DBGPRINTF(PSTR("global:SetCamAec2:Change SUCCESS\n"));
    // Write to EEPROM
    if (bEpprom) {
      // Save custom Color calibrations
      if (global::m_iExposurePreset != CAM_exp_custom) {
        char szSettingID[ESP32_PREF_KEYMAX];
        snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAec2_%d"), global::m_iExposurePreset);
        global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
      } else {
        global::m_pEepromHelper->SetSettingInt(CAM_AEC2_START, iNew);
      }
    }
  } else {
    _DBGPRINTF(PSTR("global:SetCamAec2:Change FAIL\n"));
  }
}
void global::SetCamAecValue(int iNew, bool bEpprom) {
  global::m_iCamAecValue = iNew;

  // Write to EEPROM regardless!
  if (bEpprom) {
    // Save custom Color calibrations
    if (global::m_iExposurePreset != CAM_exp_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAecValue%d"), global::m_iExposurePreset);
      global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
    } else {
      global::m_pEepromHelper->SetSettingInt("CamAecValue", iNew);
    }
  }

  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamAecValue:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }

  // Only set to CAM sensor if CamAutoExposurecontrol is enabled!
  bool bSensorSettingChanged = true;
  bSensorSettingChanged = esp32cam::Camera.changeAecValue(iNew);

  if (bSensorSettingChanged) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamAecValue:Change SUCCESS %d\n"), global::m_iCamAecValue);
  } else {
    _DBGPRINTF(PSTR("global:SetCamAecValue:Change FAIL\n"));
  }
}
void global::SetCamHMirror(int iNew, bool bEpprom) {
  global::m_iCamHMirror = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetHMirror:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changHMirror(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetHMirror:Change SUCCESS\n"));
    // Write to EEPROM
    if (bEpprom)
      global::m_pEepromHelper->SetSettingInt(CAM_HMIRROR_START, iNew);
  } else {
    _DBGPRINTF(PSTR("global:SetHMirror:Change FAIL\n"));
  }
}
void global::SetCamVFlip(int iNew, bool bEpprom) {
  global::m_iCamVFlip = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetVFlip:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changVFlip(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetVFlip:Change SUCCESS\n"));
    // Write to EEPROM
    if (bEpprom)
      global::m_pEepromHelper->SetSettingInt(CAM_VVLIP_START, iNew);
  } else {
    _DBGPRINTF(PSTR("global:SetVFlip:Change FAIL\n"));
  }
}
void global::SetCamHighQuality(byte bNew, bool bEpprom) {
  // Always OFF if no PSRAM available!
  if (psramFound()) {
    global::m_bCamHighQuality = bNew;
  } else {
    global::m_bCamHighQuality = 0;
  }

  // Write to EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("CamHighQuality", bNew);
}
void global::SetCamBPC(int iNew, bool bEpprom) {
  global::m_iCamBPC = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamBPC:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changBPC(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamBPC:Change SUCCESS\n"));
    // Write to EEPROM
    if (bEpprom)
      global::m_pEepromHelper->SetSettingInt("CamBPC", iNew);
  } else {
    _DBGPRINTF(PSTR("global:SetCamBPC:Change FAIL\n"));
  }
}
void global::SetCamWPC(int iNew, bool bEpprom) {
  global::m_iCamWPC = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamWPC:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changWPC(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamWPC:Change SUCCESS\n"));
    // Write to EEPROM
    if (bEpprom)
      global::m_pEepromHelper->SetSettingInt("CamWPC", iNew);
  } else {
    _DBGPRINTF(PSTR("global:SetCamWPC:Change FAIL\n"));
  }
}
void global::SetCamDenoise(int iNew, bool bEpprom) {
  global::m_iCamDenoise = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamDenoise:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changDenoise(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamDenoise:Change SUCCESS\n"));
    // Write to EEPROM
    if (bEpprom)
      global::m_pEepromHelper->SetSettingInt("CamDenoise", iNew);
  } else {
    _DBGPRINTF(PSTR("global:SetCamDenoise:Change FAIL\n"));
  }
}
void global::SetCamLenc(int iNew, bool bEpprom) {
  global::m_iCamLenc = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamLenc:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changLenc(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamLenc:Change SUCCESS\n"));

    // Write to EEPROM
    if (bEpprom) {
      // Save custom Color calibrations
      if (global::m_iExposurePreset != CAM_exp_custom) {
        char szSettingID[ESP32_PREF_KEYMAX];
        snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamLenc%d"), global::m_iExposurePreset);
        global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
      } else {
        global::m_pEepromHelper->SetSettingInt("CamLenc", iNew);
      }
    }
  } else {
    _DBGPRINTF(PSTR("global:SetCamLenc:Change FAIL\n"));
  }
}
void global::SetCamRawGMA(int iNew, bool bEpprom) {
  global::m_iCamRawGMA = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamRawGMA:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changRawGMA(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamRawGMA:Change SUCCESS\n"));
    // Write to EEPROM
    if (bEpprom) {
      // Save custom Color calibrations
      if (global::m_iExposurePreset != CAM_exp_custom) {
        char szSettingID[ESP32_PREF_KEYMAX];
        snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamRawGMA%d"), global::m_iExposurePreset);
        global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
      } else {
        global::m_pEepromHelper->SetSettingInt("CamRawGMA", iNew);
      }
    }
  } else {
    _DBGPRINTF(PSTR("global:SetCamRawGMA:Change FAIL\n"));
  }
}
void global::SetCamAutoExposurecontrol(int iNew, bool bEpprom) {
  global::m_iCamAutoExposurecontrol = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamAutoExposurecontrol:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }

  if (esp32cam::Camera.changAutoExposurecontrol(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamAutoExposurecontrol:Change SUCCESS\n"));

    // Write to EEPROM
    if (bEpprom) {
      // Save custom Color calibrations
      if (global::m_iExposurePreset != CAM_exp_custom) {
        char szSettingID[ESP32_PREF_KEYMAX];
        snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAec_%d"), global::m_iExposurePreset);
        global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
      } else {
        global::m_pEepromHelper->SetSettingInt("CamAec", iNew);
      }
    }
  } else {
    _DBGPRINTF(PSTR("global:SetCamAutoExposurecontrol:Change FAIL\n"));
  }
}
void global::SetCamAdaptiveExposurecontrol(int iNew, bool bEpprom) {
  global::m_iCamAdaptiveExposurecontrol = iNew;

  // Write to EEPROM
  if (bEpprom) {
    // Save custom Color calibrations
    if (global::m_iExposurePreset != CAM_exp_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAdptExp%d"), global::m_iExposurePreset);
      global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
    } else {
      global::m_pEepromHelper->SetSettingInt("CamAdptExp", iNew);
    }
  }
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamAdaptiveExposurecontrol to %d\n"), global::m_iCamAdaptiveExposurecontrol);
}
void global::SetCamAecMinValue(int iNew, bool bEpprom) {
  global::m_iCamAecMinValue = iNew;

  // Write to EEPROM regardless!
  if (bEpprom) {
    // Save custom Color calibrations
    if (global::m_iExposurePreset != CAM_exp_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAecMinVal%d"), global::m_iExposurePreset);
      global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
    } else {
      global::m_pEepromHelper->SetSettingInt("CamAecMinVal", iNew);
    }
  }
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamAecMinValue to %d\n"), global::m_iCamAecMinValue);
}
void global::SetCamAecMaxValue(int iNew, bool bEpprom) {
  global::m_iCamAecMaxValue = iNew;

  // Write to EEPROM regardless!
  if (bEpprom) {
    // Save custom Color calibrations
    if (global::m_iExposurePreset != CAM_exp_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAecMaxVal%d"), global::m_iExposurePreset);
      global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
    } else {
      global::m_pEepromHelper->SetSettingInt("CamAecMaxVal", iNew);
    }
  }
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamAecMaxValue to %d\n"), global::m_iCamAecMaxValue);
}
void global::SetCamAdaptiveChangePercent(int iNew, bool bEpprom) {
  global::m_iCamAdaptiveChangePercent = iNew;

  // Write to EEPROM regardless!
  if (bEpprom) {
    // Save custom Color calibrations
    if (global::m_iExposurePreset != CAM_exp_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAdptChPer%d"), global::m_iExposurePreset);
      global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
    } else {
      global::m_pEepromHelper->SetSettingInt("CamAdptChPer", iNew);
    }
  }
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamAdaptiveChangePercent to %d\n"), global::m_iCamAdaptiveChangePercent);
}
void global::SetCamAdaptiveChangeDelay(int iNew, bool bEpprom) {
  global::m_iCamAdaptiveChangeDelay = iNew;

  // Write to EEPROM regardless!
  if (bEpprom) {
    // Save custom Color calibrations
    if (global::m_iExposurePreset != CAM_exp_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAdptChDly%d"), global::m_iExposurePreset);
      global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
    } else {
      global::m_pEepromHelper->SetSettingInt("CamAdptChDly", iNew);
    }
  }
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamAdaptiveChangeDelay to %d\n"), global::m_iCamAdaptiveChangeDelay);
}
void global::SetCamAdaptiveGaincontrol(int iNew, bool bEpprom) {
  global::m_iCamAdaptiveGaincontrol = iNew;

  // Write to EEPROM regardless!
  if (bEpprom) {
    // Save custom Color calibrations
    if (global::m_iExposurePreset != CAM_exp_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAdptGaCtrl%d"), global::m_iExposurePreset);
      global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iNew);
    } else {
      global::m_pEepromHelper->SetSettingInt("CamAdptGaCtrl", iNew);
    }
  }
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamAdaptiveGaincontrol to %d\n"), global::m_iCamAdaptiveGaincontrol);
}
void global::SetCamWhiteActivationTreshold(byte bNew, bool bEpprom) {
  global::m_bCamWhiteActivationTreshold = bNew;
  // Write to EEPROM
  if (bEpprom) {
    // Save custom Color calibrations
    if (global::m_iExposurePreset != CAM_exp_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamWhiteThres%d"), global::m_iExposurePreset);
      global::m_pEepromHelper->SetSettingByte(&szSettingID[0], bNew);
    } else {
      global::m_pEepromHelper->SetSettingByte("CamWhiteThres", bNew);
    }
  }
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamWhiteActivationTreshold to %d\n"), global::m_bCamWhiteActivationTreshold);
}

void global::SetCamBeforeWhiteActivationTreshold(byte bNew, bool bEpprom) {
  global::m_bCamBeforeWhiteActivationTreshold = bNew;
  // Write to EEPROM
  if (bEpprom) {
    // Save custom Color calibrations
    if (global::m_iExposurePreset != CAM_exp_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamBeWhiteThr%d"), global::m_iExposurePreset);
      global::m_pEepromHelper->SetSettingByte(&szSettingID[0], bNew);
    } else {
      global::m_pEepromHelper->SetSettingByte("CamBeWhiteThr", bNew);
    }
  }
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamBeforeWhiteActivationTreshold to %d\n"), global::m_bCamBeforeWhiteActivationTreshold);
}
void global::SetCamWhiteMinimumAmount(byte bNew, bool bEpprom) {
  global::m_bCamWhiteMinimumAmount = bNew;
  // Write to EEPROM
  if (bEpprom) {
    // Save custom Color calibrations
    if (global::m_iExposurePreset != CAM_exp_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamWhiteMinA%d"), global::m_iExposurePreset);
      global::m_pEepromHelper->SetSettingByte(&szSettingID[0], bNew);
    } else {
      global::m_pEepromHelper->SetSettingByte("CamWhiteMinA", bNew);
    }
  }
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamWhiteMinimumAmount to %d\n"), global::m_bCamWhiteMinimumAmount);
}
void global::SetCamAdaptiveGainDown(byte bNew, bool bEpprom) {
  global::m_bCamAdaptiveGainDown = bNew;
  // Write to EEPROM
  if (bEpprom) {
    // Save custom Color calibrations
    if (global::m_iExposurePreset != CAM_exp_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAdptGaDown%d"), global::m_iExposurePreset);
      global::m_pEepromHelper->SetSettingByte(&szSettingID[0], bNew);
    } else {
      global::m_pEepromHelper->SetSettingByte("CamAdptGaDown", bNew);
    }
  }
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamAdaptiveGainDown to %d\n"), global::m_bCamAdaptiveGainDown);
}
void global::SetCamAdaptiveGainUp(byte bNew, bool bEpprom) {
  global::m_bCamAdaptiveGainUp = bNew;
  // Write to EEPROM
  if (bEpprom) {
    // Save custom Color calibrations
    if (global::m_iExposurePreset != CAM_exp_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAdptGaUp%d"), global::m_iExposurePreset);
      global::m_pEepromHelper->SetSettingByte(&szSettingID[0], bNew);
    } else {
      global::m_pEepromHelper->SetSettingByte("CamAdptGaUp", bNew);
    }
  }
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamAdaptiveGainUp to %d\n"), global::m_bCamAdaptiveGainUp);
}
void global::SetCamAdaptiveGainMax(byte bNew, bool bEpprom) {
  global::m_bCamAdaptiveGainMax = bNew;
  // Write to EEPROM
  if (bEpprom) {
    if (global::m_iExposurePreset != CAM_exp_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAdptGaMx%d"), global::m_iExposurePreset);
      global::m_pEepromHelper->SetSettingByte(&szSettingID[0], bNew);
    } else {
      global::m_pEepromHelper->SetSettingByte("CamAdptGaMx", bNew);
    }
  }
  _DBGPRINTF_VERBOSE(PSTR("global:SetCamAdaptiveGainMax to %d\n"), global::m_bCamAdaptiveGainMax);
}

void global::SetCamDcw(int iNew, bool bEpprom) {
  global::m_iCamDcw = iNew;
  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamDcw:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }
  if (esp32cam::Camera.changDcw(iNew)) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamDcw:Change SUCCESS\n"));
    // Write to EEPROM
    if (bEpprom)
      global::m_pEepromHelper->SetSettingInt("CamDcw", iNew);
  } else {
    _DBGPRINTF(PSTR("global:SetCamDcw:Change FAIL\n"));
  }
}
#endif

#if defined(ENABLE_DREAMSCREEN)
void global::InitDreamScreen() {
  if (m_pDreamScreen == NULL)
    m_pDreamScreen = new DreamScreen(DREAMSCREEN_UDP_PORT);
}
void global::StartServerDreamScreen() {
  InitDreamScreen();
  if (m_pDreamScreen) {
    m_pDreamScreen->StartServer();
    if (GetDreamScreenEmulation() == 1)
      DiscoverDreamScreenDevices();
  }
}
void global::SetDreamScreenGroup(byte bNew, bool bEpprom) {
  if (m_pDreamScreen) m_pDreamScreen->SetGroup(bNew, bEpprom);
}
void global::SetDreamScreenLowColorSmoothing(byte bNew, bool bEpprom) {
  if (m_pDreamScreen) m_pDreamScreen->SetLowColorSmoothing(bNew, bEpprom);
}
void global::SetDreamScreenEmulation(byte bNew, bool bEpprom) {
  if (m_pDreamScreen) m_pDreamScreen->SetEmulation(bNew, bEpprom);
}
void global::SetDreamScreenSupport(byte bNew, bool bEpprom, bool bInitDreamScreen) {
  if (m_pDreamScreen) m_pDreamScreen->SetSupport(bNew, bEpprom, bInitDreamScreen);
}
void global::DiscoverDreamScreenDevices() {
  if (m_pDreamScreen) m_pDreamScreen->DiscoverDevices();
  else global::errprintf_P(PSTR("global:DiscoverDreamScreenDevices m_pDreamScreen is NULL!\n"));
}
#endif


#if defined(ENABLE_HUE)
void global::InitHue() {
  if (m_pHueHelper && m_pHueHelper->GetSupport() == 1) {
    _DBGPRINTF(PSTR("global:InitHue: InitHueLights\n"));
    m_pHueHelper->InitHueLights();
  }
}
void global::SetHueSupport(byte bNew, bool bEpprom, bool bInitHue) {
  if (m_pHueHelper) m_pHueHelper->SetSupport(bNew, bEpprom, bInitHue);
}
void global::SetFrameLengthDtls(byte bNew, bool bEpprom) {
  if (m_pHueHelper) m_pHueHelper->SetFrameLengthDtls(bNew, bEpprom);
}
void global::SetHandshakeTimeout(byte bNew, bool bEpprom) {
  if (m_pHueHelper) m_pHueHelper->SetHandshakeTimeout(bNew, bEpprom);
}
void global::SetHueBridgeIP(char* pszNew, bool bEpprom) {
  if (m_pHueHelper) m_pHueHelper->SetBridgeIP(pszNew, bEpprom);
}
void global::SetHueBridgeUser(char* pszNew, bool bEpprom) {
  if (m_pHueHelper) m_pHueHelper->SetBridgeUser(pszNew, bEpprom);
}
void global::SetHueBridgeClientkey(char* pszNew, bool bEpprom) {
  if (m_pHueHelper) m_pHueHelper->SetBridgeClientkey(pszNew, bEpprom);
}
int global::GetHueLightState(char* pszHueID, bool bCalcHudID) {
  return m_pHueHelper ? m_pHueHelper->GetLightState(pszHueID, bCalcHudID) : 0;
}
void global::SetHueLightState(char* pszHueID, int iNew, bool bEpprom) {
  if (m_pHueHelper) m_pHueHelper->SetLightState(pszHueID, iNew, bEpprom);
}
int global::GetHueLightZoneState(char* pszHueID, int iZone, bool bCalcHudID) {
  return m_pHueHelper ? m_pHueHelper->GetLightZoneState(pszHueID, iZone, bCalcHudID) : 1;
}
void global::SetHueLightZoneState(char* pszHueID, int iZone, int iNew, bool bEpprom) {
  if (m_pHueHelper) m_pHueHelper->SetLightZoneState(pszHueID, iZone, iNew, bEpprom);
}
int global::GetHueLightColCorRed(char* pszHueID, bool bCalcHudID) {
  return m_pHueHelper ? m_pHueHelper->GetLightColCorRed(pszHueID, bCalcHudID) : 100;
}
void global::SetHueLightColCorRed(char* pszHueID, int iNew, bool bEpprom) {
  if (m_pHueHelper) m_pHueHelper->SetLightColCorRed(pszHueID, iNew, bEpprom);
}
int global::GetHueLightColCorGreen(char* pszHueID, bool bCalcHudID) {
  return m_pHueHelper ? m_pHueHelper->GetLightColCorGreen(pszHueID, bCalcHudID) : 100;
}
void global::SetHueLightColCorGreen(char* pszHueID, int iNew, bool bEpprom) {
  if (m_pHueHelper) m_pHueHelper->SetLightColCorGreen(pszHueID, iNew, bEpprom);
}
int global::GetHueLightColCorBlue(char* pszHueID, bool bCalcHudID) {
  return m_pHueHelper ? m_pHueHelper->GetLightColCorBlue(pszHueID, bCalcHudID) : 100;
}
void global::SetHueLightColCorBlue(char* pszHueID, int iNew, bool bEpprom) {
  if (m_pHueHelper) m_pHueHelper->SetLightColCorBlue(pszHueID, iNew, bEpprom);
}
int global::GetHueLightColBrightness(char* pszHueID, bool bCalcHudID) {
  return m_pHueHelper ? m_pHueHelper->GetLightColBrightness(pszHueID, bCalcHudID) : 255;
}
void global::SetHueLightColBrightness(char* pszHueID, int iNew, bool bEpprom) {
  if (m_pHueHelper) m_pHueHelper->SetLightColBrightness(pszHueID, iNew, bEpprom);
}
void global::CreateHueIDWithNum(char* pszHueID, char* pszBuf, size_t stBufLen, int iNum) {
  HueHelper::CreateHueIDWithNum(pszHueID, pszBuf, stBufLen, iNum);
}
void global::CreateHueIDWithStr(char* pszHueID, char* pszBuf, size_t stBufLen, const char* pszExtra) {
  HueHelper::CreateHueIDWithStr(pszHueID, pszBuf, stBufLen, pszExtra);
}
#endif

void global::SetDebugSyslogServer(char* pszNew) {
  DebugHelper::SetDebugSyslogServer(pszNew);
}

/* Debug / serial / reboot: delegated to DebugHelper */
void global::dbgprintf_P(PGM_P pMsg, ...) {
  va_list va;
  va_start(va, pMsg);
  DebugHelper::dbgprintf_P(pMsg, va);
  va_end(va);
}
void global::dbgprintf(const char* pMsg, ...) {
  va_list va;
  va_start(va, pMsg);
  DebugHelper::dbgprintf(pMsg, va);
  va_end(va);
}
void global::errprintf(PGM_P pMsg, ...) {
  va_list va;
  va_start(va, pMsg);
  DebugHelper::errprintf(pMsg, va);
  va_end(va);
}
void global::errprintf_P(const char* pMsg, ...) {
  va_list va;
  va_start(va, pMsg);
  DebugHelper::errprintf_P(pMsg, va);
  va_end(va);
}
void global::RebootController() {
  DebugHelper::RebootController();
}
void global::SetRemoteLEDMode(char* pszRemote, byte bLedMode) {
  DebugHelper::SetRemoteLEDMode(pszRemote, bLedMode);
}
void global::heap_caps_alloc_failed_hook(size_t requested_size, uint32_t caps, const char* function_name) {
  DebugHelper::heap_caps_alloc_failed_hook(requested_size, caps, function_name);
}
int global::dbgprintf_esp(const char* fmt, va_list args) {
  return DebugHelper::dbgprintf_esp(fmt, args);
}
void global::printfserial(const char* fmt, va_list args) {
  DebugHelper::printfserial(fmt, args);
}
void global::flushserial() {
  DebugHelper::flushserial();
}

void global::SetLedHelper(LEDHelper* pNew) {
  global::m_pLedHelper = pNew;
}

void global::SetEepromHelper(EEPROMHelper* pNew) {
  global::m_pEepromHelper = pNew;
}

void global::SetWebServerHelper(AmbiWebServer* pNew) {
  global::m_pWebServer = pNew;
}

void global::HandleButtonFunction() {
  // Handle Short Button Click
  switch (global::GetButtonFunction()) {
    case BTN_MODE_OFF:
      _DBGPRINTF(PSTR("global:HandleButtonFunction Button set to BTN_MODE_OFF\n"));
      break;
    case BTN_MODE_LED_OFF:
      _DBGPRINTF(PSTR("global:HandleButtonFunction Button set to BTN_MODE_LED_OFF\n"));
      global::m_pLedHelper->SetLedMode(LED_MODE_OFF);
      break;
#if defined(ENABLE_CAM)
    case BTN_MODE_LED_CAM:
      _DBGPRINTF(PSTR("global:HandleButtonFunction Button set to BTN_MODE_LED_CAM\n"));
      if (global::m_pLedHelper->GetLedMode() == LED_MODE_CAM)
        global::m_pLedHelper->SetLedMode(LED_MODE_OFF);
      else
        global::m_pLedHelper->SetLedMode(LED_MODE_CAM);
      break;
    case BTN_MODE_CAM_TOGGLE_COLORMODE:
      _DBGPRINTF(PSTR("global:HandleButtonFunction Button set to BTN_MODE_CAM_TOGGLE_COLORMODE\n"));
      if (global::m_pCamProcessor->GetCamMode() == LED_MODE_CAM_SINGLE)
        global::m_pCamProcessor->SetCamMode((byte)LED_MODE_CAM_BAR);
      else if (global::m_pCamProcessor->GetCamMode() == LED_MODE_CAM_BAR)
        global::m_pCamProcessor->SetCamMode((byte)LED_MODE_CAM_DETAIL);
      else
        global::m_pCamProcessor->SetCamMode((byte)LED_MODE_CAM_SINGLE);
      break;
    case BTN_MODE_CAM_TOGGLE_COLORPRESET:
      _DBGPRINTF(PSTR("global:HandleButtonFunction Button set to BTN_MODE_CAM_TOGGLE_COLORPRESET - TODO!!!\n"));
      // TODO!!!
      break;
#endif
#if defined(ENABLE_HUE)
    case BTN_MODE_SMART_HUE_TOGGLE:
      _DBGPRINTF(PSTR("global:HandleButtonFunction Button set to BTN_MODE_SMART_HUE_TOGGLE %d\n"), global::GetHueSupport());
      if (global::GetHueSupport() == 0)
        global::SetHueSupport(1);
      else
        global::SetHueSupport(0);
      break;
#endif
#if defined(ENABLE_DREAMSCREEN)
    case BTN_MODE_SMART_DREAM_TOGGLE:
      _DBGPRINTF(PSTR("global:HandleButtonFunction Button set to BTN_MODE_SMART_DREAM_TOGGLE %d\n"), global::GetDreamScreenEmulation());
      if (global::GetDreamScreenEmulation() == 0)
        global::SetDreamScreenEmulation(1);
      else
        global::SetDreamScreenEmulation(0);
      break;
#endif
    default:
      _DBGPRINTF(PSTR("global:HandleButtonFunction Button set to %d, unknown mode!\n"), global::GetButtonFunction());
      break;
  }
}

void global::SetButtonFunction(byte bNew, bool bEpprom) {
  if (bNew >= BTN_MODE_OFF && bNew <= BTN_MODE_LAST ) {
    global::m_bButtonFunction = bNew;
    _DBGPRINTF(PSTR("global:SetButtonFunction Value=%d\n"), bNew);
    // Write to EEPROM!
    if (bEpprom)
      global::m_pEepromHelper->SetSettingByte("SysBtnFunc", global::m_bButtonFunction);
  } else {
    _DBGPRINTF(PSTR("global:SetButtonFunction:INVALID FUNCTION %d\n"), bNew);
  }
}

void global::SetWifiFailHandling(byte bNew, bool bEpprom) {
  if (bNew >= WIFI_FAIL_MIN  && bNew <= WIFI_FAIL_MAX  ) {
    global::m_bWifiFailHandling = bNew;
    _DBGPRINTF(PSTR("global:SetWifiFailHandling Value=%d\n"), bNew);
    // Write to EEPROM!
    if (bEpprom)
      global::m_pEepromHelper->SetSettingByte("SysWifiFail", global::m_bWifiFailHandling);
  } else {
    _DBGPRINTF(PSTR("global:SetWifiFailHandling:INVALID MODE %d\n"), bNew);
  }
}

void global::HandleWifiFail(void) {

}

#endif /* defined(ESP32) */
