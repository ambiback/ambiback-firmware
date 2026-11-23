#ifndef global_h
#define global_h

// Needed for global pointer
#include "LEDHelper.h"
#include "eepromhelper.h"
#include "ambiwebserver.h"  // Include WebServer reference now!

#if defined(ENABLE_HUE)
# include "huehelper.h"
#endif
#if defined(ENABLE_DREAMSCREEN)
# include "dreamscreen.h"
#endif

class DebugHelper;

// Syslog Server variables
// Default syslog target. Empty means no target until one is configured.
#ifndef AMBIBACK_SYSLOG_SERVER
#define AMBIBACK_SYSLOG_SERVER ""
#endif
#define SYSLOG_SERVER PSTR(AMBIBACK_SYSLOG_SERVER)
#define SYSLOG_PORT 10514
#define SYSLOG_APP_NAME AMBIBACK_NAME

// Config Size defines
#define WLAN_SSID_MAX 33
#define WLAN_KEY_MAX 65
#define DEVICE_NAME_MAX 65
#define DEVICE_ID_MAX 33
#define DEVICE_MAC_MAX 20
#define WIFI_PROVISIONING_TOKEN_MAX 17
#define WIFI_PROVISIONING_BOOT_WINDOW_DEFAULT_MINUTES 5
#define WIFI_PROVISIONING_BOOT_WINDOW_MIN_MINUTES 1
#define WIFI_PROVISIONING_BOOT_WINDOW_MAX_MINUTES 60
#define STR_IPNAME_MAX 65
#define BYTE_VARIABLE_MAX 1
#define INT_VARIABLE_MAX 4
#define ESP32_PREF_KEYMAX 16
#if defined(ENABLE_HUE)
# define HUE_USER_MAX 129
# define HUE_TASK_MODE_INIT 0
# define HUE_TASK_MODE_LOOP 1
# define HUE_CONNECT_TIMEOUT 10
// # define HUE_WRITEFAIL_TIMEOUT 100
# define HUE_WRITEFAIL_TIMEOUT 0
# define HUE_WRITEFAIL_DELAY 50

#endif
#if defined(ENABLE_RELAY) 
# define AMBIBACK_MAX_DEVICES 20
# define AMBIBACK_MAX_IDLEN 16
# define DEVICE_NOT_FOUND -1
# define VALUE_NOT_FOUND -1
# define AMBIBACK_INIT_RETRY_WAIT 5000
# define RELAY_FRAME_LENGTH 20
# define RELAY_DELAY_LENGTH 40
#endif
enum RELAYMODES
{
  RELAY_UDP = 1,
  RELAY_TCP = 2,
  RELAY_BT = 3,
};

// Serial Helper constants
#define SERIAL_CMD_MAXLENGTH 16
#define SERIAL_CMD_TIMEOUT 10
#define SERIAL_LEDDATA_MAXLENGTH 1152
#define SERIAL_LEDDATA_TIMEOUT 50
#define SERIAL_HEADERSIZE 4
#define SERIAL_CHUNKSIZE 4
#define SERIAL_INVALIDBYTE -1

// Other Helper constants
#define DIAG_DEBUG_OUTPUT_TIME 5000 // Output diagnostic data every 5 seconds
#define SYS_FREEMEM_CHECK_TIME 5000 // Check Free Memory every 5 seconds
#define SYS_FREEMEM_MIN 20          // This is the minimum amount of FREE memory we need to operate!
#define SYS_FREEMEM_OTA_MIN 40      // This is the minimum amount of FREE memory we need to perform OTA with own task!
#define SYS_ALLOC_PSRAM_MIN 4096    // Applied to heap_caps_malloc_extmem_enable to use more PSRAM dynamic allocs above 1024 bytes
#define SYS_TASK_MIN_STACK 4096     // Minimum stack memory applied for tasks we create!

enum doActions {
    ACTION_off = 0x00, 
#ifdef ENABLE_DREAMSCREEN
    ACTION_AddDreamScreenToNetwork = 0x01, 
#endif 
};

#if defined(ENABLE_CAM)
enum CAMPRESETS
{
  CAM_auto_default = 1,
  CAM_auto_sunny = 2,
  CAM_auto_cloudy = 3,
  CAM_auto_colorful = 4,
  CAM_auto_office = 5,
  CAM_manual_default = 6,

  CAM_custom = 999,
};

enum EXPPRESETS
{
  CAM_exp_adaptivelens = 1,
  CAM_exp_adaptive = 2,
  CAM_exp_normal= 3,
  CAM_exp_bright= 4,
  CAM_exp_verybright = 5,
  CAM_exp_autocontrol = 6,
  CAM_exp_adaptiveraw = 7,
  CAM_exp_adaptivelensraw = 8,

  CAM_exp_custom = 999,
};

enum INNERPRESETS
{
  CAM_inn_veryminimal = 1,
  CAM_inn_minimal = 2,
  CAM_inn_normal = 3,
  CAM_inn_large = 4,
  CAM_inn_verylarge = 5,
  CAM_inn_custom = 999,
};

# define CAMFREQ_MINIDX 0
# define CAMFREQ_MAXIDX 10
enum CAMFREQ
{
  FREQ_10 = 0,
  FREQ_12 = 1,
  FREQ_14 = 2,
  FREQ_16 = 3,
  FREQ_18 = 4,
  FREQ_20 = 5,
  FREQ_22 = 6,
  FREQ_24 = 7,
  FREQ_25 = 8,
  FREQ_26 = 9,
  FREQ_27 = 10,
};
# define CAMFLICKER_MINIDX 0
# define CAMFLICKER_MAXIDX 2
enum CAMFLICKER
{
  FLICKER_AUTO = 0,
  FLICKER_50 = 1,
  FLICKER_60 = 2,
};

# define CAMPERF_MINIDX 0
# define CAMPERF_MAXIDX 6
enum CAMPERF
{
  FPS_50  = 0,
  FPS_45  = 1,
  FPS_40  = 2,
  FPS_35  = 3,
  FPS_30  = 4,
  FPS_25  = 5,
  FPS_20  = 6,
};


# define BLACKDETECT_MAX 10
# if CAM_SENSOR == CAM_SENSOR_OV2640
# define BLACKDETECT_DEFAULT 3        // Set to higher value to avoid low color flickering
# define BLACKACTTHRESHOLD_DEFAULT 20 // 
# elif CAM_SENSOR == CAM_SENSOR_OV3660
# define BLACKDETECT_DEFAULT 2        // Lower default for OV3660
# define BLACKACTTHRESHOLD_DEFAULT 15 // Lower default for OV3660
# endif
# define BLACKREMTHRESHOLD_DEFAULT 5
# define BLACK_AUTOONOFFSECONDS 15
# define BLACK_AUTOCPULOWSECONDS 60

# define CAM_WHITE_AEC_CHANGEPERC 8   // Old default 5
# define CAM_ADPT_WHITE_MIN 25        // Old default 25
# define CAM_ADPT_BEFORE_ACT 16
# define CAM_ADPT_CHANGE_DELAY 500

# define CAM_ADPT_GAIN_UP 48
# define CAM_ADPT_GAIN_DOWN 92

# define RESOLUTION_MINIDX 0
# define RESOLUTION_MAXIDX 11
static int FRAMESIZE_WIDTH[12] = { 160, 128, 176, 240, 320, 400, 640, 800, 1024, 1280, 1600, 2048};
static int FRAMESIZE_HEIGHT[12] = { 120, 160, 144, 176, 240, 296, 480, 600, 768, 1024, 1200, 1536};

