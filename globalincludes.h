#ifndef globalincludes_h
#define globalincludes_h
// --- Version Include FIRST !
#include "core_version.h"
// --- 
#define BROADCAST_DISCOVER_MSG PSTR("HELLO AMBIBACK DEVICES")

// Common string prefixes for code size reduction
// Note: These are regular string literals (not PSTR) so they can be concatenated
// The compiler will optimize these into single strings
#define PSTR_SETUP "setup: "
#define PSTR_LOOP "loop: "
#define PSTR_LED "LED: "
#define PSTR_WIFI "WiFi: "
#define PSTR_WIFI_EV "WiFiEv: "  // Shorter than "handleWiFiEvents: "
#define PSTR_GLOBAL "global: "
#define PSTR_FAILED "failed"
#define PSTR_SUCCESS "success"
#define PSTR_ERROR "Error"

#define AMBIBACK_MAJOR 0
#define AMBIBACK_MINOR 5
#define AMBIBACK_BUILD 231
#define AMBIBACK_VERSION "0.5.231"

// Base URL for firmware updates. Override it with a compiler flag for your own update server.
#ifndef AMBIBACK_FIRMWARE_BASE_URL
#define AMBIBACK_FIRMWARE_BASE_URL "http://firmware.ambiback.com/firmware/esp32"
#endif

// Seed for the generated access point password (32-bit value).
// Set your own value for your builds and use the same value for AmbiBack Manager.
// The default 0x0000FFFF keeps the password of existing devices unchanged.
#ifndef AMBIBACK_AP_PASSWORD_SEED
#define AMBIBACK_AP_PASSWORD_SEED 0x0000FFFFUL
#endif

#define EMBED_MINIMAL_UI
#define FORCE_WIFIWPS_FIRSTTIME false   
#define WIFI_SETSLEEP                     // Sets Wifi.Sleep calls after connect



// Debug Firmware Update
#define DEBUG_UPDATER Serial

// If defined, OTA updater runs it it's own task!
#define OTA_UPDATE_TASK

// DIRTY FIX TO DISABLE FASTLED PRAGMA MESSAGES
#define FASTLED_INTERNAL

// --- FIRST, set the desired board type!
// TODO make WS2812 Control LEDs
// #define USE_WS2812_CONTROL_LEDS 

// ESP32 BRIDGE Module
#define ESP32_BRIDGE 0
// ESP32 Naked Module
#define ESP32_NAKED 1
// ESP32 Wroover Cam Module
#define ESP32_WROOVER_CAM 2
// ESP32 TTGO Cam Module
#define ESP32_TTGO_CAM 20
// ESP32 TTGO Cam + Mic Module
#define ESP32_TTGO_CAM_MIC 21
// ESP32 TTGO T-Journal Module
#define ESP32_TTGO_CAM_JOURNAL 22
// ESP32 TTGO Cam Plus Module
#define ESP32_TTGO_CAM_PLUS 23

// ESP32 M5STACK ESP32CAM Module
#define ESP32_M5ST_CAM 30
// ESP32 M5STACK CAM X OV2660 Module
#define ESP32_M5ST_CAM_X 31
// ESP32 M5STACK Atom Lite Module
#define ESP32_M5ST_ATOMLITE 32
// ESP32 M5STACK Atom Matrix Module
#define ESP32_M5ST_ATOMMATRIX 33
// ESP32 M5STACK CAM Timer X OV3660 Module
#define ESP32_M5ST_CAM_TIMER_X 34
// ESP32 AmbiBack LED Module (M5STACK Atom Lite Module)
#define ESP32_AMBIBACK_LEDV1 35

// PIN Defines for different camera modules
#define TTGO_T_CAMERA_MIC_V16 100
#define TTGO_T_CAMERA_V05 101
#define TTGO_T_CAMERA_V1_7_2 102
#define TTGO_T_JORNAL 103
#define TTGO_T_CAMERA_PLUS 104
#define M5_ESP32CAM 200
#define M5_ESP32CAM_X 201
#define M5_ESP32CAM_TIMER_X 202

// Camera Sensor definces
#define CAM_SENSOR_OV2640 1
#define CAM_SENSOR_OV3660 2

// Define which module to build 
#ifndef ESP32_DEVICE_TYPE
// # define ESP32_DEVICE_TYPE ESP32_BRIDGE
// # define ESP32_DEVICE_TYPE ESP32_NAKED
// # define ESP32_DEVICE_TYPE ESP32_WROOVER_CAM
// # define ESP32_DEVICE_TYPE ESP32_TTGO_CAM
// # define ESP32_DEVICE_TYPE ESP32_TTGO_CAM_MIC
// # define ESP32_DEVICE_TYPE ESP32_TTGO_CAM_JOURNAL
// # define ESP32_DEVICE_TYPE ESP32_TTGO_CAM_PLUS
// # define ESP32_DEVICE_TYPE ESP32_M5ST_CAM
// # define ESP32_DEVICE_TYPE ESP32_M5ST_CAM_X
# define ESP32_DEVICE_TYPE ESP32_M5ST_ATOMLITE
// # define ESP32_DEVICE_TYPE ESP32_M5ST_ATOMMATRIX
// # define ESP32_DEVICE_TYPE ESP32_AMBIBACK_LEDV1
// # define ESP32_DEVICE_TYPE ESP32_M5ST_CAM_TIMER_X
#endif
#pragma BUILD for ESP32_DEVICE_TYPE

// Experimental Modules
// # define ESP32_TTGO_CAM_PLUS    // Undef for ESP32 TTGO Cam Plus Module
// BROKEN # define ESP32_LLGO_CAM_MIC     // Undef for ESP32 LILYGO Cam Module
// --- 

// NOT WORKING | RMT INTERNAL DRIVER NEEDS TO MUCH MEMORY
// #define FASTLED_RMT_BUILTIN_DRIVER 1
// Interrupt options
#define FASTLED_ESP32_FLASH_LOCK 1
// ALLOW Interrupts, otherwise bad PANIC ERRORs with IllegalInstruction in OTA!
#define FASTLED_ALLOW_INTERRUPTS 1