# if CAM_SENSOR == CAM_SENSOR_OV2640
#   define CAM_ADPT_BRIGHTNESS -2
#   define CAM_ADPT_AEC_MIN 128
#   define CAM_ADPT_AEC_MAX 336
//  olds default 368
#   define CAM_ADPT_DEFAULT_MODE  CAM_exp_adaptive
#   define WHITEACTTHRESHOLD_LOW_DEFAULT 160
#   define WHITEACTTHRESHOLD_HIGH_DEFAULT 224
#   define WHITEACTTHRESHOLD_DEFAULT 184
#   define CAM_ADPT_GAIN_DEFAULT 4
#   define CAM_ADPT_GAIN_MIN 0
#   define CAM_ADPT_GAIN_MAX 6
#   define CAM_ADPT_GAIN_MAX_LOW 2
#   define CAM_ADPT_GAIN_MAX_HIGH 9
# elif CAM_SENSOR == CAM_SENSOR_OV3660
#   define CAM_ADPT_BRIGHTNESS 0
#   define CAM_ADPT_AEC_MIN 600
#   define CAM_ADPT_AEC_MAX 1200
#   define CAM_ADPT_DEFAULT_MODE  CAM_exp_adaptive
#   define WHITEACTTHRESHOLD_LOW_DEFAULT 160
#   define WHITEACTTHRESHOLD_HIGH_DEFAULT 224
#   define WHITEACTTHRESHOLD_DEFAULT 184
#   define CAM_ADPT_GAIN_DEFAULT 6
#   define CAM_ADPT_GAIN_MIN 1
#   define CAM_ADPT_GAIN_MAX 6
#   define CAM_ADPT_GAIN_MAX_LOW 2
#   define CAM_ADPT_GAIN_MAX_HIGH 9
# else
#   error "Please select sensor type"
# endif

#endif

// global defines
#define _DEBUG

#ifdef _DEBUG
# define DEBUG_ESP_WIFI
# define DEBUG_ESP_PORT
# define _DEBUG_SERIAL true
// DISABLE/ENABLE UDP DEBUG on DEMAND!
# define _DEBUG_UDP false

# ifdef _DEBUG_UDP
#   include <WiFi.h>
#   include "Syslog.h"
# endif 
#endif 

#if defined(ENABLE_CAM)
class CamProcessor;  /* at :: scope; defined in camprocessor.h */
#endif

class global
{
  friend class DebugHelper;
public:
  static void Init();
#if defined(ENABLE_CAM)
  static bool PrepareInitCam();
  static void DeInitCam();
  static void InitCam();
  static void InitCamClck(int iMul, int iPre, int iPclk);
  static void SetCamPerformance(int iIndex) { global::SetCamPerformance(iIndex, true); }; 
  static void SetCamPerformance(int iIndex, bool bEpprom);
  inline static int GetCamPerformance(void) { return m_iCamPerformance; };
  static int GetCamPerformanceFrameTime(void);
  static void SetCamResolution(int iIndex) { global::SetCamResolution(iIndex, true); }; 
  static void SetCamResolution(int iIndex, bool bEpprom) { global::SetCamResolution(iIndex, true, true); }; 
  static void SetCamResolution(int iIndex, bool bEpprom, bool bStoreVal);
  inline static int GetCamResolution(void) { return m_iCamIndex; };
  static void SetCamFreq(int iIndex) { global::SetCamFreq(iIndex, true); }; 
  static void SetCamFreq(int iIndex, bool bEpprom);
  inline static int GetCamFreq(void) { return m_iCamFreq; };
  static int GetCamFreqMhz(void);
  static void SetCamFrameBuffer(int iNew) { global::SetCamFrameBuffer(iNew, true); }; 
  static void SetCamFrameBuffer(int iNew, bool bEpprom);
  inline static int GetCamFrameBuffer(void) { return m_iCamFrameBuffer; };
  static int GetCamBandMode(void);
  static void SetCamFlicker(int iIndex) { global::SetCamFlicker(iIndex, true); }; 
  static void SetCamFlicker(int iIndex, bool bEpprom);
  inline static int GetCamFlicker(void) { return m_iCamFlicker; };
# if CAM_SENSOR == CAM_SENSOR_OV3660
  static void SetCamOverclock(byte  bNew) { global::SetCamOverclock(bNew, true); }; 
  static void SetCamOverclock(byte  bNew, bool bEpprom);
  inline static byte GetCamOverclock(void) { return m_bCamOverclock; };
# endif
  static void SetCamColorCorRed(byte bNew) { global::SetCamColorCorRed(bNew, true); }; 
  static void SetCamColorCorRed(byte bNew, bool bEpprom);
  inline static byte GetCamColorCorRed(void) { return m_bCamColorCorRed; };
  static void SetCamColorCorGreen(byte bNew) { global::SetCamColorCorGreen(bNew, true); }; 
  static void SetCamColorCorGreen(byte bNew, bool bEpprom);
  inline static byte GetCamColorCorGreen(void) { return m_bCamColorCorGreen; };
  static void SetCamColorCorBlue(byte bNew) { global::SetCamColorCorBlue(bNew, true); }; 
  static void SetCamColorCorBlue(byte bNew, bool bEpprom);
  inline static byte GetCamColorCorBlue(void) { return m_bCamColorCorBlue; };
  static void SetCamActivationThreshold(byte bNew) { global::SetCamActivationThreshold(bNew, true); }; 
  static void SetCamActivationThreshold(byte bNew, bool bEpprom);
  inline static byte GetCamActivationThreshold(void) { return m_bCamActivationThreshold; };
  static void SetCamBlackRemoveThreshold(byte bNew) { global::SetCamBlackRemoveThreshold(bNew, true); }; 
  static void SetCamBlackRemoveThreshold(byte bNew, bool bEpprom);
  inline static byte GetCamBlackRemoveThreshold(void) { return m_bCamBlackRemoveThreshold; };
  static void SetCamMinPixelAmount(byte bNew) { global::SetCamMinPixelAmount(bNew, true); };
  static void SetCamMinPixelAmount(byte bNew, bool bEpprom);
  inline static byte GetCamMinPixelAmount(void) { return m_bCamMinPixelAmount; };
  static void SetCamAutoOnOff(byte bNew) { global::SetCamAutoOnOff(bNew, true); }; 
  static void SetCamAutoOnOff(byte bNew, bool bEpprom);
  inline static byte GetCamAutoOnOff(void) { return m_bCamAutoOnOff; };
  static void SetCamAutoOnOffDelay(int iNew) { global::SetCamAutoOnOffDelay(iNew, true); }; 
  static void SetCamAutoOnOffDelay(int iNew, bool bEpprom);
  inline static int GetCamAutoOnOffDelay(void) { return m_iCamAutoOnOffDelay; };
  static void SetCamContrast(int iNew) { global::SetCamContrast(iNew, true); }; 
  static void SetCamContrast(int iNew, bool bEpprom);
  inline static int GetCamContrast(void) { return m_iCamContrast; };
  static void SetCamBrightness(int iNew) { global::SetCamBrightness(iNew, true); }; 
  static void SetCamBrightness(int iNew, bool bEpprom);
  inline static int GetCamBrightness(void) { return m_iCamBrightness; };
  static void SetCamSaturation(int iNew) { global::SetCamSaturation(iNew, true); }; 
  static void SetCamSaturation(int iNew, bool bEpprom);
  inline static int GetCamSaturation(void) { return m_iCamSaturation; };
  static void SetCamSpecialEffect(int iNew) { global::SetCamSpecialEffect(iNew, true); }; 
  static void SetCamSpecialEffect(int iNew, bool bEpprom);
  inline static int GetCamSpecialEffect(void) { return m_iCamSpecialEffect; };
  static void SetCamWbMode(int iNew) { global::SetCamWbMode(iNew, true); }; 
  static void SetCamWbMode(int iNew, bool bEpprom);
  inline static int GetCamWbMode(void) { return m_iCamWbMode; };
  static void SetCamAeLevels(int iNew) { global::SetCamAeLevels(iNew, true); }; 
  static void SetCamAeLevels(int iNew, bool bEpprom);
  inline static int GetCamAeLevels(void) { return m_iCamAeLevels; };
  static void SetCamQuality(int iNew, bool bEpprom);
  static void SetCamQuality(int iNew) { global::SetCamQuality(iNew, true); }; 
  inline static int GetCamQuality(void) { return m_iCamQuality; };
  static void SetCamAec2(int iNew, bool bEpprom);
  static void SetCamAec2(int iNew) { global::SetCamAec2(iNew, true); }; 
  inline static int GetCamAec2(void) { return m_iCamAec2; };
  static void SetCamAecValue(int iNew, bool bEpprom);
  static void SetCamAecValue(int iNew) { global::SetCamAecValue(iNew, true); }; 
  inline static int GetCamAecValue(void) { return m_iCamAecValue; };
  static void SetCamAwbGainControl(int iNew, bool bEpprom);
  static void SetCamAwbGainControl(int iNew) { global::SetCamAwbGainControl(iNew, true); }; 
  inline static int GetCamAwbGainControl(void) { return m_iCamAwbGainControl; };
  static void SetCamHMirror(int iNew, bool bEpprom);
  static void SetCamHMirror(int iNew) { global::SetCamHMirror(iNew, true); }; 
  inline static int GetCamHMirror(void) { return m_iCamHMirror; };
  static void SetCamVFlip(int iNew, bool bEpprom);
  static void SetCamVFlip(int iNew) { global::SetCamVFlip(iNew, true); }; 
  inline static int GetCamVFlip(void) { return m_iCamVFlip; };
  static void SetCamHighQuality(byte bNew) { global::SetCamHighQuality(bNew, true); }; 
  static void SetCamHighQuality(byte bNew, bool bEpprom);
  inline static byte GetCamHighQuality(void) { return m_bCamHighQuality; };
  static void SetCamBPC(int iNew, bool bEpprom);
  static void SetCamBPC(int iNew) { global::SetCamBPC(iNew, true); }; 
  inline static int GetCamBPC(void) { return m_iCamBPC; };
  static void SetCamWPC(int iNew, bool bEpprom);
  static void SetCamWPC(int iNew) { global::SetCamWPC(iNew, true); }; 
  inline static int GetCamWPC(void) { return m_iCamWPC; };
  static void SetCamDenoise(int iNew, bool bEpprom);
  static void SetCamDenoise(int iNew) { global::SetCamDenoise(iNew, true); }; 
  inline static int GetCamDenoise(void) { return m_iCamDenoise; };
  static void SetCamLenc(int iNew, bool bEpprom);
  static void SetCamLenc(int iNew) { global::SetCamLenc(iNew, true); }; 
  inline static int GetCamLenc(void) { return m_iCamLenc; };
  static void SetCamRawGMA(int iNew, bool bEpprom);
  static void SetCamRawGMA(int iNew) { global::SetCamRawGMA(iNew, true); }; 
  inline static int GetCamRawGMA(void) { return m_iCamRawGMA; };