// #define FASTLED_RMT_MAX_CHANNELS 1
// #define FASTLED_RMT_MAX_CONTROLLERS 8

// DEBUG FASTLED
// #define FASTLED_RMT_SERIAL_DEBUG 1

/*
#if defined(ARDUINO_ESP32_RELEASE_2_0_0)
// Use I2S Driver, other driver does not work anymore!
// https://github.com/FastLED/FastLED/blob/e5921775c64c2eb3428e70f9f39ebd7d76f51b5e/src/platforms/esp/32/clockless_i2s_esp32.h
# define FASTLED_ESP32_I2S true
# define FASTLED_I2S_MAX_CONTROLLERS 4
#endif 
*/

# if CONFIG_FREERTOS_UNICORE
WTFNESS NOT TESTED
#   define SYSTEM_RUNNING_CORE 0
# else
// ESP32 usually has core 1 as running core!
#   if defined(SYSTEM_RUNNING_CORE)
#     undef SYSTEM_RUNNING_CORE
#   endif
#   define SYSTEM_RUNNING_CORE 0
#   if defined(ARDUINO_RUNNING_CORE)
#     undef ARDUINO_RUNNING_CORE
#   endif
#   define ARDUINO_RUNNING_CORE 1
# endif

// See https://github.com/espressif/arduino-esp32/blob/c2b3f2d6afc1911db1974a324782730dd642d524/cores/esp32/main.cpp
# define DEFAULT_TASK_PRIORITY 1
# define IDLE_TASK_PRIORITY 0 /* DO NOT USE */

// Needs to be set in AsyncTCP library as well!
# define CONFIG_ASYNC_TCP_RUNNING_CORE 0

// If defined, we print out run time stats (Needs vTaskGetRunTimeStats)
# define ESP32_RUNTIMESTATS

// Release build configuration - disable debug features for smaller code size
// Uncomment for production/release builds (smaller binary, no debug)
// #define RELEASE_BUILD
#ifndef RELEASE_BUILD
// Uncomment for MEM Usage DEBUG | COSTLY!
# define ESP32_DEBUG_FREEMEM
#endif

// If defined, CPU Speed is lowered automatically if module is inactive
// INSTABLE! 
// # define ESP32_CPU_AUTOLOW

// Uncomment for extra DIAG output | COSTLY!
// # define ESP32_EXTRA_DIAG
// # define ESP32_EXTRA_HEAP_TRACING    // Very verbose

// Uncomment for normal debug output (disabled when RELEASE_BUILD defined)
#ifndef RELEASE_BUILD
# define ENABLE_NORMAL_DEBUG
#endif

// Uncomment for ULTRA DIAG output | COSTLY!
// # define ENABLE_ULTRA_DEBUG

// Uncomment for verbose DIAG output | COSTLY!
// # define ENABLE_VERBOSE_DEBUG

// Set to true/false for Startup ULTRA DEBUG
# define ESP32_ULTRA_DEBUG_ENABLED false

// Uncomment if you need to debug internal ESP32 code
#ifndef RELEASE_BUILD
# define ESP32_REDIR_DEBUG
# define ESP32_DEBUG_LEVEL  ESP_LOG_WARN
#endif
//                          ESP_LOG_INFO
//                          ESP_LOG_VERBOSE

// NEW - DEBUG defines for debugging
# define DEBUGLEVEL_VERBOSE  128
# define DEBUGLEVEL_ULTRA    64
# define DEBUGLEVEL_INFO     32
# define DEBUGLEVEL_WARN     16
# define DEBUGLEVEL_ERROR    8
# define _ERRPRINTF(...) { global::errprintf_P(__VA_ARGS__); }
# if defined(ENABLE_NORMAL_DEBUG) 
#   define _DBGPRINTF(...) if(global::GetDebugEnabled()) { global::dbgprintf_P(__VA_ARGS__); }
# else
#   define _DBGPRINTF(...) ((void)0)  // Compile-time elimination
# endif
# if defined(ENABLE_ULTRA_DEBUG) 
#   define _DBGPRINTF_ULTRA(...) if(global::GetDebugEnabled() && global::GetUltraDebugEnabled()) { global::dbgprintf_P(__VA_ARGS__); }
# else
#   define _DBGPRINTF_ULTRA(...) ((void)0)  // Compile-time elimination
# endif
# if defined(ENABLE_VERBOSE_DEBUG) 
#   define _DBGPRINTF_VERBOSE(...) if(global::GetDebugEnabled() && global::GetUltraDebugEnabled()) { global::dbgprintf_P(__VA_ARGS__); }
# else
#   define _DBGPRINTF_VERBOSE(...) ((void)0)  // Compile-time elimination
# endif

// More ESP32 defines 
// # define  ESP32_DISABLED_BROWNOUT

// Own preprocessors
# define _yield() yield();
# define _delay(x) if((int)x > 0) { delay(x); }
# define _delayMicroseconds(x) if((int)x > 0) { delayMicroseconds(x); }
# define _taskYIELD() taskYIELD()
# define _taskDelay(x) if((int)x > 0) { vTaskDelay(x / portTICK_PERIOD_MS); }

// DEFAULT: Use external jquery files for now!
# define HTTP_GOOGLE_JQUERYFILES 1

// Configure Frame Smoothing Max 
# define DEFAULT_FRAME_SMOOTHING 0
# define DEFAULT_FRAME_MAX 8


// Compile with second LED Port?
// --- Needs about 1kb memory 
// #define USE_HAS_SECOND_PORT 
#if !defined(USE_HAS_SECOND_PORT)
// # define CLONE_TO_SECOND_PORT
#endif

// Define our maximum EEPROM Size used to store settings
#define EEPROM_SIZE 4096
#define EEPROM_MAX_STRING_SIZE 256