  static void SetCamAutoExposurecontrol(int iNew, bool bEpprom);
  static void SetCamAutoExposurecontrol(int iNew) { global::SetCamAutoExposurecontrol(iNew, true); }; 
  inline static int GetCamAutoExposurecontrol(void) { return m_iCamAutoExposurecontrol; };

  static void SetCamDcw(int iNew, bool bEpprom);
  static void SetCamDcw(int iNew) { global::SetCamDcw(iNew, true); }; 
  inline static int GetCamDcw(void) { return m_iCamDcw; };

  static void SetCamWhitebalance(int iNew, bool bEpprom);
  static void SetCamWhitebalance(int iNew) { global::SetCamWhitebalance(iNew, true); }; 
  inline static int GetCamWhitebalance(void) { return m_iCamAutoWhitebalance; };
  
  static void SetCamGaincontrol(int iNew, bool bEpprom);
  static void SetCamGaincontrol(int iNew) { global::SetCamGaincontrol(iNew, true); }; 
  inline static int GetCamGaincontrol(void) { return m_iCamGaincontrol; };

  static void SetCamAgcGain(int iNew, bool bEpprom);
  static void SetCamAgcGain(int iNew) { global::SetCamAgcGain(iNew, true); }; 
  inline static int GetCamAgcGain(void) { return m_iCamAgcGain; };
  
  static void SetCamGainceilingSensor(int iNew);
  static void SetCamColorbar(int iNew);

  static void SetCamAdaptiveExposurecontrol(int iNew, bool bEpprom);
  static void SetCamAdaptiveExposurecontrol(int iNew) { global::SetCamAdaptiveExposurecontrol(iNew, true); }; 
  inline static int GetCamAdaptiveExposurecontrol(void) { return m_iCamAdaptiveExposurecontrol; };
  static void SetCamAecMinValue(int iNew, bool bEpprom);
  static void SetCamAecMinValue(int iNew) { global::SetCamAecMinValue(iNew, true); }; 
  inline static int GetCamAecMinValue(void) { return m_iCamAecMinValue; };
  static void SetCamAecMaxValue(int iNew, bool bEpprom);
  static void SetCamAecMaxValue(int iNew) { global::SetCamAecMaxValue(iNew, true); }; 
  inline static int GetCamAecMaxValue(void) { return m_iCamAecMaxValue; };
  static void SetCamAdaptiveChangePercent(int iNew, bool bEpprom);
  static void SetCamAdaptiveChangePercent(int iNew) { global::SetCamAdaptiveChangePercent(iNew, true); }; 
  inline static int GetCamAdaptiveChangePercent(void) { return m_iCamAdaptiveChangePercent; };
  static void SetCamAdaptiveChangeDelay(int iNew, bool bEpprom);
  static void SetCamAdaptiveChangeDelay(int iNew) { global::SetCamAdaptiveChangeDelay(iNew, true); }; 
  inline static int GetCamAdaptiveChangeDelay(void) { return m_iCamAdaptiveChangeDelay; };
  static void SetCamAdaptiveGaincontrol(int iNew, bool bEpprom);
  static void SetCamAdaptiveGaincontrol(int iNew) { global::SetCamAdaptiveGaincontrol(iNew, true); }; 
  inline static int GetCamAdaptiveGaincontrol(void) { return m_iCamAdaptiveGaincontrol; };
  
  static void SetCamWhiteActivationTreshold(byte bNew) { global::SetCamWhiteActivationTreshold(bNew, true); }; 
  static void SetCamWhiteActivationTreshold(byte bNew, bool bEpprom);
  inline static byte GetCamWhiteActivationTreshold(void) { return m_bCamWhiteActivationTreshold; };
  static void SetCamBeforeWhiteActivationTreshold(byte bNew) { global::SetCamBeforeWhiteActivationTreshold(bNew, true); }; 
  static void SetCamBeforeWhiteActivationTreshold(byte bNew, bool bEpprom);
  inline static byte GetCamBeforeWhiteActivationTreshold(void) { return m_bCamBeforeWhiteActivationTreshold; };
  static void SetCamWhiteMinimumAmount(byte bNew) { global::SetCamWhiteMinimumAmount(bNew, true); }; 
  static void SetCamWhiteMinimumAmount(byte bNew, bool bEpprom);
  inline static byte GetCamWhiteMinimumAmount(void) { return m_bCamWhiteMinimumAmount; };
  static void SetCamAdaptiveGainDown(byte bNew) { global::SetCamAdaptiveGainDown(bNew, true); }; 
  static void SetCamAdaptiveGainDown(byte bNew, bool bEpprom);
  inline static byte GetCamAdaptiveGainDown(void) { return m_bCamAdaptiveGainDown; };
  static void SetCamAdaptiveGainUp(byte bNew) { global::SetCamAdaptiveGainUp(bNew, true); }; 
  static void SetCamAdaptiveGainUp(byte bNew, bool bEpprom);
  inline static byte GetCamAdaptiveGainUp(void) { return m_bCamAdaptiveGainUp; };
  static void SetCamAdaptiveGainMax(byte bNew) { global::SetCamAdaptiveGainMax(bNew, true); }; 
  static void SetCamAdaptiveGainMax(byte bNew, bool bEpprom);
  inline static byte GetCamAdaptiveGainMax(void) { return m_bCamAdaptiveGainMax; };

  inline static byte GetCamJpegCalcDirect(void) { return m_bCamJpegCalcDirect; };
#endif

#if defined(ENABLE_HUE)
  static void InitHue();
  inline static byte GetHueSupport(void) { return m_pHueHelper ? m_pHueHelper->GetSupport() : 0; }
  static void SetHueSupport(byte bNew, bool bEpprom, bool bInitHue); 
  static void SetHueSupport(byte bNew, bool bEpprom) { global::SetHueSupport(bNew, true, true); } 
  static void SetHueSupport(byte bNew) { global::SetHueSupport(bNew, true); } 
  static void SetFrameLengthDtls(byte bNew, bool bEpprom);
  static void SetFrameLengthDtls(byte bNew) { global::SetFrameLengthDtls(bNew, true); }; 
  inline static byte GetFrameLengthDtls(void) { return m_pHueHelper ? m_pHueHelper->GetFrameLengthDtls() : 40; }
  static void SetHandshakeTimeout(byte bNew, bool bEpprom);
  static void SetHandshakeTimeout(byte bNew) { global::SetHandshakeTimeout(bNew, true); }; 
  inline static byte GetHandshakeTimeout(void) { return m_pHueHelper ? m_pHueHelper->GetHandshakeTimeout() : 10; }
  inline static char* GetHueBridgeIP() { return m_pHueHelper ? m_pHueHelper->GetBridgeIP() : (char*)""; } 
  static void SetHueBridgeIP(char* pszNew, bool bEpprom); 
  static void SetHueBridgeIP(char* pszNew) { global::SetHueBridgeIP(pszNew, true); } 
  inline static char* GetHueBridgeUser() { return m_pHueHelper ? m_pHueHelper->GetBridgeUser() : (char*)""; } 
  static void SetHueBridgeUser(char* pszNew, bool bEpprom); 
  static void SetHueBridgeUser(char* pszNew) { global::SetHueBridgeUser(pszNew, true); } 
  inline static char* GetHueBridgeClientkey() { return m_pHueHelper ? m_pHueHelper->GetBridgeClientkey() : (char*)""; } 
  static void SetHueBridgeClientkey(char* pszNew, bool bEpprom); 
  static void SetHueBridgeClientkey(char* pszNew) { global::SetHueBridgeClientkey(pszNew, true); } 
  static int GetHueLightState(char* pszHueID) { return global::GetHueLightState(pszHueID, true); }; 
  static int GetHueLightState(char* pszHueID, bool bCalcHudID); 
  static void SetHueLightState(char* pszHueID, int iNew, bool bEpprom); 
  static void SetHueLightState(char* pszHueID, int iNew) { global::SetHueLightState(pszHueID, iNew, true); } 
  static int GetHueLightZoneState(char* pszHueID, int iZone) { return global::GetHueLightZoneState(pszHueID, iZone, true); };  
  static int GetHueLightZoneState(char* pszHueID, int iZone, bool bCalcHudID); 
  static void SetHueLightZoneState(char* pszHueID, int iZone, int iNew, bool bEpprom); 
  static void SetHueLightZoneState(char* pszHueID, int iZone, int iNew) { global::SetHueLightZoneState(pszHueID, iZone, iNew, true); } 
  static int GetHueLightColCorRed(char* pszHueID) { return global::GetHueLightColCorRed(pszHueID, true); }; 
  static int GetHueLightColCorRed(char* pszHueID, bool bCalcHudID); 
  static void SetHueLightColCorRed(char* pszHueID, int iNew, bool bEpprom); 
  static void SetHueLightColCorRed(char* pszHueID, int iNew) { global::SetHueLightColCorRed(pszHueID, iNew, true); } 
  static int GetHueLightColCorGreen(char* pszHueID) { return global::GetHueLightColCorGreen(pszHueID, true); }; 
  static int GetHueLightColCorGreen(char* pszHueID, bool bCalcHudID); 
  static void SetHueLightColCorGreen(char* pszHueID, int iNew, bool bEpprom); 
  static void SetHueLightColCorGreen(char* pszHueID, int iNew) { global::SetHueLightColCorGreen(pszHueID, iNew, true); } 
  static int GetHueLightColCorBlue(char* pszHueID) { return global::GetHueLightColCorBlue(pszHueID, true); }; 
  static int GetHueLightColCorBlue(char* pszHueID, bool bCalcHudID); 
  static void SetHueLightColCorBlue(char* pszHueID, int iNew, bool bEpprom); 
  static void SetHueLightColCorBlue(char* pszHueID, int iNew) { global::SetHueLightColCorBlue(pszHueID, iNew, true); } 
  static int GetHueLightColBrightness(char* pszHueID) { return global::GetHueLightColBrightness(pszHueID, true); }; 
  static int GetHueLightColBrightness(char* pszHueID, bool bCalcHudID); 
  static void SetHueLightColBrightness(char* pszHueID, int iNew, bool bEpprom); 
  static void SetHueLightColBrightness(char* pszHueID, int iNew) { global::SetHueLightColBrightness(pszHueID, iNew, true); } 

  static void CreateHueIDWithNum(char* pszHueID, char* pszBuf, size_t stBufLen, int iNum); 
  static void CreateHueIDWithStr(char* pszHueID, char* pszBuf, size_t stBufLen, const char* pszExtra);

  static void SetHuePreset(int iIndex) { global::SetHuePreset(iIndex, true); }; 
  static void SetHuePreset(int iIndex, bool bEpprom);
  inline static int GetHuePreset(void) { return m_pHueHelper ? m_pHueHelper->GetPreset() : 3; }
#endif

#if defined(ENABLE_DREAMSCREEN)
  static void InitDreamScreen();
  static void StartServerDreamScreen();
  static void DiscoverDreamScreenDevices();
  inline static byte GetDreamScreenSupport(void) { return m_pDreamScreen ? m_pDreamScreen->GetSupport() : 0; }
  static void SetDreamScreenSupport(byte bNew, bool bEpprom, bool bInitDreamScreen); 
  static void SetDreamScreenSupport(byte bNew) { global::SetDreamScreenSupport(bNew, true, true); } 
  inline static byte GetDreamScreenEmulation(void) { return m_pDreamScreen ? m_pDreamScreen->GetEmulation() : 0; }
  static void SetDreamScreenEmulation(byte bNew, bool bEpprom); 
  static void SetDreamScreenEmulation(byte bNew) { global::SetDreamScreenEmulation(bNew, true); } 
  inline static byte GetDreamScreenGroup(void) { return m_pDreamScreen ? m_pDreamScreen->GetGroup() : 0; }
  static void SetDreamScreenGroup(byte bNew, bool bEpprom); 
  static void SetDreamScreenGroup(byte bNew) { global::SetDreamScreenGroup(bNew, true); } 