// Define other max variables
#define STR_IP_MAX 46           // Sufficient for ipv6 as well
#define MAX_URL_SIZE 256        // Sufficient for our needs
#define MAX_STATUS_MSG_SIZE 512 // Sufficient for our needs
#define MAX_INT_TO_STR 10       // Max Size for INT to STR conversion
#define AMBIBACK_MAX_URLSIZE 128// Max Size for AMBIBACK urls

// Paket, Network related max
#define DEFAULT_SERIAL_RATE 115200     // 115200 
#define MAX_SERIAL_RATE 921600         // UART_BITRATE_MAX == 5000000 
#define MAX_PAKET_LENGTH 2048          // Max network paket length - should be sufficient for all cases!
#define MAX_PAKET_LEDDATA_LENGTH 1152   // Max network paket length for LED DATA!
#define NET_HEADLEN 8
#define NET_CHUNKSIZE 4
#define NET_BROADCAST_SIZE 128          // Size for UDP broadcast
#define NET_MAXSEQUENCE_DIFF 256        // Max diff for sequence check before stored sequence is being resetted back to 0
#define NET_SLOW_FPS_FRAME 100          // reduced network framesize (ms) if we are experiencing any issues (OTA update, relay failures, etc.)
#define NET_FAST_FPS_FRAME 10           // demo network framesize (ms)

// Defines for possible LED Modes
#define LED_MODE_OFF 0          // OFF
#define LED_MODE_VIDEO 1        // VIDEO
#define LED_MODE_AMBIENT 2      // AMBIENT
#define LED_MODE_CAM 3          // CAM 
#define LED_MODE_CAM_SINGLE 0   // CAM Single Color Mode
#define LED_MODE_CAM_BAR 1      // CAM Light Bar Mode
#define LED_MODE_CAM_DETAIL 2   // CAM Detailed Color Mode
#define LED_MODE_RELAY 4        // RELAY Mode like DreamScreen to SmartLight
#define LED_MODE_DEMO 5         // TEST Mode for debugging and DEMO
#define LED_MODE_MIN LED_MODE_OFF
#define LED_MODE_MAX LED_MODE_DEMO

#define LED_MAX_COLORBOOST 100  // Maximum valuie for colorboost

// Defines for possible Control Button Modes
#define BTN_MODE_OFF 0                      // DO NOTHING
#define BTN_MODE_LED_OFF 1                  // TOGGLE LED MODE to OFF
#define BTN_MODE_LED_CAM 2                  // TOGGLE LED MODE to CAM
#define BTN_MODE_CAM_TOGGLE_COLORMODE 3     // TOGGLE Cam ColorMode to OFF
#define BTN_MODE_CAM_TOGGLE_COLORPRESET 4   // TOGGLE Cam Colorpreset to OFF
#define BTN_MODE_SMART_HUE_TOGGLE 5         // TOGGLE HUE Support ON and OFF 
#define BTN_MODE_SMART_DREAM_TOGGLE 6       // TOGGLE DREAMSCREEN Support ON and OFF 
#define BTN_MODE_LAST BTN_MODE_SMART_DREAM_TOGGLE // LAST MODE NUM 

#define WIFI_FAIL_RECONNECT 0                // Reconnect 
#define WIFI_FAIL_RECONNECTREBOOT 1          // First reconnect than reboot
#define WIFI_FAIL_REBOOT 2                   // Reboot
#define WIFI_FAIL_MIN WIFI_FAIL_RECONNECT
#define WIFI_FAIL_MAX WIFI_FAIL_REBOOT

// *** DEFAULT PORTS ***
#define UDP_PORT 1337
#define TCP_PORT 1337
#define HTTP_PORT_FORWARD 81
#define HTTP_PORT 80

// *** LED SPECIFIC DEFINES ***
// Whats the maximum for LEDS
#define MAX_LEDS 256
#define MAX_CONTROL_LEDS 1     /* How many Control LEDs */

// Frame/Delay Max defaults
# define MAX_FRAME_LENGTH 100
# define MAX_DELAY_LENGTH 500 // Limit to 500ms!
// Relay Frame/Delay Max Defaults 
# define MAX_RELAY_DELAY_LENGTH 100
# define DEFAULT_RELAY_FRAME_LENGTH 15
// 2 Ports, Framelength will result in 50 frames
#if defined(USE_HAS_SECOND_PORT) || defined(CLONE_TO_SECOND_PORT) 
//Framelength will result in 50 frames
# define DEFAULT_FRAME_LENGTH 15
# define DEFAULT_DELAY_LENGTH 60
#else
# if defined(ESP32)
#   if ESP32_DEVICE_TYPE == ESP32_BRIDGE
//    Framelength will result in 60 frames
//    reduced Framelength because otherwise Hue Calc is to slow!
#     define DEFAULT_FRAME_LENGTH 10
// #     define DEFAULT_FRAME_LENGTH 15
#     define DEFAULT_DELAY_LENGTH 40
#   else
//    Framelength will result in 100 frames
#     define DEFAULT_FRAME_LENGTH 10
#     define DEFAULT_DELAY_LENGTH 40
#   endif
# else
//  Framelength will result in 100 frames
#   define DEFAULT_FRAME_LENGTH 10
#   define DEFAULT_DELAY_LENGTH 40
# endif
#endif

// Default LED Video Mode timeout
#define DEFAULT_LED_TIMEOUT 15000
// Default EEPROM AutoSave Timeout
#define DEFAULT_EEPROM_SAVE_TIMEOUT 5000

// Default WIFI Connect Retry timeout
#define DEFAULT_WIFI_CONNECT_TIMEOUT 20000
#define DEFAULT_WIFI_REBOOT_AFTER_FAILS 5
#define DEFAULT_WIFI_FORCE_REBOOT_AFTER_FAILS 60
#define DEFAULT_WIFI_STARTUP_CHECK_WAIT 15000