  inline static byte GetDreamScreenLowColorSmoothing() { return m_pDreamScreen ? m_pDreamScreen->GetLowColorSmoothing() : 0; }
  static void SetDreamScreenLowColorSmoothing(byte bNew, bool bEpprom);
  static void SetDreamScreenLowColorSmoothing(byte bNew) { global::SetDreamScreenLowColorSmoothing(bNew, true); }
#endif

#if defined(ENABLE_RELAY) 
  static byte GetAmbibackDeviceCount();
  static const char* GetAmbibackDeviceName(byte bIndex);
  static const char* GetAmbibackDeviceID(byte bIndex);
  static const char* GetAmbibackDeviceMac(byte bIndex);
  static const char* GetAmbibackDeviceIP(byte bIndex);
  static void SetAmbibackDeviceCount(byte bNew);
  static void SetAmbibackDeviceName(byte bIndex, char* psz);
  static void SetAmbibackDeviceID(byte bIndex, char* psz);
  static void SetAmbibackDeviceMac(byte bIndex, char* psz);
  static void SetAmbibackDeviceIP(byte bIndex, char* psz);
  inline static char* GetAmbibackSelectedDevice() { return &m_ambiback_selected_device[0]; }
  static void SetAmbibackSelectedDevice(char* pszNew, bool bEpprom);
  static void SetAmbibackSelectedDevice(char* pszNew) { global::SetAmbibackSelectedDevice(pszNew, true); }
  inline static bool GetAmbibackRelaymodeEnabled() { return m_ambiback_relaymode_enabled; }
  static void SetAmbibackRelaymodeEnabled(bool bNew);
  inline static bool GetAmbibackRelaymodeInitialized() { return m_ambiback_relaymode_initialized; }
  static void SetAmbibackRelaymodeInitialized(bool bNew) { m_ambiback_relaymode_initialized = bNew; };
  inline static byte GetAmbibackRelayModeDirect(void) { return m_ambiback_relaymode_direct; };
  static void SetAmbibackRelayModeDirect(byte bNew, bool bEpprom); 
  static void SetAmbibackRelayModeDirect(byte bNew) { global::SetAmbibackRelayModeDirect(bNew, true); } 
  inline static byte GetAmbibackRelayMode(void) { return m_ambiback_relay_mode; };
  static void SetAmbibackRelayMode(byte bNew, bool bEpprom); 
  static void SetAmbibackRelayMode(byte bNew) { global::SetAmbibackRelayMode(bNew, true); } 
  inline static byte GetAmbibackWifiDirectMode(void) { return m_ambiback_relay_wifidirect_enabled; };
  static void SetAmbibackWifiDirectMode(byte bNew, bool bEpprom);
  static void SetAmbibackWifiDirectMode(byte bNew) { global::SetAmbibackWifiDirectMode(bNew, true); }
  
  inline static IPAddress GetAmbibackRelayIPAddr() { return global::m_ambiback_relaymode_ipaddr; }
  inline static char* GetAmbibackRelayIPAddrChar() { return (char*) global::m_ambiback_relaymode_ipaddr.toString().c_str(); }
  static void SetAmbibackRelayIPAddr(char* pszNew, bool bEpprom);
  static void SetAmbibackRelayIPAddr(char* pszNew) { global::SetAmbibackRelayIPAddr(pszNew, true); }

  inline static char* GetAmbibackRelaySsid() { return &m_ambiback_relay_ssid[0]; }
  inline static char* GetAmbibackRelayPass() { return &m_ambiback_relay_password[0]; }
  static void SetAmbibackRelaySsid(char* pszNew, bool bEpprom);
  static void SetAmbibackRelaySsid(char* pszNew) { global::SetAmbibackRelaySsid(pszNew, true); }
  static void SetAmbibackRelayPass(char* pszNew, bool bEpprom);
  static void SetAmbibackRelayPass(char* pszNew) { global::SetAmbibackRelayPass(pszNew, true); }

  inline static int GetAmbibackRelayFailed() { return m_ambiback_relay_failed; } 
  inline static void IncrementAmbibackRelayFailed() { m_ambiback_relay_failed++; }
  inline static void ResetAmbibackRelayFailed() { m_ambiback_relay_failed = 0; }
  inline static bool GetAmbibackDiscoverySend(void) { return m_ambiback_discovery_send; };
  inline static void SetAmbibackDiscoverySend(bool bNew) { m_ambiback_discovery_send = bNew; }
  
  static bool CheckInitializeAmbibackRelaymode(char* pszRemoteIP); 
  static bool InitializeAmbibackRelaymode(void); 
  static bool ConnectWifiDirect();
#endif

  static byte GetButtonFunction() {return m_bButtonFunction; }
  static void SetButtonFunction(byte bNew, bool bEpprom);
  static void SetButtonFunction(byte bNew) { global::SetButtonFunction(bNew, true); }
  static void HandleButtonFunction(void);

  // WIFI Handling Functions
  static byte GetWifiFailHandling() {return m_bWifiFailHandling; }
  static void SetWifiFailHandling(byte bNew, bool bEpprom);
  static void SetWifiFailHandling(byte bNew) { global::SetWifiFailHandling(bNew, true); }
  static void HandleWifiWPS(void* pVoidEsp32wps_config);
  static void HandleWifiFail(void);
  static void HandleWifiAPSetup(void);
  static void StartWifiAP(void);
  static void StopWifiAP(void);
# if defined(ENABLE_WIFI_AP)
  static void StartWifiProvisioning(void);
  static void StopWifiProvisioning(void);
  static void HandleWifiProvisioningWindow(void);
  static bool IsWifiProvisioningActive(void) { return m_bWifiProvisioningActive; }
  static bool IsWifiProvisioningPending(void) { return m_bWifiProvisioningPending; }
  static bool GetWifiProvisioningEnabled(void) { return m_bWifiProvisioningEnabled; }
  static byte GetWifiProvisioningTimeoutMinutes(void) { return m_bWifiProvisioningTimeoutMinutes; }
  static unsigned long GetWifiProvisioningStarted(void) { return m_lWifiProvisioningStarted; }
  static unsigned long GetWifiProvisioningDeadline(void) { return m_lWifiProvisioningDeadline; }
  static unsigned long GetWifiProvisioningTimeoutMs(void);
  static unsigned long GetWifiProvisioningExpiresIn(void);
  static char* GetWifiProvisioningToken(void) { return &m_wifi_provisioning_token[0]; }
  static char* GetWifiProvisioningSSID(void) { return &m_wifi_provisioning_ssid[0]; }
  static char* GetWifiProvisioningPassword(void) { return &m_wifi_provisioning_password[0]; }
  static bool ValidateWifiProvisioningToken(char* pszToken);
  static void SetWifiProvisioningPending(bool bNew) { m_bWifiProvisioningPending = bNew; }
  static void SetWifiProvisioningActive(bool bNew) { m_bWifiProvisioningActive = bNew; }
  static void SetWifiProvisioningStarted(unsigned long lNew) { m_lWifiProvisioningStarted = lNew; }
  static void SetWifiProvisioningToken(char* pszNew);
  static void SetWifiProvisioningCredentials(char* pszSSID, char* pszPassword);
  static void SetWifiProvisioningEnabled(byte bNew, bool bEpprom);
  static void SetWifiProvisioningEnabled(byte bNew) { global::SetWifiProvisioningEnabled(bNew, true); }
  static void SetWifiProvisioningTimeoutMinutes(byte bNew, bool bEpprom);
  static void SetWifiProvisioningTimeoutMinutes(byte bNew) { global::SetWifiProvisioningTimeoutMinutes(bNew, true); }
  static void BuildGeneratedApPasswordFromMac(char* pszMac, char* pszOut, size_t stOutLen);
  static void ClearWifiProvisioning(void);
# endif
  
# if defined(ENABLE_WIFI_NAT)
  static void HandleWifiAPNAT(void);
  static void AddPortMappingWifiAPNAT(u16_t sourcePort, u32_t destAddr, u16_t destPort);
  static void RemovePortMappingWifiAPNAT(u16_t sourcePort);
# endif
  static void HandleWifiMode(void);
  static void HandleWifiConnectSetup(void);
  static void HandleWifiCheck(void);
  static void HandleStartServers(void);
  static void WIFIScanChannels(void);
  static bool WIFIConnect(char* pszNewSSID, char* pszNewKEY);
  static void WIFIDisconnect(char* pszCurrentSSID);
  static bool ConnectWifiConfigured(char* szSSID, char* szKEY, bool bAsync);

  static bool GetButtonPressed() {return m_bButtonPressed; }
  static void SetButtonPressed(bool bNew) { m_bButtonPressed = bNew;}
  static unsigned long GetButtonPressedStart() {return m_lButtonPressedStart; }
  static void SetButtonPressedStart(unsigned long lNew) { m_lButtonPressedStart = lNew;}

  /* Debug functions*/
  static void dbgprintf_P(PGM_P pMsg, ...); 
  static void dbgprintf(const char* pMsg, ...);
  static int dbgprintf_esp(const char *fmt, va_list args);
  static void heap_caps_alloc_failed_hook(size_t requested_size, uint32_t caps, const char *function_name);

  /* Tool Helpers */
  static void printEspPartitions(); 
  static int getFreeSketchSpace(); 
  static void esp32Stophandler(void);
  static void printEspMemUsage(const char* pszSource, const char* pszFunc);

#ifdef ENABLE_SSD1306
  static void oledSetObj(void* pOled); 
  static void uiSetObj(void* pUI);
  static void uiInit();
  static void uiReset();
  static void oledDrawStringCenter(String str); 
  static void oledDrawStringCenter(String str ,String str2);
  static void oledDrawStringCenter(String str ,String str2, String str3);
  static void oledDrawStringLeft(String str); 
  static void oledDrawStringRight(String str); 
  static void oledDrawStringQueue(String str1, String str2, String str3); 
  static bool oledProcessQueue(); 
#endif

  static void errprintf(PGM_P pMsg, ...); 
  static void errprintf_P(const char* pMsg, ...);
  static void printfserial(const char *fmt, va_list args);
  static void flushserial(); 
  static char* GetWifiStatusTxt(int iStatus);
  static char* GetWifiAuthTxt(int iStatus);

  /* Checksum helpers */
  static uint8_t crc_8( byte* input_bytes, size_t num_bytes ); 
  static inline uint8_t update_crc_8( unsigned char crc, unsigned char val ); 

  /* Power/Ctl LED functions*/
  static void TogglePwrLed(); 
  static void TogglePwrLed(byte bRed, byte bGreen, byte bBlue); 
  static void ToggleCtlLed(); 
  static void ToggleCtlLed(byte bRed, byte bGreen, byte bBlue); 
  static void TogglePwrLedOn();
  static void TogglePwrLedOn(bool bDirect);
  static void TogglePwrLedOn(byte bRed, byte bGreen, byte bBlue, bool bState);
  static void TogglePwrLedOn(byte bRed, byte bGreen, byte bBlue, bool bState, bool bDirect); 
  static void ToggleCtlLedOn(); 
  static void ToggleCtlLedOn(byte bRed, byte bGreen, byte bBlue, bool bState); 
  static void TogglePwrLedOff(); 
  static void TogglePwrLedOff(bool bDirect); 
  static void TogglePwrLedMsg(); 
  static void ToggleCtlLedOff();   
  static void RebootController();
  static void SetRemoteLEDMode(char *pszRemote, byte bLedMode);
  
  /* Get/Set Functions **/
  inline static char* GetWifiSSID() { return &m_wifi_ssid[0]; }
  inline static char* GetWifiPassword() { return &m_wifi_password[0]; }
  inline static char* GetDeviceName() { return &m_devicename[0]; }
  inline static char* GetDeviceID() { return &m_deviceid[0]; }
  inline static char* GetDeviceIDMac() { return &m_deviceidmac[0]; }
  inline static char* GetDeviceAPssid() { return &m_wifi_ap_ssid[0]; }
  inline static char* GetDeviceAPpass() { return &m_wifi_ap_password[0]; }
  inline static bool GetWifiConnectCheck() {return m_bWifiConnectCheck; }
  inline static bool GetWifiTryWPS() {return m_bWifiTryWPS; }
  inline static bool GetWifiConnected() {return m_bWifiConnected; };
  inline static bool GetWifiDirectConnected() {return m_bWifiDirectConnected; };
  inline static bool GetForceWifiTryWPS() {return m_bForceWifiTryWPS; };
  inline static bool GetWifiTryConnect() {return m_bWifiTryConnect; };
  inline static bool GetWifiTryReConnect() {return m_bWifiTryReConnect; };
  #if defined(ENABLE_WIFI_AP)
  inline static bool GetWifiAPEnable() {return m_wifi_ap_enabled; }
  #else
  inline static bool GetWifiAPEnable() {return false; }
  #endif  
  static void SetWifiSSID(char* pszNew, bool bEpprom);
  static void SetWifiSSID(char* pszNew) { global::SetWifiSSID(pszNew, true); }
  static void SetWifiPassword(char* pszNew, bool bEpprom);
  static void SetWifiPassword(char* pszNew) { global::SetWifiPassword(pszNew, true); }
  static void SetDeviceName(char* pszNew, bool bEpprom);
  static void SetDeviceName(char* pszNew) { global::SetDeviceName(pszNew, true); }
  static void SetDeviceAPssid(char* pszNew, bool bEpprom);
  static void SetDeviceAPssid(char* pszNew) { global::SetDeviceAPssid(pszNew, true); }
  static void SetDeviceAPpass(char* pszNew, bool bEpprom);
  static void SetDeviceAPpass(char* pszNew) { global::SetDeviceAPpass(pszNew, true); }
  static void SetWifiAPEnable(byte bNew, bool bEpprom);
  static void SetWifiAPEnable(byte bNew) { global::SetWifiAPEnable(bNew, true); }
  static bool GetRestoreLastLedMode(void) { return m_bRestoreLastLedMode; }
  static void SetRestoreLastLedMode(byte bNew, bool bEpprom);
  static void SetRestoreLastLedMode(byte bNew) { global::SetRestoreLastLedMode(bNew, true); }
  
  static void SetWifiConnectCheck(bool bNew) { m_bWifiConnectCheck = bNew; }
  static void SetWifiTryWPS(bool bNew) { m_bWifiTryWPS = bNew; }
  static void SetWifiConnected(bool bNew) { m_bWifiConnected = bNew; }
  static void SetWifiDirectConnected(bool bNew) { m_bWifiDirectConnected = bNew; }
  static void SetForceWifiTryWPS(bool bNew) { m_bForceWifiTryWPS = bNew; }
  static void SetWifiTryConnect(bool bNew) { m_bWifiTryConnect = bNew; }
  static void SetWifiTryReConnect(bool bNew) { m_bWifiTryReConnect = bNew; }

  static void SetDebugSyslogServer(char* pszNew); 
  static void SetDebugSyslogPort(int iNew) { m_iDebugsyslogport = iNew; }
  static char* GetDebugSyslogServer() {return &m_DebugSyslogServer[0];} 
  static char* GetDebugSyslogAppName() {return &m_DebugSyslogAppName[0];} 
  static int GetDebugSyslogPort() {return m_iDebugsyslogport; }

  static void SetSerialEnabled(bool bNew) {m_bSerialEnabled = bNew; }
         static void SetSyslogEnabled(bool bNew);
  static void SetUltraDebugEnabled(bool bNew) {m_bUltraDebugEnabled = bNew; }
  static bool GetSerialEnabled() {return m_bSerialEnabled; }
  static bool GetSyslogEnabled() {return m_bSyslogEnabled; }
  static bool GetUltraDebugEnabled() {return m_bUltraDebugEnabled; }
  static bool GetDebugEnabled() {return (m_bSerialEnabled | m_bSyslogEnabled); }

  inline static bool GetSettingsChanged() { return m_bSettingsChanged; } 
  static void SetSettingsChanged(bool bNew) { m_bSettingsChanged = bNew; }
  inline static bool GetNetworkStarted() { return m_bNetworkStarted; } 
  static void SetNetworkStarted(bool bNew) { m_bNetworkStarted = bNew; }
  inline static bool GetShouldReboot() { return m_shouldReboot; } 
  static void SetShouldReboot(bool bNew) { m_shouldReboot = bNew; }
  inline static unsigned long GetRebootAt() { return m_lRebootAt; }
  static void ScheduleReboot(unsigned long lDelayMs);
  static void ClearScheduledReboot(void) { m_shouldReboot = false; m_lRebootAt = 0; }
  inline static bool GetShouldStartWps() { return m_shouldStartWps; } 
  static void SetShouldStartWps(bool bNew) { m_shouldStartWps = bNew; }
  inline static bool GetShouldStartFirmwareUpdate() { return m_shouldStartFirmwareUpdate; }
  static void SetShouldStartFirmwareUpdate(bool bNew) { m_shouldStartFirmwareUpdate = bNew; }
  inline static bool GetFirmwareRunning() { return m_firmwareRunning; } 
  static void SetFirmwareRunning(bool bNew) { m_firmwareRunning = bNew; }