// Default REBOOT WPS Check Timeout
#define DEFAULT_WPS_REBOOT_CHECK_TIMOUT 10000
#define DEFAULT_WPS_TIMOUT 120000

// LED Controller Indexes
#define LED_PRT1IDX 0
#define LED_PRT2IDX 1
#define LED_PWRIDX 2
#define LED_CTRLIDX 3

// *** PIN DEFINES ***
/* NOT USEABLE PINS: 
 *  4 = internal LED 
 *  PSRAM /CE (pin 1) > ESP32 GPIO 16
    PSRAM SO (pin 2) > flash DO
    PSRAM SIO[2] (pin 3) > flash WP
    PSRAM SI (pin 5) > flash DI
    PSRAM SCLK (pin 6) > ESP32 GPIO 17
    PSRAM SIO[3] (pin 7) > flash HOLD
    PSRAM Vcc (pin 8) > ESP32 VCC_SDIO
 *  */
#if ESP32_DEVICE_TYPE == ESP32_WROOVER_CAM // ESP32 Wroover CAM Module! 
      // Product features
# define ENABLE_VIDEO       // Enable VIDEO Mode (LED Controller)
# define ENABLE_CAM         // Enable CAM Module
# define DISABLE_BLUETOOTH  // Disable Bluetooth for memory release
# define ENABLE_HUE         // Enable HUE Support for Wroover CAM Module

# define PWRLED_PIN 33
# define PWRLED_PIN_ON LOW
# define PWRLED_PIN_OFF HIGH
# define CTLLED_PIN -1
# define BTN_PIN 14
# define BTN_STATE_ON 1     // 1 means PUSHED
# define BTN_STATE_OFF 0    // 0 means NOT PUSHED
# define ENABLE_DBLRBT_WPS  // Enable double reboot WPS mode, needed of no BUTTON is connected!
# define IRRECV_PIN 2

# define CAM_BUFFER_COUNT     2
# define CAM_BUFFER_COUNT_MAX 5
# define CAM_SENSOR           CAM_SENSOR_OV2640
# define CAM_VFLIP            0
# define CAM_HMIRROR          0

# define LEDCTRL_PIN 15     // 15 used for BOOTMode stuff, see https://github.com/espressif/esptool/wiki/ESP32-Boot-Mode-Selection
# define LEDCTRL_PIN_SEC 16 // 16 used by PSRAM, TODO FIND ANOTHER PIN or limit to ONE PORT    

# define DEVICE_NAME "AmbiBack Cam Controller"
# define AMBIBACK_NAME "AmbiBack_Cam_wr_Controller"
# define NETWORK_NAME "ambibackcam"
# define SHORTCUT_NAME "AmbiBack Cam"
# define HTTP_BASE_URL AMBIBACK_FIRMWARE_BASE_URL "/wroover"
#elif ESP32_DEVICE_TYPE == ESP32_TTGO_CAM // ESP32 TTGO CAM Module! 
      // Product features
# define ENABLE_CAM         // Enable CAM Module
# define ENABLE_SERIAL_INPUT// Enable Serial INPUT commands
# define DISABLE_BLUETOOTH  // Disable Bluetooth for memory release

# define PWRLED_PIN -1      // NOT KNOWN
# define PWRLED_PIN_ON HIGH
# define PWRLED_PIN_OFF LOW
# define CTLLED_PIN -1
# define BTN_PIN 34
# define BTN_STATE_ON 0     // 1 means PUSHED
# define BTN_STATE_OFF 1    // 0 means NOT PUSHED
# define IRRECV_PIN -1
# define ENABLE_SSD1306     // Enable DISPLAY
# define ENABLE_AS312       // Enable movment detection sensor

# define CAM_BUFFER_COUNT     2
# define CAM_BUFFER_COUNT_MAX 2
# define CAM_SENSOR           CAM_SENSOR_OV2640
# define CAM_VFLIP            0
# define CAM_HMIRROR          0
# define ESP32_CUSTOM_CAM     TTGO_T_CAMERA_V1_7_2
# include "globaldefinesttgo.h" // Include extra PIN defines!

# define LEDCTRL_PIN 2     // 
# define LEDCTRL_PIN_SEC -1 // 

# define DEVICE_NAME "AmbiBack Cam Controller"
# define AMBIBACK_NAME "AmbiBack_Cam_tt_Controller"
# define NETWORK_NAME "ambibackcam"
# define SHORTCUT_NAME "AmbiBack Cam"
# define HTTP_BASE_URL AMBIBACK_FIRMWARE_BASE_URL "/ttgo"

#elif ESP32_DEVICE_TYPE == ESP32_TTGO_CAM_MIC // ESP32 TTGO CAM +Mic Module! 
      // Product features
# define ENABLE_CAM         // Enable CAM Support
# define ENABLE_MIC         // Enable MIC Support
# define ENABLE_SERIAL_INPUT// Enable Serial INPUT commands
# define DISABLE_BLUETOOTH  // Disable Bluetooth for memory release

# define PWRLED_PIN -1      // NOT KNOWN
# define PWRLED_PIN_ON HIGH
# define PWRLED_PIN_OFF LOW
# define CTLLED_PIN -1
# define BTN_PIN 15
# define BTN_STATE_ON 0     // 1 means PUSHED
# define BTN_STATE_OFF 1    // 0 means NOT PUSHED
# define IRRECV_PIN -1
# define ENABLE_SSD1306     // Enable DISPLAY
# define ENABLE_AS312       // Enable movment detection sensor

# define CAM_BUFFER_COUNT     2
# define CAM_BUFFER_COUNT_MAX 5
# define CAM_SENSOR           CAM_SENSOR_OV2640
# define CAM_VFLIP            0
# define CAM_HMIRROR          0
# define ESP32_CUSTOM_CAM     TTGO_T_CAMERA_MIC_V16
# include "globaldefinesttgo.h" // Include extra PIN defines!

# define LEDCTRL_PIN 2      //
# define LEDCTRL_PIN_SEC -1 //

# define DEVICE_NAME "AmbiBack Cam Controller"
# define AMBIBACK_NAME "AmbiBack_Cam_tm_Controller"
# define NETWORK_NAME "ambibackcam"
# define SHORTCUT_NAME "AmbiBack Cam"
# define HTTP_BASE_URL AMBIBACK_FIRMWARE_BASE_URL "/ttgotm"
#elif ESP32_DEVICE_TYPE == ESP32_TTGO_CAM_PLUS // ESP32 TTGO CAM Module! 
      // Product features
# define ENABLE_VIDEO       // Enable VIDEO Mode (LED Controller)
# define ENABLE_CAM         // Enable CAM Module
# define ENABLE_SERIAL_INPUT// Enable Serial INPUT commands
# define DISABLE_BLUETOOTH  // Disable Bluetooth for memory release

# define PWRLED_PIN_OFF LOW
# define CTLLED_PIN -1
# define BTN_PIN 34
# define BTN_STATE_ON 0     // 1 means PUSHED
# define BTN_STATE_OFF 1    // 0 means NOT PUSHED
# define IRRECV_PIN -1
// # define ENABLE_SSD1306     // Enable DISPLAY
# define ENABLE_AS312       // Enable movment detection sensor
# define PWRLED_PIN -1      // NOT KNOWN
# define PWRLED_PIN_ON HIGH

# define CAM_BUFFER_COUNT     2
# define CAM_BUFFER_COUNT_MAX 5
# define CAM_SENSOR           CAM_SENSOR_OV2640
# define CAM_VFLIP            0
# define CAM_HMIRROR          0
# define ESP32_CUSTOM_CAM     TTGO_T_CAMERA_PLUS
# include "globaldefinesttgo.h" // Include extra PIN defines!

# define LEDCTRL_PIN 2     // 
# define LEDCTRL_PIN_SEC -1 // 

# define DEVICE_NAME "AmbiBack Cam Controller"
# define AMBIBACK_NAME "AmbiBack_Cam_pl_Controller"
# define NETWORK_NAME "ambibackcam"
# define SHORTCUT_NAME "AmbiBack Cam"
# define HTTP_BASE_URL AMBIBACK_FIRMWARE_BASE_URL "/ttgopl"
#elif ESP32_DEVICE_TYPE == ESP32_TTGO_CAM_JOURNAL // ESP32 TTGO T-Journal Module!
      // Product features
# define ENABLE_VIDEO       // Enable VIDEO Mode (LED Controller)
# define ENABLE_CAM         // Enable CAM Module
# define ENABLE_SERIAL_INPUT// Enable Serial INPUT commands
# define DISABLE_BLUETOOTH  // Disable Bluetooth for memory release

# define PWRLED_PIN -1      // NOT KNOWN
# define PWRLED_PIN_ON HIGH
# define PWRLED_PIN_OFF LOW
# define CTLLED_PIN -1
# define BTN_PIN 32
# define BTN_STATE_ON 0     // 1 means PUSHED
# define BTN_STATE_OFF 1    // 0 means NOT PUSHED
# define IRRECV_PIN -1
# define ENABLE_SSD1306     // Enable DISPLAY

# define CAM_BUFFER_COUNT     2
# define CAM_BUFFER_COUNT_MAX 2
# define CAM_SENSOR           CAM_SENSOR_OV2640
# define CAM_VFLIP            0
# define CAM_HMIRROR          0
# define ESP32_CUSTOM_CAM     TTGO_T_JORNAL
# include "globaldefinesttgo.h" // Include extra PIN defines!

# define LEDCTRL_PIN 4     // 
# define LEDCTRL_PIN_SEC -1 // 

# define DEVICE_NAME "AmbiBack Cam Controller"
# define AMBIBACK_NAME "AmbiBack_Cam_jr_Controller"
# define NETWORK_NAME "ambibackcam"
# define SHORTCUT_NAME "AmbiBack Cam"
# define HTTP_BASE_URL AMBIBACK_FIRMWARE_BASE_URL "/ttgojr"
#elif ESP32_DEVICE_TYPE == ESP32_M5ST_CAM // ESP32 M5STACK CAM Module! 
      // Product features
# define ENABLE_CAM         // Enable CAM Module
# define ENABLE_SERIAL_INPUT    /* Enable Serial INPUT commands */
# undef MAX_SERIAL_RATE 
# define MAX_SERIAL_RATE 1500000/* Use different MAX SERIAL RATE */
# define ENABLE_DBLRBT_WPS  // Enable double reboot WPS mode, needed of no BUTTON is connected!
# define DISABLE_BLUETOOTH  // Disable Bluetooth for memory release

# define PWRLED_PIN 16      // Internal LED
# define PWRLED_PIN_ON HIGH
# define PWRLED_PIN_OFF LOW
# define CTLLED_PIN -1
# define BTN_PIN 4          // IO4 GROVE Interface pin 
# define BTN_STATE_ON 1     // 1 means PUSHED
# define BTN_STATE_OFF 0    // 0 means NOT PUSHED
# define IRRECV_PIN -1      // IR disabled

# define CAM_BUFFER_COUNT     2
# define CAM_BUFFER_COUNT_MAX 2
# define CAM_SENSOR           CAM_SENSOR_OV2640
# define CAM_VFLIP            0
# define CAM_HMIRROR          0
# define ESP32_CUSTOM_CAM     M5_ESP32CAM
# include "globaldefinesttgo.h" // Include extra PIN defines!

# define LEDCTRL_PIN 13     // IO13 GROVE Interface pin 
# define LEDCTRL_PIN_SEC -1 // 