  inline static int GetDoAction() { return m_doAction; } 
  static void SetDoAction(int iNew) { m_doAction = iNew; }

  /* Preset Functions */
  static void SetColorPreset(int iIndex) { global::SetColorPreset(iIndex, true); }; 
  static void SetColorPreset(int iIndex, bool bEpprom);
  inline static int GetColorPreset(void) { return m_iColorPreset; };

  static void SetBrightnessPreset(int iIndex) { global::SetBrightnessPreset(iIndex, true); }; 
  static void SetBrightnessPreset(int iIndex, bool bEpprom);
  inline static int GetBrightnessPreset(void) { return m_iBrightnessPreset; };

  static void SetMotionPreset(int iIndex) { global::SetMotionPreset(iIndex, true); }; 
  static void SetMotionPreset(int iIndex, bool bEpprom);
  inline static int GetMotionPreset(void) { return m_iMotionPreset; };

#if defined(ENABLE_CAM)
  static void SetCamPreset(int iIndex) { global::SetCamPreset(iIndex, NULL, true, false); }; 
  static void SetCamPreset(int iIndex, bool bEpprom) { global::SetCamPreset(iIndex, NULL, bEpprom, false); }; 
  static void SetCamPreset(int iIndex, void* voidresponse) { global::SetCamPreset(iIndex, voidresponse, true, false); }; 
  static void SetCamPreset(int iIndex, void* voidresponse, bool bEpprom, bool bReadOnly);
  inline static int GetCamPreset(void) { return m_iCamPreset; };

  static void SetExposurePreset(int iIndex) { global::SetExposurePreset(iIndex, NULL, true, false); }; 
  static void SetExposurePreset(int iIndex, bool bEpprom) { global::SetExposurePreset(iIndex, NULL, bEpprom, false); }; 
  static void SetExposurePreset(int iIndex, void* voidresponse) { global::SetExposurePreset(iIndex, voidresponse, true, false); }; 
  static void SetExposurePreset(int iIndex, void* voidresponse, bool bEpprom, bool bReadOnly);
  inline static int GetExposurePreset(void) { return m_iExposurePreset; };

  static void SetCamInnerPreset(int iIndex) { global::SetCamInnerPreset(iIndex, NULL, true, false); }; 
  static void SetCamInnerPreset(int iIndex, bool bEpprom) { global::SetCamInnerPreset(iIndex, NULL, bEpprom, false); }; 
  static void SetCamInnerPreset(int iIndex, void* voidresponse) { global::SetCamInnerPreset(iIndex, voidresponse, true, false); }; 
  static void SetCamInnerPreset(int iIndex, void* voidresponse, bool bEpprom, bool bReadOnly);
  inline static int GetCamInnerPreset(void) { return m_iCamInnerPreset; };  
#endif

  /* Ambient Mode preset (0=Solid, 1-11 scenes, 12=40Hz Gamma, 13=10Hz Alpha, 14=34Hz Gamma Red, 15=Rotlicht Abend) */
  static void SetAmbientPreset(int iIndex) { global::SetAmbientPreset(iIndex, true); };
  static void SetAmbientPreset(int iIndex, bool bEpprom);
  inline static int GetAmbientPreset(void) { return m_iAmbientPreset; };
  static void SetAmbientSpeed(int iNew) { global::SetAmbientSpeed(iNew, true); };
  static void SetAmbientSpeed(int iNew, bool bEpprom);
  inline static int GetAmbientSpeed(void) { return m_iAmbientSpeed; };
  static void SetAmbientIntensity(int iNew) { global::SetAmbientIntensity(iNew, true); };
  static void SetAmbientIntensity(int iNew, bool bEpprom);
  inline static int GetAmbientIntensity(void) { return m_iAmbientIntensity; };
  static void SetAmbientHealthIntensity(int iNew) { global::SetAmbientHealthIntensity(iNew, true); };
  static void SetAmbientHealthIntensity(int iNew, bool bEpprom);
  inline static int GetAmbientHealthIntensity(void) { return m_iAmbientHealthIntensity; };

  /* Network Receiver helpers */
  inline static bool GetClientConnected() { return m_bClientConnected; } 
  static void SetClientConnected(bool bNew);
  inline static bool GetClientConnectedIsNat() { return m_bClientConnectedIsNat; } 
  inline static void SetClientConnectedIsNat(bool bNew) { m_bClientConnectedIsNat = bNew; }
  inline static char* GetClientConnectedIpStr() { return m_szCurClientIP; }
  inline static u32_t GetClientConnectedAddr() { return m_CurClientAddr; }
  static void SetClientConnectedIpStr(IPAddress* pIPClient);

  /* Serial Helpers */
  inline static bool GetSerialConnected() { return m_bSerialConnected; } 
  static void SetSerialConnected(bool bNew) { m_bSerialConnected= bNew; } 

  /* Frame Counter helper functions */ 
  inline static byte GetCurrentPkts() { return m_bCurrentPkts; }
  inline static void SetCurrentPkts(byte bNew) { m_bCurrentPkts = bNew; }
  inline static byte GetCurrentPktsDropped() { return m_bCurrentPktsDropped; }
  inline static void SetCurrentPktsDropped(byte bNew) { m_bCurrentPktsDropped = bNew; }
  inline static byte GetCurrentFramesCtr() { return m_bCurrentFramesCtr; }
  inline static void SetCurrentFramesCtr(byte bNew) { m_bCurrentFramesCtr = bNew; }
  inline static byte GetCurrentDroppedCtr() { return m_bCurrentDroppedCtr; }
  inline static void SetCurrentDroppedCtr(byte bNew) { m_bCurrentDroppedCtr = bNew; }
  inline static void IncrementCurrentFramesCtr() { m_bCurrentFramesCtr++; }
  inline static void IncrementCurrentDroppedCtr() { m_bCurrentDroppedCtr++; }
  static void CalcFramesCtr(void); 

  /* Other helper functions */ 
  static size_t GetDeviceInfo(char* pszIn, size_t stLen);
  static void CalcDynamicDelayLength(unsigned long lLastUpdate);
  static void OTAUpdateStarted();
  static void OTAUpdateFinished();
  static void OTAUpdateProgress(int cur, int total);
  static void OTAUpdateError(int err);
  static void StartHttpUpdate();
  static void DisableSysCoreWatchDog();
  static void EnableSysCoreWatchDog();
  static void DisableArduinoCoreWatchDog();
  static void EnableArduinoCoreWatchDog();
  
  /* UDP Syslog helpers */
  static void DeInitSyslogClient(); 
  static void InitSyslogClient(); 
  static Syslog* m_pSyslog;