# define DEVICE_NAME "AmbiBack Cam Controller"
# define AMBIBACK_NAME "AmbiBack_Cam_m5_Controller"
# define NETWORK_NAME "ambibackcam"
# define SHORTCUT_NAME "AmbiBack Cam"
# define HTTP_BASE_URL AMBIBACK_FIRMWARE_BASE_URL "/m5esp32"
/* Uncomment for esp32 debug!
#     undef ESP32_DEBUG_LEVEL  
# define ESP32_DEBUG_LEVEL  ESP_LOG_VERBOSE
*/
#elif ESP32_DEVICE_TYPE == ESP32_M5ST_CAM_X // ESP32_M5ST_CAM_X + PSRAM Module!
      // Product features
# define ENABLE_CAM         // Enable CAM Module
# define ENABLE_SERIAL_INPUT    /* Enable Serial INPUT commands */
# undef MAX_SERIAL_RATE 
# define MAX_SERIAL_RATE 1500000/* Use different MAX SERIAL RATE */
# define ENABLE_DBLRBT_WPS  // Enable double reboot WPS mode, needed of no BUTTON is connected!
# define DISABLE_BLUETOOTH  // Disable Bluetooth for memory release

# define PWRLED_PIN 14      // Internal LED
# define PWRLED_PIN_ON HIGH
# define PWRLED_PIN_OFF LOW
# define CTLLED_PIN -1
# define BTN_PIN 4          // IO4 GROVE Interface pin 
# define BTN_STATE_ON 1     // 1 means PUSHED
# define BTN_STATE_OFF 0    // 0 means NOT PUSHED
# define IRRECV_PIN -1      // IR disabled

# define CAM_BUFFER_COUNT     2
# define CAM_BUFFER_COUNT_MAX 5
# define CAM_SENSOR           CAM_SENSOR_OV2640
# define CAM_VFLIP            0
# define CAM_HMIRROR          0
# define ESP32_CUSTOM_CAM     M5_ESP32CAM_X
# include "globaldefinesttgo.h" // Include extra PIN defines!

# define LEDCTRL_PIN 13     // IO13 GROVE Interface pin 
# define LEDCTRL_PIN_SEC -1 // 
// MIC INTERFACE ! SPM1423

# define DEVICE_NAME "AmbiBack Cam Controller"
# define AMBIBACK_NAME "AmbiBack_Cam_5x_Controller"
# define NETWORK_NAME "ambibackcam"
# define SHORTCUT_NAME "AmbiBack Cam"
# define HTTP_BASE_URL AMBIBACK_FIRMWARE_BASE_URL "/m5camx"
/* Uncomment for esp32 debug!
#     undef ESP32_DEBUG_LEVEL  
# define ESP32_DEBUG_LEVEL  ESP_LOG_VERBOSE
/**/
#elif ESP32_DEVICE_TYPE == ESP32_M5ST_CAM_TIMER_X // ESP32 M5ST CAM TIMER X + 8MB PSRAM Module! 
      // Product features
# define ENABLE_CAM         // Enable CAM Module
# define ENABLE_SERIAL_INPUT    /* Enable Serial INPUT commands */
#     undef MAX_SERIAL_RATE 
# define MAX_SERIAL_RATE 1500000/* Use different MAX SERIAL RATE */
//# define ENABLE_DBLRBT_WPS  // Enable double reboot WPS mode, needed of no BUTTON is connected!
# define DISABLE_BLUETOOTH  // Disable Bluetooth for memory release

# define PWRLED_PIN 2      // Internal LED
# define PWRLED_PIN_ON HIGH
# define PWRLED_PIN_OFF LOW
# define CTLLED_PIN -1
# define BTN_PIN 38          // IO4 GROVE Interface pin 
# define BTN_STATE_ON 1     // 1 means PUSHED
# define BTN_STATE_OFF 0    // 0 means NOT PUSHED
# define IRRECV_PIN -1      // IR disabled

# define CAM_BUFFER_COUNT     3
# define CAM_BUFFER_COUNT_MAX 5
# define CAM_SENSOR           CAM_SENSOR_OV3660
# define CAM_VFLIP            1
# define CAM_HMIRROR          0
# define ESP32_CUSTOM_CAM     M5_ESP32CAM_TIMER_X
#     include "globaldefinesttgo.h" // Include extra PIN defines!

# define LEDCTRL_PIN 13     // IO13 GROVE Interface pin 
# define LEDCTRL_PIN_SEC -1 // 

# define DEVICE_NAME "AmbiBack CAM Controller"
# define AMBIBACK_NAME "AmbiBack_Cam_5tx_Controller"
# define NETWORK_NAME "ambibackcam"
# define SHORTCUT_NAME "AmbiBack Cam"
# define HTTP_BASE_URL AMBIBACK_FIRMWARE_BASE_URL "/m5camtx"
/* Uncomment for esp32 debug!
#     undef ESP32_DEBUG_LEVEL  
# define ESP32_DEBUG_LEVEL  ESP_LOG_VERBOSE
/**/

#elif ESP32_DEVICE_TYPE == ESP32_M5ST_ATOMLITE // ESP32 M5Stack Atom Lite Module! 
# define ENABLE_VIDEO       /* Enable VIDEO Mode (LED Controller) */
# define ENABLE_TCP         /* Enable TCP Service  */
# define DISABLE_BLUETOOTH  /* Disable Bluetooth for memory release */
# define ENABLE_SERIAL_INPUT    /* Enable Serial INPUT commands */
#undef MAX_SERIAL_RATE 
# define MAX_SERIAL_RATE 1500000/* Use different MAX SERIAL RATE */
# define ENABLE_HUE             /* Enable HUE Support for EPS32 */
# define ENABLE_WIFI_AP         /* Enable local WIFI AP */
# define ENABLE_WIFI_NAT        /* Enable NAT on WIFI AP */
// Use internal jquery files (For SoftAP Mode)
// #undef HTTP_GOOGLE_JQUERYFILES