  /* Global Helper Objects */
  static void SetLedHelper(LEDHelper* pNew);
  static void SetEepromHelper(EEPROMHelper* pNew);
  static void SetWebServerHelper(AmbiWebServer* pNew);
  static LEDHelper* m_pLedHelper; 
  static EEPROMHelper* m_pEepromHelper; 
  static AmbiWebServer* m_pWebServer;

#if defined(ENABLE_CAM)
  static ::CamProcessor* m_pCamProcessor;
  static bool m_bCameraPrepareOK;
  static bool m_bCameraOK;
  static int m_iCamPerformance;
  static int m_iCamIndex;
  static int m_iCamFreq;
  static int m_iCamFrameBuffer;
  static int m_iCamFlicker;
# if CAM_SENSOR == CAM_SENSOR_OV3660
  static byte m_bCamOverclock;
# endif
  // Color related 
  static byte m_bCamColorCorRed;
  static byte m_bCamColorCorGreen;
  static byte m_bCamColorCorBlue;
  static byte m_bCamActivationThreshold;
  static byte m_bCamBlackRemoveThreshold;
  static byte m_bCamMinPixelAmount;
  // Image related  
  static int m_iCamContrast;
  static int m_iCamBrightness;
  static int m_iCamSaturation;
  static int m_iCamSpecialEffect;
  static int m_iCamWbMode;
  static int m_iCamQuality;
  static int m_iCamAwbGainControl; 
  static int m_iCamVFlip; 
  static int m_iCamHMirror;
  static int m_bCamHighQuality;
  static int m_iCamBPC;
  static int m_iCamWPC;
  static int m_iCamDenoise;
  static int m_iCamLenc;
  static int m_iCamRawGMA;
  static int m_iCamDcw;
  static int m_iCamAutoWhitebalance;
  static int m_iCamGaincontrol;
  static int m_iCamAgcGain;
  // Auto Exposure related
  static int m_iCamAutoExposurecontrol;
  static int m_iCamAec2; // Automatic Exposure Control
  static int m_iCamAeLevels;  
  static int m_iCamAecValue;
  // Adaptiv Exposure related
  static int m_iCamAdaptiveExposurecontrol;
  static int m_iCamAecMaxValue;
  static int m_iCamAecMinValue;
  static int m_iCamAdaptiveChangePercent;
  static int m_iCamAdaptiveChangeDelay;
  static int m_iCamAdaptiveGaincontrol;
  static byte m_bCamWhiteActivationTreshold;
  static byte m_bCamBeforeWhiteActivationTreshold;
  static byte m_bCamWhiteMinimumAmount;
  static byte m_bCamAdaptiveGainDown;
  static byte m_bCamAdaptiveGainUp;
  static byte m_bCamAdaptiveGainMax;

  // Camera Handling 
  static byte m_bCamAutoOnOff;
  static int m_iCamAutoOnOffDelay;

  // Cam geometry is owned and exposed by CamProcessor.
  static byte m_bCamJpegCalcDirect;
   
#endif
#if defined(ENABLE_HUE)
  // Public Helper obj
  static HueHelper* m_pHueHelper; 
#endif

#if defined(ENABLE_DREAMSCREEN)
  // Public Helper obj
  static DreamScreen* m_pDreamScreen;
#endif

  // Public Static members
  static bool m_shouldReboot;
  static unsigned long m_lRebootAt;
  static bool m_shouldStartWps;
  static bool m_firmwareRunning;          // Firmware Update Running Helper
  static bool m_shouldStartFirmwareUpdate;// Trigger Firmware Update Helper
  static bool m_bUltraDebugEnabled; 
  static int m_doAction;

  // Public Static time helpers
  static unsigned long g_timeNow; 
  static unsigned long g_timeButtonPressed;
  static unsigned long g_timeWIFICheck;
  static unsigned long g_timeDebugCheck;
  static unsigned long g_timeFreeMemCheck;
  static unsigned long g_timeWIFITryConnect;
  static unsigned long g_timeWIFITryConnectCount;
#if defined(ENABLE_SSD1306)
  static unsigned long g_timeSS1306Wait;
#endif
  static unsigned long g_timeNowLastNetworkErr; 
  static int g_httpUpdateCheck;

private:
  // Private Static members
  static char m_wifi_ssid[WLAN_SSID_MAX];
  static char m_wifi_password[WLAN_KEY_MAX];
  static char m_devicename[DEVICE_NAME_MAX];
  static char m_deviceid[DEVICE_ID_MAX];
  static char m_deviceidmac[DEVICE_MAC_MAX];
  static bool m_wifi_ap_enabled;
  static char m_wifi_ap_ssid[WLAN_SSID_MAX];
  static char m_wifi_ap_password[WLAN_KEY_MAX];
  static bool m_bWifiConnectCheck;
  static bool m_bWifiTryWPS;
  static bool m_bWifiConnected;
  static bool m_bWifiDirectConnected;
  static bool m_bWifiTryConnect;
  static bool m_bWifiTryReConnect;
  static bool m_bForceWifiTryWPS;
  static bool m_bRestoreLastLedMode;
# if defined(ENABLE_WIFI_AP)
  static bool m_bWifiProvisioningEnabled;
  static byte m_bWifiProvisioningTimeoutMinutes;
  static bool m_bWifiProvisioningActive;
  static bool m_bWifiProvisioningPending;
  static unsigned long m_lWifiProvisioningStarted;
  static unsigned long m_lWifiProvisioningDeadline;
  static char m_wifi_provisioning_token[WIFI_PROVISIONING_TOKEN_MAX];
  static char m_wifi_provisioning_ssid[WLAN_SSID_MAX];
  static char m_wifi_provisioning_password[WLAN_KEY_MAX];
# endif

  static bool m_bSerialEnabled; 
  static bool m_bSyslogEnabled; 
  
  static char m_DebugSyslogServer[STR_IPNAME_MAX]; 
  static char m_DebugSyslogAppName[STR_IPNAME_MAX]; 
  static int m_iDebugsyslogport; 
  
  static byte m_bButtonFunction;
  static bool m_bButtonPressed;
  static unsigned long m_lButtonPressedStart;
  static byte m_bWifiFailHandling;

  static int m_iColorPreset;
  static int m_iBrightnessPreset;
  static int m_iMotionPreset;
  static int m_iAmbientPreset;
  static int m_iAmbientSpeed;
  static int m_iAmbientIntensity;
  static int m_iAmbientHealthIntensity;
#if defined(ENABLE_CAM)
  static int m_iCamPreset;
  static int m_iExposurePreset;
  static int m_iCamInnerPreset;
#endif
  static bool m_bSettingsChanged; 
  static bool m_bNetworkStarted; 
  static bool m_bClientConnected;
  static bool m_bClientConnectedIsNat;
  static bool m_bSerialConnected;
  static char m_szCurClientIP[STR_IP_MAX];
  static u32_t m_CurClientAddr;

  // Frame Package Counter variables
  static byte m_bCurrentPkts;            // Received Pkts/s
  static byte m_bCurrentPktsDropped;     // Dropped Pkts/s
  static byte m_bCurrentFramesCtr;       // Counter for received packets
  static byte m_bCurrentDroppedCtr;      // Counter for dropped packets

#if defined(ENABLE_RELAY) 
  // Static variables for RELAY
  static char m_ambiback_devices_name[AMBIBACK_MAX_DEVICES][DEVICE_NAME_MAX];
  static char m_ambiback_devices_id[AMBIBACK_MAX_DEVICES][AMBIBACK_MAX_IDLEN];
  static char m_ambiback_devices_mac[AMBIBACK_MAX_DEVICES][AMBIBACK_MAX_IDLEN];
  static char m_ambiback_devices_ip[AMBIBACK_MAX_DEVICES][AMBIBACK_MAX_IDLEN];
  static byte m_ambiback_devices_count;
  static char m_ambiback_selected_device[AMBIBACK_MAX_IDLEN];
  static bool m_ambiback_relaymode_enabled;
  static bool m_ambiback_relaymode_initialized;
  static byte m_ambiback_relaymode_direct;
  static byte m_ambiback_relay_mode;
  static byte m_ambiback_relay_wifidirect_enabled;
  static bool m_ambiback_discovery_send;

  static int m_ambiback_relay_failed;
  static IPAddress m_ambiback_relaymode_ipaddr;

  static char m_ambiback_relay_ssid[WLAN_SSID_MAX];
  static char m_ambiback_relay_password[WLAN_KEY_MAX];

#endif

};

#endif 