# define BTN_PIN 39         /* GPIO39 */
# define BTN_STATE_ON 0     /* 0 means PUSHED */
# define BTN_STATE_OFF 1    /* 1 means NOT PUSHED */
// # define LEDCTRL_PIN 32     /* Use use PIN32 on Grove Connector */
# define LEDCTRL_PIN 26     /* Use use PIN26 (YELLOW) on Grove Connector */
# define LEDCTRL_PIN_SEC -1 /* - not used */

# define DEVICE_NAME "AmbiBack LED Controller"
# define AMBIBACK_NAME "AmbiBack_Pc_M5AL_Controller"
# define NETWORK_NAME "ambibackled"
# define SHORTCUT_NAME "AmbiBack LED"
# define HTTP_BASE_URL AMBIBACK_FIRMWARE_BASE_URL "/m5alit"

# define USE_WS2812_CONTROL_LEDS 
# define USE_WS2812_POWER_LED 0 /* Set to 0 to disable POWER LED */
# define WS2812_CONTROL_LEDS_STEPS 16
# define WS2812_CONTROL_LEDS_BRIGHTNESS 64
# define PWRLED_PIN 27
# define PWRLED_PIN_ON HIGH
# define PWRLED_PIN_OFF LOW
# define CTLLED_PIN -1
# define IRRECV_PIN 12

#elif ESP32_DEVICE_TYPE == ESP32_M5ST_ATOMMATRIX // ESP32 M5Stack Atom Matrix Module! 
# define ENABLE_VIDEO       /* Enable VIDEO Mode (LED Controller) */
# define ENABLE_TCP         /* Enable TCP Service  */
# define DISABLE_BLUETOOTH  /* Disable Bluetooth for memory release */
# define ENABLE_SERIAL_INPUT/* Enable Serial INPUT commands */
# undef MAX_SERIAL_RATE 
# define MAX_SERIAL_RATE 1500000/* Use different MAX SERIAL RATE */
# define ENABLE_HUE         /* Enable HUE Support for EPS32 */
# define ENABLE_WIFI_AP         /* Enable local WIFI AP */
# define ENABLE_WIFI_NAT        /* Enable NAT on WIFI AP */
// Use internal jquery files (For SoftAP Mode)
// #undef HTTP_GOOGLE_JQUERYFILES

# define BTN_PIN 39         /* GPIO39 */
# define BTN_STATE_ON 0     /* 0 means PUSHED */
# define BTN_STATE_OFF 1    /* 1 means NOT PUSHED */
# define LEDCTRL_PIN 26     /* Use use PIN26 (YELLOW) on Grove Connector */
# define LEDCTRL_PIN_SEC -1 /* - not used */

# define DEVICE_NAME "AmbiBack LED Controller"
# define AMBIBACK_NAME "AmbiBack_Pc_M5AX_Controller"
# define NETWORK_NAME "ambibackled"
# define SHORTCUT_NAME "AmbiBack LED"
# define HTTP_BASE_URL AMBIBACK_FIRMWARE_BASE_URL "/m5almx"

# define USE_WS2812_CONTROL_LEDS 
# undef MAX_CONTROL_LEDS  
# define MAX_CONTROL_LEDS 25    /* How many Control LEDs */
# define USE_WS2812_POWER_LED 0 /* Set to 0 to disable POWER LED */
# define WS2812_CONTROL_LEDS_STEPS 16
# define WS2812_CONTROL_LEDS_BRIGHTNESS 64
# define PWRLED_PIN 27
# define PWRLED_PIN_ON HIGH
# define PWRLED_PIN_OFF LOW
# define CTLLED_PIN -1
# define IRRECV_PIN 12

#elif ESP32_DEVICE_TYPE == ESP32_NAKED // ESP32 Naked Module! 
# define ENABLE_VIDEO       // Enable VIDEO Mode (LED Controller)
# define ENABLE_TCP         // Enable TCP Service 
# define DISABLE_BLUETOOTH  // Disable Bluetooth for memory release

# define BTN_PIN 0 // GPIO0 - EN / FLash Button!
# define BTN_STATE_ON 0     // 0 means PUSHED
# define BTN_STATE_OFF 1    // 1 means NOT PUSHED
# define LEDCTRL_PIN 13     // Use use PIN13, directly besides GND
# define LEDCTRL_PIN_SEC -1 // - not used
# define ENABLE_SERIAL_INPUT// Enable Serial INPUT commands
# define ENABLE_HUE         // Enable HUE Support for EPS32
# define ENABLE_WIFI_AP         /* Enable local WIFI AP */
# define ENABLE_WIFI_NAT        /* Enable NAT on WIFI AP */
// Use internal jquery files (For SoftAP Mode)
// #undef HTTP_GOOGLE_JQUERYFILES

# define DEVICE_NAME "AmbiBack LED Controller"
# define AMBIBACK_NAME "AmbiBack_Pc_Controller"
# define NETWORK_NAME "ambibackled"
# define SHORTCUT_NAME "AmbiBack LED"
# define HTTP_BASE_URL AMBIBACK_FIRMWARE_BASE_URL "/naked"
// TODO USE INTERNAL LED!
# define PWRLED_PIN 2
# define PWRLED_PIN_ON HIGH
# define PWRLED_PIN_OFF LOW
# define CTLLED_PIN -1
# define IRRECV_PIN -1
#elif ESP32_DEVICE_TYPE == ESP32_BRIDGE // ESP32 Bridge Module! 
      // Product features
# define ENABLE_DREAMSCREEN // Enable DREAMSCREEN Support for BRIDGE
# define ENABLE_HUE         // Enable HUE Support for EPS32
# define ENABLE_SERIAL_INPUT// Enable Serial INPUT commands
# define DISABLE_BLUETOOTH  // Disable Bluetooth for memory release

# define BTN_PIN 0 // GPIO0 - EN / FLash Button!
# define BTN_STATE_ON 0     // 0 means PUSHED
# define BTN_STATE_OFF 1    // 1 means NOT PUSHED
# define LEDCTRL_PIN 13     // FOR NOW, just use PIN15 
# define LEDCTRL_PIN_SEC -1 // - not used

# define DEVICE_NAME "AmbiBack Bridge Controller"
# define AMBIBACK_NAME "AmbiBack_Bridge_Controller"
# define NETWORK_NAME "ambiback"
# define SHORTCUT_NAME "AmbiBack Bridge"
# define HTTP_BASE_URL AMBIBACK_FIRMWARE_BASE_URL "/bridge"
// TODO USE INTERNAL LED!
# define PWRLED_PIN 2
# define PWRLED_PIN_ON HIGH
# define PWRLED_PIN_OFF LOW
# define CTLLED_PIN -1
# define IRRECV_PIN -1
/* Uncomment for esp32 debug!
#     undef ESP32_DEBUG_LEVEL
# define ESP32_DEBUG_LEVEL  ESP_LOG_VERBOSE
/**/
#elif ESP32_DEVICE_TYPE == ESP32_AMBIBACK_LEDV1 // ESP32 AMBIBACK LED Module (M5Stack Atom Lite Module)!
# define ENABLE_VIDEO           /* Enable VIDEO Mode (LED Controller) */
# define ENABLE_TCP             /* Enable TCP Service  */
# define DISABLE_BLUETOOTH  /* Disable Bluetooth for memory release */
# define ENABLE_SERIAL_INPUT    /* Enable Serial INPUT commands */
# undef MAX_SERIAL_RATE 
# define MAX_SERIAL_RATE 1500000/* Use different MAX SERIAL RATE */
# define ENABLE_DREAMSCREEN     /* Enable DREAMSCREEN Support own PCB! */
# define ENABLE_HUE             /* Enable HUE Support for EPS32 */
# define ENABLE_WIFI_AP         /* Enable local WIFI AP */
# define ENABLE_WIFI_NAT        /* Enable NAT on WIFI AP */
// Use internal jquery files (For SoftAP Mode)
// #undef HTTP_GOOGLE_JQUERYFILES

# define BTN_PIN 39         /* GPIO39 */
# define BTN_STATE_ON 0     /* 0 means PUSHED */
# define BTN_STATE_OFF 1    /* 1 means NOT PUSHED */
# define LEDCTRL_PIN 33     /* Use use PIN33 on 5Port Connector ROW */
# define LEDCTRL_PIN_SEC -1 /* - not used */
// TODO: USB DETECTION PORT Support!
# define USBDTCT_PIN 21     /* Use use PIN21 on 4Port Connector ROW */

# define DEVICE_NAME "AmbiBack LED Controller"
# define AMBIBACK_NAME "AmbiBack_Led_M5A1_Controller"
# define NETWORK_NAME "ambibackled"
# define SHORTCUT_NAME "AmbiBack LED"
# define HTTP_BASE_URL AMBIBACK_FIRMWARE_BASE_URL "/ambiledv1"

# define USE_WS2812_CONTROL_LEDS 
# define USE_WS2812_POWER_LED 0 /* Set to 0 to disable POWER LED */
# define WS2812_CONTROL_LEDS_STEPS 16
# define WS2812_CONTROL_LEDS_BRIGHTNESS 64
# define PWRLED_PIN 27
# define PWRLED_PIN_ON HIGH
# define PWRLED_PIN_OFF LOW
# define CTLLED_PIN -1
# define IRRECV_PIN 12

#else 
// WILL FAIL!
UNKNOWN BOARD
#endif


#if defined(ENABLE_CAM) 
# define ENABLE_RELAY           // Enable support for RELAY Mode (Camera)
# define LED_DEF_MINLUMINOSITY  5
#else
# define LED_DEF_MINLUMINOSITY  0
#endif

// --- Global Includes
#include "Arduino.h"
#include "global.h"

// Moved below header includes!
# if defined(ESP32_REDIR_DEBUG)
#   undef log_v
#   undef log_d
#   undef log_i
#   undef log_w
#   undef log_e
#   define log_v(format, ...) global::dbgprintf("esp32debug verbose: " format "\n", ##__VA_ARGS__)
#   define log_d(format, ...) global::dbgprintf("esp32debug debug: " format "\n", ##__VA_ARGS__)
#   define log_i(format, ...) global::dbgprintf("esp32debug info: " format "\n", ##__VA_ARGS__)
#   define log_w(format, ...) global::errprintf("esp32debug warning: " format "\n", ##__VA_ARGS__)
#   define log_e(format, ...) global::errprintf("esp32debug error: " format "\n", ##__VA_ARGS__)
# endif
// ---

#if defined(ENABLE_SSD1306)
# define SSD1306_MAXSTR 22
#endif

// *******************
// Global Helper functions
// Return PSRAM ONLY if successfully found!
static void *_malloc_psram(size_t size) { return (psramFound() ? heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : heap_caps_malloc(size, MALLOC_CAP_8BIT) ); }
// *******************

// --- Bit Helper functions
/*
void  SetBit( int A[],  int k ) {
      A[k/32] |= 1 << (k%32);  // Set the bit at the k-th position in A[i]
}

void  ClearBit( int A[],  int k ) {
      A[k/32] &= ~(1 << (k%32));
}

int TestBit( int A[],  int k ) {
      return ( (A[k/32] & (1 << (k%32) )) != 0 ) ;     
}
*/
#define SetBit(A,k)     ( A[(k/32)] |= (1 << (k%32)) )
#define ClearBit(A,k)   ( A[(k/32)] &= ~(1 << (k%32)) )            
#define TestBit(A,k)    ( A[(k/32)] & (1 << (k%32)) )
// #define BitIndex(a,b,c) ((a*(frameHeight*frameWidth)) + (b*frameWidth) + c)
#define BitIndex(a,b,lev1size) ((a*(lev1size)) + b)
#define BitIndexEx(a,b,c,lev1size,lev2size) ((a*(lev1size)) + (b*lev2size) + c)
// ---

#endif
