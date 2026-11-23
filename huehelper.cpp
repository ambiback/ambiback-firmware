// Include all needed files
#include "globalincludes.h"

#ifdef ENABLE_HUE
# include "huehelper.h"
# include "FastLED.h"    // For CRGB

HueHelper* pHueHelper = NULL; 
# include <cJSON.h>

/* 
// --- HttpRequestMethod
typedef enum {
  HTTP_GET     = 0b00000001,
  HTTP_POST    = 0b00000010,
  HTTP_DELETE  = 0b00000100,
  HTTP_PUT     = 0b00001000,
} HttpRequestMethod;
// ---
*/
# if defined(ESP32)
# include <HTTPClient.h>
# include <WiFiClient.h>
# include <WiFiClientSecureUdp.h>

// HOTFIX USE STATIC HTTP CLIENT
HTTPClient mHueHttp;

// Task Handles 
TaskHandle_t handleTaskHueLoop;
// Task Functions
void TaskHueLoop(void * pvParameters);
# endif

// --- RGB to HSV converter helpers
static RgbColor HsvToRgb(HsvColor hsv);
static HsvColor RgbToHsv(RgbColor rgb);
// ---

// --- Static helper objects
static CRGB m_leds_hue[MAX_HUE_LEDS];
static char m_hue_devices[MAX_HUE_DEVICES][MAX_HUE_MAXID];
static int m_iHueEntertainmentGroupID; 
// static char m_szHueEntertainmentGroupID[MAX_HUE_MAXNAME];
static int m_hue_device_ids[MAX_HUE_DEVICES];
static byte m_hue_device_state[MAX_HUE_DEVICES];
// ---Variable cache helpers
static byte m_hue_device_var_enabled[MAX_HUE_DEVICES];
static byte m_hue_device_var_max_brightness[MAX_HUE_DEVICES];
static int m_hue_device_var_color_red[MAX_HUE_DEVICES];
static int m_hue_device_var_color_green[MAX_HUE_DEVICES];
static int m_hue_device_var_color_blue[MAX_HUE_DEVICES];
static byte m_hue_device_var_light_zones[MAX_HUE_DEVICES][MAX_HUE_LEDS];
// ---
static int m_hue_devices_count = 0;
static HTTPClient* m_pHttp[MAX_HUE_HTTPHELPERS]; 
static WiFiClientSecureUdp* m_pSecureClient;

// Hardcoded for now!
static int m_nHueCT = 153; // 153 (6500K) to 500 (2000K)
static int m_nDTLSPort = 2100;
static int m_nFrameLength = 150;
// --- 

// Hue Processing Timer helpers
unsigned long m_nHueTimeStart = 0;
unsigned long m_nHueTimeFPSCheck = 0;
unsigned long m_nHueTimeDebug1 = 0;
unsigned long m_nHueTimeDebug2 = 0;
unsigned long m_nHueTimeFinish = 0; 

// Start class functions 
HueHelper::HueHelper(){
  m_bHueConnected = false;
  this->SetTaskMode(HUE_TASK_MODE_INIT); 
  this->SetTaskRunning(false);
  pHueHelper = this;
  // Hardcoded for testing now!
  this->SetHueUseDtls( true); 

  // Set status variables  
  this->SetHueDtlsPrepared(false); 
  this->SetHueDtlsConnected(false); 
  m_bCurrentFramesCtr = 0; 
  m_bCurrentFps = 0; 
  m_bCurrentFpsSkip = 0;       
  m_bCurrentFramesSkipCtr = 0; 
  m_bNewLedData = false;
  m_bUpdateHueConfig = false;

  // Bridge/config state (moved from global)
  m_bHueSupport = 0;
  m_nFrameLengthDtls = 40;
  m_nHandshakeTimeout = 10;
  m_huebridgeip[0] = '\0';
  m_huebridgeuser[0] = '\0';
  m_huebridgeclientkey[0] = '\0';
  m_iHuePreset = 3;
 
  memset(m_leds_hue, 0, MAX_HUE_LEDS * sizeof(struct CRGB));
  memset(m_hue_device_ids, 0, MAX_HUE_DEVICES * sizeof(int));
  memset(m_hue_device_state, 0, MAX_HUE_DEVICES * sizeof(byte));
  memset(m_hue_device_var_enabled, 0, MAX_HUE_DEVICES * sizeof(byte));
  memset(m_hue_device_var_max_brightness, 0, MAX_HUE_DEVICES * sizeof(byte));
  memset(m_hue_device_var_color_red, 0, MAX_HUE_DEVICES * sizeof(int));
  memset(m_hue_device_var_color_green, 0, MAX_HUE_DEVICES * sizeof(int));
  memset(m_hue_device_var_color_blue, 0, MAX_HUE_DEVICES * sizeof(int));
  memset(m_hue_device_var_light_zones, 0, MAX_HUE_DEVICES * MAX_HUE_LEDS * sizeof(byte));

  m_iHueEntertainmentGroupID = HUE_NOT_INITIALIZED; 
  // memset(m_szHueEntertainmentGroupID, 0, MAX_HUE_MAXNAME);

  // Init Pointers for HTTP Clients
  for(int i=0; i < MAX_HUE_HTTPHELPERS; i++)
    m_pHttp[i] = NULL; 
  // Init DTLS Helper
  m_pSecureClient = NULL; 

  handleTaskHueLoop = NULL; 
}

void HueHelper::SetSupport(byte bNew, bool bEpprom, bool bInitHue) {
  m_bHueSupport = bNew;
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingByte("HueSupport", m_bHueSupport);
  if (m_bHueSupport == 1) {
    _DBGPRINTF(PSTR("HueHelper:SetSupport:Enabled\n"));
    if (bInitHue) {
      _DBGPRINTF(PSTR("HueHelper:SetSupport:InitHueLights\n"));
      InitHueLights();
    }
  } else {
    _DBGPRINTF(PSTR("HueHelper:SetSupport:Disabled\n"));
  }
}

void HueHelper::SetFrameLengthDtls(byte bNew, bool bEpprom) {
  m_nFrameLengthDtls = bNew;
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingByte("HueFrmLen", bNew);
}

void HueHelper::SetHandshakeTimeout(byte bNew, bool bEpprom) {
  m_nHandshakeTimeout = bNew;
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingByte("HueHndTout", bNew);
}

void HueHelper::SetBridgeIP(char* pszNew, bool bEpprom) {
  size_t strLen = strlen(pszNew);
  if (strLen > (HUE_STR_IP_MAX - 1)) strLen = HUE_STR_IP_MAX - 1;
  memcpy(m_huebridgeip, pszNew, strLen);
  m_huebridgeip[strLen] = '\0';
  _DBGPRINTF(PSTR("HueHelper:SetBridgeIP=%s\n"), m_huebridgeip);
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingString("HueBridgeIP", m_huebridgeip, HUE_STR_IP_MAX);
}

void HueHelper::SetBridgeUser(char* pszNew, bool bEpprom) {
  size_t strLen = strlen(pszNew);
  if (strLen > (HUE_BRIDGE_USER_MAX - 1)) strLen = HUE_BRIDGE_USER_MAX - 1;
  memcpy(m_huebridgeuser, pszNew, strLen);
  m_huebridgeuser[strLen] = '\0';
  _DBGPRINTF(PSTR("HueHelper:SetBridgeUser=%s\n"), m_huebridgeuser);
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingString("HueBridgeUser", m_huebridgeuser, HUE_BRIDGE_USER_MAX);
}

void HueHelper::SetBridgeClientkey(char* pszNew, bool bEpprom) {
  size_t strLen = strlen(pszNew);
  if (strLen > (HUE_BRIDGE_USER_MAX - 1)) strLen = HUE_BRIDGE_USER_MAX - 1;
  memcpy(m_huebridgeclientkey, pszNew, strLen);
  m_huebridgeclientkey[strLen] = '\0';
  _DBGPRINTF(PSTR("HueHelper:SetBridgeClientkey set\n"));
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingString("HueBridgeKey", m_huebridgeclientkey, HUE_BRIDGE_USER_MAX);
}

void HueHelper::SetPreset(int iIndex, bool bEpprom) {
  m_iHuePreset = iIndex;
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingInt("HuePreset", iIndex);
  if (iIndex == 1) {
    SetFrameLengthDtls(20, false);
    SetHandshakeTimeout(10, false);
  } else if (iIndex == 2) {
    SetFrameLengthDtls(28, false);
    SetHandshakeTimeout(10, false);
  } else if (iIndex == 3) {
    SetFrameLengthDtls(40, false);
    SetHandshakeTimeout(10, false);
  } else {
    if (global::m_pEepromHelper) {
      SetFrameLengthDtls(global::m_pEepromHelper->GetSettingByte("HueFrmLen", 28), false);
      SetHandshakeTimeout(global::m_pEepromHelper->GetSettingByte("HueHndTout", 10), false);
    }
  }
  _DBGPRINTF(PSTR("HueHelper:SetPreset to %d\n"), m_iHuePreset);
}

int HueHelper::GetLightState(char* pszHueID, bool bCalcHudID) const {
  char szSettingID[ESP32_PREF_KEYMAX];
  char* pszSettingID = pszHueID;
  if (bCalcHudID) {
    HueHelper::CreateHueIDWithStr(const_cast<char*>(pszHueID), szSettingID, ESP32_PREF_KEYMAX, "_st");
    pszSettingID = &szSettingID[0];
  }
  if (global::m_pEepromHelper == NULL) return 0;
  return global::m_pEepromHelper->GetSettingInt(pszSettingID, 0, false);
}

void HueHelper::SetLightState(char* pszHueID, int iNew, bool bEpprom) {
  char szSettingID[ESP32_PREF_KEYMAX];
  CreateHueIDWithStr(pszHueID, szSettingID, ESP32_PREF_KEYMAX, "_st");
  _DBGPRINTF(PSTR("HueHelper:SetLightState SettingID=%s Value=%d\n"), szSettingID, iNew);
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingInt(szSettingID, iNew);
}

int HueHelper::GetLightZoneState(char* pszHueID, int iZone, bool bCalcHudID) const {
  char szSettingID[ESP32_PREF_KEYMAX];
  char* pszSettingID = pszHueID;
  if (bCalcHudID) {
    CreateHueIDWithNum(const_cast<char*>(pszHueID), szSettingID, ESP32_PREF_KEYMAX, iZone);
    pszSettingID = &szSettingID[0];
  }
  if (global::m_pEepromHelper == NULL) return 1;
  return global::m_pEepromHelper->GetSettingInt(pszSettingID, 1, false);
}

void HueHelper::SetLightZoneState(char* pszHueID, int iZone, int iNew, bool bEpprom) {
  char szSettingID[ESP32_PREF_KEYMAX];
  CreateHueIDWithNum(pszHueID, szSettingID, ESP32_PREF_KEYMAX, iZone);
  _DBGPRINTF(PSTR("HueHelper:SetLightZoneState SettingID=%s Value=%d\n"), szSettingID, iNew);
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingInt(szSettingID, iNew);
  setUpdateHueConfig(true);
}

int HueHelper::GetLightColCorRed(char* pszHueID, bool bCalcHudID) const {
  char szSettingID[ESP32_PREF_KEYMAX];
  char* pszSettingID = pszHueID;
  if (bCalcHudID) {
    CreateHueIDWithStr(const_cast<char*>(pszHueID), szSettingID, ESP32_PREF_KEYMAX, "_ccr");
    pszSettingID = &szSettingID[0];
  }
  if (global::m_pEepromHelper == NULL) return 100;
  return global::m_pEepromHelper->GetSettingInt(pszSettingID, 100, false);
}

void HueHelper::SetLightColCorRed(char* pszHueID, int iNew, bool bEpprom) {
  char szSettingID[ESP32_PREF_KEYMAX];
  CreateHueIDWithStr(pszHueID, szSettingID, ESP32_PREF_KEYMAX, "_ccr");
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingInt(szSettingID, iNew);
  setUpdateHueConfig(true);
}

int HueHelper::GetLightColCorGreen(char* pszHueID, bool bCalcHudID) const {
  char szSettingID[ESP32_PREF_KEYMAX];
  char* pszSettingID = pszHueID;
  if (bCalcHudID) {
    CreateHueIDWithStr(const_cast<char*>(pszHueID), szSettingID, ESP32_PREF_KEYMAX, "_ccg");
    pszSettingID = &szSettingID[0];
  }
  if (global::m_pEepromHelper == NULL) return 100;
  return global::m_pEepromHelper->GetSettingInt(pszSettingID, 100, false);
}

void HueHelper::SetLightColCorGreen(char* pszHueID, int iNew, bool bEpprom) {
  char szSettingID[ESP32_PREF_KEYMAX];
  CreateHueIDWithStr(pszHueID, szSettingID, ESP32_PREF_KEYMAX, "_ccg");
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingInt(szSettingID, iNew);
  setUpdateHueConfig(true);
}

int HueHelper::GetLightColCorBlue(char* pszHueID, bool bCalcHudID) const {
  char szSettingID[ESP32_PREF_KEYMAX];
  char* pszSettingID = pszHueID;
  if (bCalcHudID) {
    CreateHueIDWithStr(const_cast<char*>(pszHueID), szSettingID, ESP32_PREF_KEYMAX, "_ccb");
    pszSettingID = &szSettingID[0];
  }
  if (global::m_pEepromHelper == NULL) return 100;
  return global::m_pEepromHelper->GetSettingInt(pszSettingID, 100, false);
}

void HueHelper::SetLightColCorBlue(char* pszHueID, int iNew, bool bEpprom) {
  char szSettingID[ESP32_PREF_KEYMAX];
  CreateHueIDWithStr(pszHueID, szSettingID, ESP32_PREF_KEYMAX, "_ccb");
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingInt(szSettingID, iNew);
  setUpdateHueConfig(true);
}

int HueHelper::GetLightColBrightness(char* pszHueID, bool bCalcHudID) const {
  char szSettingID[ESP32_PREF_KEYMAX];
  char* pszSettingID = pszHueID;
  if (bCalcHudID) {
    CreateHueIDWithStr(const_cast<char*>(pszHueID), szSettingID, ESP32_PREF_KEYMAX, "_cbr");
    pszSettingID = &szSettingID[0];
  }
  if (global::m_pEepromHelper == NULL) return 255;
  return global::m_pEepromHelper->GetSettingInt(pszSettingID, 255, false);
}

void HueHelper::SetLightColBrightness(char* pszHueID, int iNew, bool bEpprom) {
  char szSettingID[ESP32_PREF_KEYMAX];
  CreateHueIDWithStr(pszHueID, szSettingID, ESP32_PREF_KEYMAX, "_cbr");
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingInt(szSettingID, iNew);
  setUpdateHueConfig(true);
}

void HueHelper::CreateHueIDWithNum(char* pszHueID, char* pszBuf, size_t stBufLen, int iNum) {
  size_t stCpyLen = strlen(pszHueID);
  if (stCpyLen <= 10) {
    memcpy(pszBuf, pszHueID, stCpyLen);
  } else {
    memcpy(pszBuf, pszHueID + (stCpyLen - 10), 10);
    stCpyLen = 10;
  }
  if (iNum >= 10) {
    *(pszBuf + stCpyLen) = '_';
    *(pszBuf + stCpyLen + 1) = (char)(iNum / 10) + 48;
    *(pszBuf + stCpyLen + 2) = (char)(iNum % 10) + 48;
    *(pszBuf + stCpyLen + 3) = '\0';
  } else {
    *(pszBuf + stCpyLen) = '_';
    *(pszBuf + stCpyLen + 1) = (char)iNum + 48;
    *(pszBuf + stCpyLen + 2) = '\0';
  }
}

void HueHelper::CreateHueIDWithStr(char* pszHueID, char* pszBuf, size_t stBufLen, const char* pszExtra) {
  size_t stCpyLen = strlen(pszHueID);
  size_t stXtrLen = strlen(pszExtra);
  if (stCpyLen <= 10) {
    memcpy(pszBuf, pszHueID, stCpyLen);
  } else {
    memcpy(pszBuf, pszHueID + (stCpyLen - 10), 10);
    stCpyLen = 10;
  }
  memcpy(pszBuf + stCpyLen, pszExtra, stXtrLen);
  *(pszBuf + stCpyLen + stXtrLen) = '\0';
}

void HueHelper::InitHttpHelper(int iDeviceID) {
  if (m_pHttp[iDeviceID] == NULL) {
    if (iDeviceID == 0) {
      m_pHttp[iDeviceID] = &mHueHttp;
    } else {
      // HTTP Helper variables
      m_pHttp[iDeviceID] = new HTTPClient(); 
    }
    m_pHttp[iDeviceID]->setConnectTimeout(1000); // Set Timeout low to 1000ms
//  m_pHttp[iDeviceID]->setTimeout(1000); // Set Timeout low to 1000ms
    m_pHttp[iDeviceID]->setReuse(true);
  }
}
void HueHelper::DeInitHttpHelper(int iDeviceID) {
  if (m_pHttp[iDeviceID] != NULL) {
    if (iDeviceID > 0) {
      delete m_pHttp[iDeviceID]; 
      m_pHttp[iDeviceID] = NULL; 
    }
  }
}
void HueHelper::InitSecureClient() {
  this->DeInitSecureClient(); 
  m_pSecureClient = new WiFiClientSecureUdp(); 
  m_pSecureClient->setPreSharedKey(GetBridgeUser(), GetBridgeClientkey());
  m_pSecureClient->setHandshakeTimeout(GetHandshakeTimeout());  // Handshake timeout
  m_pSecureClient->setwriteFailTimeout(HUE_WRITEFAIL_TIMEOUT);          // Write Failure timeout (Default 5000ms)
  m_pSecureClient->setwriteFailDelay(HUE_WRITEFAIL_DELAY);              // Write Failure delay (Default 100ms)
}
void HueHelper::DeInitSecureClient() {
  if (m_pSecureClient != NULL) {
    delete m_pSecureClient;
    m_pSecureClient = NULL; 
  }
}
bool HueHelper::ConnectSecureClient() {
  if (m_pSecureClient == NULL) {
    InitSecureClient();
  }

  bool bConnected = m_pSecureClient->connected(); 
  if (!bConnected) {
    _DBGPRINTF(PSTR("HueHelper: ConnectSecureClient: DTLS Connect BEGIN to %s Port %d\n"), 
      GetBridgeIP(), m_nDTLSPort);
    if (!m_pSecureClient->connect(GetBridgeIP(), m_nDTLSPort)) {
      char szSecureError[MAX_HUE_PUTSIZE];
      int iLastError = m_pSecureClient->lastError(&szSecureError[0], MAX_HUE_PUTSIZE); 
      global::errprintf_P(PSTR("HueHelper: ConnectSecureClient: DTLS Connect failed to %s Port %d, Error Msg: %d/%s\n"), 
        GetBridgeIP(), m_nDTLSPort, iLastError, szSecureError);
    } else {
      _DBGPRINTF(PSTR("HueHelper: ConnectSecureClient: DTLS Connect SUCCESS to %s Port %d\n"), 
        GetBridgeIP(), m_nDTLSPort);
      bConnected = true; 
    }
  }

  // Set DTLS Connect State 
  pHueHelper->SetHueDtlsConnected(bConnected); 

  // return result
  return bConnected; 
}
void HueHelper::DisconnectSecureClient() {
  if (m_pSecureClient != NULL && m_pSecureClient->connected() ) {
    m_pSecureClient->stop();
  }
  // Set DTLS Connect State 
  pHueHelper->SetHueDtlsConnected(false); 
}
size_t HueHelper::SendToSecureClient(const uint8_t* pbBuf, size_t stBufLen) {
  size_t stRet = 0; 
  if (m_pSecureClient != NULL && m_pSecureClient->connected()) {
    // Write data out
    stRet = m_pSecureClient->write(pbBuf, stBufLen);
    if (stRet > 0) {
      _DBGPRINTF_ULTRA(PSTR("HueHelper: SendToSecureClient: Successfully send %d/%d bytes\n"), stBufLen, stRet);
    } else {
      // Perform dummy read first
      uint8_t dummy = 0;
      m_pSecureClient->read(&dummy, 0);
    }
  } else {
    global::errprintf_P(PSTR("HueHelper: SendToSecureClient: Error, INVALID TLS Session, DisconnectSecureClient\n"));
    this->DisconnectSecureClient();
    this->InitSecureClient(); // Make sure to drop old connection object
    // NOT NEEDED _taskDelay(1000); // Sleep for one second to give other taks time to work
  }
  return stRet; 
}

void HueHelper::InitHueLights() {
// TODO: WAIT FOR OLD TASK TO FINISH 
  // Set to INIT Mode
  this->SetTaskMode(HUE_TASK_MODE_INIT); 

  // Start/RESUME task
  this->InitHueTask();

  if (!pHueHelper->GetHueConnected()) {
    // Hue not connected first time, so we wait a little
    int iLoop = 0; 
    while( !pHueHelper->GetHueConnected() && iLoop < 1000 ) {
      _taskDelay(1);
      iLoop++; 
    }
  }
}
void HueHelper::InitHueTask() {
  // --- Create Hue Background TASK 
  if (handleTaskHueLoop == NULL) {
    // Create new Task      
    xTaskCreatePinnedToCore(&TaskHueLoop, 
                            "TaskHueLoop", 
                            8196 /*5120*/, 
                            pHueHelper, 
                            DEFAULT_TASK_PRIORITY + 1, /* +1 priority */
                            &handleTaskHueLoop,
                            SYSTEM_RUNNING_CORE /*ARDUINO_RUNNING_CORE*/); /* pin task to core 0 | avoid flickering */
    _DBGPRINTF(PSTR("HueHelper: InitHueTask: Created TASK for TaskHueLoop with HANDLE %d\n"), handleTaskHueLoop);
  } else {
    int iStatus = eTaskGetState(handleTaskHueLoop);
    if (iStatus == eSuspended) {
      _DBGPRINTF(PSTR("HueHelper: InitHueTask: TASK for TaskHueCode with HANDLE %d is suspended, resuming ... \n"), handleTaskHueLoop);
      vTaskResume(handleTaskHueLoop); 
    } else {
      _DBGPRINTF_VERBOSE(PSTR("HueHelper: InitHueTask: TASK for TaskHueCode already running ... \n"));
    }
  }
  // ---   
}
void* HueHelper::getLedsHuePtr() {
  return (void*) &m_leds_hue; 
}
byte HueHelper::getLedsMax() {
  return MAX_HUE_LEDS; 
}
RgbColor HueHelper::HueLightCalcColor(int iDeviceID) {
  int iSectors = 0; 
  int iRed = 0; 
  int iGreen = 0; 
  int iBlue = 0; 
  RgbColor finalRgbColor;

  // Reload Configs if changed!
  if (m_bUpdateHueConfig) {
    m_hue_device_var_max_brightness[iDeviceID] = GetLightColBrightness(m_hue_devices[iDeviceID]);
    m_hue_device_var_color_red[iDeviceID] = GetLightColCorRed(m_hue_devices[iDeviceID]);
    m_hue_device_var_color_green[iDeviceID] = GetLightColCorGreen(m_hue_devices[iDeviceID]);
    m_hue_device_var_color_blue[iDeviceID] = GetLightColCorBlue(m_hue_devices[iDeviceID]);

    // Get Zone Configs
    for (int i = 0; i < MAX_HUE_LEDS; i++) {
      m_hue_device_var_light_zones[iDeviceID][i] = GetLightZoneState( m_hue_devices[iDeviceID], i );
    }
    this->setUpdateHueConfig(false);
  } 
  
  // Loop through sectors
  for (int i = 0; i < MAX_HUE_LEDS; i++) {
    // Check if ZONE is enabled!
    if ( m_hue_device_var_light_zones[iDeviceID][i] == 1) {
      iRed += m_leds_hue[i].r;
      iGreen += m_leds_hue[i].g;
      iBlue += m_leds_hue[i].b;
      iSectors++;
    }
  }

  /* 1. Calc Common Color */
  if (iSectors > 0) {
    iRed = iRed / iSectors; 
    iGreen = iGreen / iSectors; 
    iBlue = iBlue / iSectors; 
  }

  /* 2. Calc color correction */
  if ( m_hue_device_var_color_red[iDeviceID] != 100) { iRed = (iRed * m_hue_device_var_color_red[iDeviceID]) / 100; }
  if ( m_hue_device_var_color_green[iDeviceID] != 100) { iGreen = (iGreen * m_hue_device_var_color_green[iDeviceID]) / 100; }
  if ( m_hue_device_var_color_blue[iDeviceID] != 100) { iBlue = (iBlue * m_hue_device_var_color_blue[iDeviceID]) / 100; }

  /* 3. Calc for max brightness */
  iRed = (iRed * m_hue_device_var_max_brightness[iDeviceID]) / 255;
  iGreen = (iGreen * m_hue_device_var_max_brightness[iDeviceID]) / 255;
  iBlue = (iBlue * m_hue_device_var_max_brightness[iDeviceID]) / 255;
  
  // Set final Color for Light
  finalRgbColor.r = (byte)(iRed < 255 ? (iRed > 0 ? iRed : 0) : 255);
  finalRgbColor.g = (byte)(iGreen < 255 ? (iGreen > 0 ? iGreen : 0) : 255);
  finalRgbColor.b = (byte)(iBlue < 255 ? (iBlue > 0 ? iBlue : 0) : 255);

  return finalRgbColor; 
}

String HueHelper::HueWebCommand(char* pszHueUpdateUrl, char* pszHuePutData, size_t stPutLen, int httpMethod) {
  int iHttpCode = 0; 
  int iRetryCnt = 0;
  bool bSuccess = false; 

  do 
  {
    // Init HTTP Helper if needed
    this->InitHttpHelper(0); 
    
    // Perform HTTP 
    m_pHttp[0]->begin(pszHueUpdateUrl); //HTTP
    if (httpMethod == HTTP_GET) {
      iHttpCode = m_pHttp[0]->GET();
    } else if (httpMethod == HTTP_PUT) { 
      iHttpCode = m_pHttp[0]->PUT( (uint8_t*) pszHuePutData, stPutLen);
    } else if (httpMethod == HTTP_POST) { 
      iHttpCode = m_pHttp[0]->POST( (uint8_t*) pszHuePutData, stPutLen);
    } else {
      global::errprintf_P(PSTR("HueHelper: HueWebCommand: INVALID HTTP Request Type %d\n"), httpMethod);
      return String();
    }
    
    // Output Putdata to debug
    if (global::m_bUltraDebugEnabled && pszHuePutData != NULL) { _DBGPRINTF(PSTR("HueHelper: HueWebCommand: Http request payload: %s\n"), pszHuePutData); }

    // Check http result
    if(iHttpCode == HTTP_CODE_OK) {
      // HTTP header has been send and Server response header has been handled
      _DBGPRINTF_ULTRA(PSTR("HueHelper: HueWebCommand: Http request to %s, HTTP code: %d\n"), pszHueUpdateUrl, iHttpCode);
  
      // Get String from http request for debug
      String payload = m_pHttp[0]->getString();
      _DBGPRINTF_ULTRA(PSTR("HueHelper: HueWebCommand: Http response payload: %.*s\n"), (payload.length() < 924 ? payload.length() : 924), payload.c_str());

      // Return payload
      return payload; 
    } else {
      _DBGPRINTF(PSTR("HueHelper: HueWebCommand: Http request try %d to %s failed with code %d, Error Msg: %s\n"), iRetryCnt, pszHueUpdateUrl, iHttpCode, m_pHttp[0]->errorToString(iHttpCode).c_str());
      // FAILED, retry!
      iRetryCnt++;

      // DeInit HTTP Helper
      pHueHelper->DeInitHttpHelper(0); 
      
      // Wait for a while before retry
      if (iRetryCnt < MAX_HUE_HTTP_RETRY) 
        _taskDelay(MAX_HUE_HTTP_RETRY_WAIT);
    }
  } while(!bSuccess && iRetryCnt < MAX_HUE_HTTP_RETRY ); 
  
  // FAILED!
  return String(); 
}
void HueHelper::HueLightOn(int iDeviceID) {
  char szHueUpdateUrl[MAX_HUE_URLSIZE]; 
  char szHuePutData[MAX_HUE_PUTSIZE]; 
  
  // Create State URL
  snprintf_P(szHueUpdateUrl, MAX_HUE_URLSIZE, PSTR("http://%s/api/%s/lights/%d/state"),
                              GetBridgeIP(), 
                              GetBridgeUser(),
                              m_hue_device_ids[iDeviceID] );
  // Create PUT Data
  size_t stPutLen = snprintf_P(szHuePutData, MAX_HUE_PUTSIZE, 
  PSTR(  "{\n"
            " \"on\": true,\n"
            " \"transitiontime\": %d,\n"
            " \"ct\": %d\n"
            "}\n"), 
            m_nFrameLength, 
            m_nHueCT
          ); 

  String payload = this->HueWebCommand(&szHueUpdateUrl[0], &szHuePutData[0], stPutLen, HTTP_PUT ); 
  if (payload.length() > 0) {
    // Set state to ON!
    m_hue_device_state[iDeviceID] = 1; 
  } else {
    global::errprintf_P(PSTR("HueHelper: HueLightOn: Failed to turn Hue Light ID %d ON\n"), iDeviceID);
  }
}
void HueHelper::HueLightOff(int iDeviceID) {
  char szHueUpdateUrl[MAX_HUE_URLSIZE]; 
  char szHuePutData[MAX_HUE_PUTSIZE]; 

  // Create State URL
  snprintf_P(szHueUpdateUrl, MAX_HUE_URLSIZE, PSTR("http://%s/api/%s/lights/%d/state"),
                              GetBridgeIP(), 
                              GetBridgeUser(),
                              m_hue_device_ids[iDeviceID] );
  // Create PUT Data
  size_t stPutLen = snprintf_P(szHuePutData, MAX_HUE_PUTSIZE, 
  PSTR(  "{\n"
            " \"on\": false\n"
            "}\n")
          ); 

  String payload = this->HueWebCommand(&szHueUpdateUrl[0], &szHuePutData[0], stPutLen, HTTP_PUT ); 
  if (payload.length() > 0) {
    _DBGPRINTF_ULTRA(PSTR("HueHelper: HueLightOff: Http response payload: %s\n"), payload.c_str());

    // Set state to off!
    m_hue_device_state[iDeviceID] = 0; 
  } else {
    global::errprintf_P(PSTR("HueHelper: HueLightOff: Failed to turn Hue Light ID %d OFF\n"), iDeviceID);
  }
}
void HueHelper::HueLightSetColor(int iDeviceID) {
  char szHueUpdateUrl[MAX_HUE_URLSIZE]; 
  char szHuePutData[MAX_HUE_PUTSIZE]; 

  // Calc Final RGB Color
  RgbColor finalRgbColor = this->HueLightCalcColor(iDeviceID); 
  // Convert to HSV for Hue!
  HsvColor finalHsvColor = RgbToHsv(finalRgbColor); 

  _DBGPRINTF_ULTRA(PSTR("HueHelper: HueLightSetColor: Set Hue Color for %s %d/%d/%d | %d/%d/%d\n"), 
      m_hue_devices[iDeviceID],
      finalRgbColor.r, finalRgbColor.g, finalRgbColor.b,
      finalHsvColor.h, finalHsvColor.s, finalHsvColor.v
      );

  // Create State URL
  snprintf_P(szHueUpdateUrl, MAX_HUE_URLSIZE, PSTR("http://%s/api/%s/lights/%d/state"),
                              GetBridgeIP(), 
                              GetBridgeUser(),
                              m_hue_device_ids[iDeviceID] );
  
  // Calc reduced color amount
  finalHsvColor.v = (finalHsvColor.v * GetLightColBrightness(m_hue_devices[iDeviceID]) / 255); 

  // Create PUT Data
  size_t stPutLen = snprintf_P(szHuePutData, MAX_HUE_PUTSIZE, 
  PSTR( "{\n"
        "  \"hue\": %d,\n"
        "  \"sat\": %d,\n"
        "  \"bri\": %d\n"
        "}\n"), 
        finalHsvColor.h * 256,
        finalHsvColor.s,
        finalHsvColor.v); 
  _DBGPRINTF_ULTRA(PSTR("HueHelper: HueLightSetColor: Http request data %s\n"), szHuePutData);

  String payload = this->HueWebCommand(&szHueUpdateUrl[0], &szHuePutData[0], stPutLen, HTTP_PUT ); 
  if (payload.length() == 0) {
    global::errprintf_P(PSTR("HueHelper: HueLightSetColor: Failed to set Color for Hue Light ID %d\n"), iDeviceID);
  }
}
bool HueHelper::HueStartDtls() {
  char szHueQueryUrl[MAX_HUE_URLSIZE]; 
  char szHuePutData[MAX_HUE_PUTSIZE]; 
  char szHueGroupName[MAX_HUE_MAXNAME]; 
  char szLights[MAX_HUE_MAXLIGHTIDS];
  char szIntToStr[MAX_INT_TO_STR];
  bool bGroupFound = false;
  
  // Init Hue GroupName 
  snprintf_P(szHueGroupName, MAX_HUE_MAXNAME, PSTR("AmbiBack %s"), global::GetDeviceID() );

  // --- Check existing GROUPS first
  // Get Groups URL
  snprintf_P(szHueQueryUrl, MAX_HUE_URLSIZE, PSTR("http://%s/api/%s/groups"),
                              GetBridgeIP(), 
                              GetBridgeUser() );
  { // Use Block due duplicated local vars                              
    String payload = this->HueWebCommand(&szHueQueryUrl[0], NULL, 0, HTTP_GET ); 
    if (payload.length() > 0) {
      _DBGPRINTF(PSTR("HueHelper: HueStartDtls: SUCCESSFULLY obtained Entertainment groups\n"));
  
      // Check if our entertain group exists
      // Parse JSON
      cJSON *json = cJSON_Parse(payload.c_str());
      if (json != NULL) {
        // Loop through devices
        const cJSON *json_group = NULL;
        cJSON_ArrayForEach(json_group, json) {
          cJSON *json_name = cJSON_GetObjectItemCaseSensitive(json_group, "name");
          _DBGPRINTF(PSTR("HueHelper: HueStartDtls: Group '%s' id '%s' found\n"), 
            (json_name != NULL && json_name->valuestring != NULL ? json_name->valuestring : "NULL"), (json_group != NULL && json_group->string != NULL ? json_group->string : "NULL" ));
  
          if ( strcmp(szHueGroupName, json_name->valuestring) == 0) {
            bGroupFound = true; 
            // Get and convert Entertainment GroupID to INT
            m_iHueEntertainmentGroupID = atoi(json_group->string);
            break;
          }
        }
        // Free JSON
        cJSON_Delete(json);
      }
    } else {
      global::errprintf_P(PSTR("HueHelper: HueStartDtls: Failed to read Entertainment Groups\n") );
  
      // FAILED!
      return false; 
    }
  }
  // --- 

  // --- Create Light IDs string
  szLights[0] = '\0';
  szIntToStr[0] = '\0';
  int iLightsCounter = 0; 
  // Limit to 10 Hue Lamps!
  for(int i=0; i < m_hue_devices_count && iLightsCounter < 10; i++) {
    if ( GetLightState(m_hue_devices[i]) == 1) {
      _DBGPRINTF(PSTR("HueHelper: HueStartDtls: Light enabled %d/%s\n"), m_hue_device_ids[i], m_hue_devices[i]);
      size_t stLightstLen = strlen(szLights);
      if (m_hue_device_ids[i] != HUE_NOT_INITIALIZED) {
        // Convert INT to STR
        itoa(m_hue_device_ids[i], &szIntToStr[0], MAX_INT_TO_STR);
        // Append ID to string
        strcat (szLights, (stLightstLen > 0 ? ", \"" : "\""));
        strcat (szLights, szIntToStr);
        strcat (szLights, "\"");
        /*
        snprintf_P(szLights + stLightstLen, MAX_HUE_MAXID-stLightstLen, PSTR("%s\"%d\""), 
          (stLightstLen > 0 ? ", \n" : ""), 
          m_hue_device_ids[i]);
        */

        iLightsCounter++; 
      } else {
        global::errprintf_P(PSTR("HueHelper: HueStartDtls: Light enabled %s, but Hue Bridge ID is UNKNOWN\n"), m_hue_devices[i]);
      }
    }
  }
  _DBGPRINTF(PSTR("HueHelper: HueStartDtls: Added %d light to Entertainment Group %s\n"), iLightsCounter, szHueGroupName);
  // --- 
  
  // --- Create / Update Entertainment Group
  int iHttpMethod = 0; 
  size_t stPostLen = 0; 
  if (bGroupFound && m_iHueEntertainmentGroupID != HUE_NOT_INITIALIZED) {
    // Create URL
    snprintf_P(szHueQueryUrl, MAX_HUE_URLSIZE, PSTR("http://%s/api/%s/groups/%d"),
                                GetBridgeIP(), 
                                GetBridgeUser(), 
                                m_iHueEntertainmentGroupID);
    // Create PUT Data
    stPostLen = snprintf_P(szHuePutData, MAX_HUE_PUTSIZE, 
      PSTR( "{\n"
            "\"lights\": [\n"
              "%s\n"
            "]\n"
            "}\n"), szLights); 
    iHttpMethod = HTTP_PUT; 
  } else {
    // Create URL
    snprintf_P(szHueQueryUrl, MAX_HUE_URLSIZE, PSTR("http://%s/api/%s/groups"),
                                GetBridgeIP(), 
                                GetBridgeUser() );
    // Create POST Data
    stPostLen = snprintf_P(szHuePutData, MAX_HUE_PUTSIZE, 
      PSTR( "{\n"
            "\"type\": \"Entertainment\",\n"
            "\"name\": \"%s\",\n"
            "\"lights\": [\n"
              "%s\n"
            "],\n"
            "\"class\": \"TV\"\n"
            "}\n"), szHueGroupName, szLights); 

    iHttpMethod = HTTP_POST; 
  }

  { // do BLOCK due duplicated local variables
    String payload = this->HueWebCommand(&szHueQueryUrl[0], &szHuePutData[0], stPostLen, iHttpMethod); 
    if (payload.length() > 0) {
      // GET HueGroupID!
      // Parse JSON
      cJSON *json = cJSON_Parse(payload.c_str());
      if (json != NULL) {
        // Loop through devices
        cJSON *json_success = cJSON_GetObjectItemCaseSensitive(json->child, "success");
        if (json_success != NULL) {
          _DBGPRINTF(PSTR("HueHelper: HueStartDtls: Entertainment Group ADDED with Status: %s\n"), (json_success->string != NULL ? json_success->string : "NULL"));
          
          cJSON *json_id = cJSON_GetObjectItemCaseSensitive(json_success, "id");
          if (json_id != NULL) {
            _DBGPRINTF(PSTR("HueHelper: HueStartDtls: json_id FOUND %s/%s\n"),
              (json_id->string != NULL ? json_id->string : "NULL"), (json_id->valuestring != NULL ? json_id->valuestring : "NULL"));
            _DBGPRINTF(PSTR("HueHelper: HueStartDtls: Entertainment Group ADDED with ID '%s'\n"), 
              json_id->valuestring );

            // Set Entertainment GroupID
            m_iHueEntertainmentGroupID = atoi(json_id->valuestring);   
            // snprintf_P(m_szHueEntertainmentGroupID, MAX_HUE_MAXID, PSTR("%s"), json_id->valuestring );
          }
        } else {
          global::errprintf_P(PSTR("HueHelper: HueStartDtls: Entertainment Group ADD FAILED with response: %s\n"), payload.c_str());

          // Free JSON
          cJSON_Delete(json);
          
          // FAILED!
          return false; 
        }
  
        // Free JSON
        cJSON_Delete(json);
      }    
    } else {
      global::errprintf_P(PSTR("HueHelper: HueStartDtls: Failed to turn Create/Set Hue Entertainment Group '%s' \n"), szHueGroupName);
  
      // FAILED!
      return false; 
    }
  }
  // ---

  // --- Set Group ACTION
  // Get Groups URL
  snprintf_P(szHueQueryUrl, MAX_HUE_URLSIZE, PSTR("http://%s/api/%s/groups/%d/action"),
                              GetBridgeIP(), 
                              GetBridgeUser(),
                              m_iHueEntertainmentGroupID);
  // Create PUT Data
  stPostLen = snprintf_P(szHuePutData, MAX_HUE_PUTSIZE, 
    PSTR( "{\n"
          "\"on\": true,"
          "\"hue\": 2000,"
          "\"effect\": \"colorloop\""
          "}\n")); 
                              
  { // Use Block due duplicated local vars                
    String payload = this->HueWebCommand(&szHueQueryUrl[0], &szHuePutData[0], stPostLen, HTTP_PUT); 
    if (payload.length() > 0) {
      // Parse JSON
      cJSON *json = cJSON_Parse(payload.c_str());
      if (json != NULL) {
        // Loop through devices
        cJSON *json_success = cJSON_GetObjectItemCaseSensitive(json->child, "success");
        if (json_success != NULL) {
          _DBGPRINTF(PSTR("HueHelper: HueStartDtls: SUCCESSFULLY set Action to Entertainment groups with Status: %s\n"), (json_success->string != NULL ? json_success->string : "NULL"));
        } else {
          global::errprintf_P(PSTR("HueHelper: HueStartDtls: FAILED to set Action to Entertainment group response: %s\n"), payload.c_str());
        }
        // Free JSON
        cJSON_Delete(json);
      } else {
        global::errprintf_P(PSTR("HueHelper: HueStartDtls: FAILED to set Action to Entertainment group #1\n") );
      }
    } else {
      global::errprintf_P(PSTR("HueHelper: HueStartDtls: FAILED to set Action to Entertainment group #2\n") );
  
      // FAILED!
      return false; 
    }
  }
  // --- 
  
  // --- Set enable streaming FLAG 
  // Create URL
  snprintf_P(szHueQueryUrl, MAX_HUE_URLSIZE, PSTR("http://%s/api/%s/groups/%d"),
                              GetBridgeIP(), 
                              GetBridgeUser(), 
                              m_iHueEntertainmentGroupID);
  // Create PUT Data
  size_t stPutLen = snprintf_P(szHuePutData, MAX_HUE_PUTSIZE, PSTR( "{\"stream\":{\"active\":true}}" )); 

  {
    String payload = this->HueWebCommand(&szHueQueryUrl[0], &szHuePutData[0], stPutLen, HTTP_PUT); 
    if (payload.length() > 0) {
      _DBGPRINTF(PSTR("HueHelper: HueStartDtls: SUCCESSFULLY send enable streaming flag!\n"));
    } else {
      global::errprintf_P(PSTR("HueHelper: HueStartDtls: FAILED to send enable streaming flag!\n"));
  
      // FAILED!
      return false; 
    }
  }
 
  // Init Secure Client for DTLS Streaming
  this->InitSecureClient(); 

  // Set DTLS Prepared State 
  this->SetHueDtlsPrepared(true); 

  // Init Light States!
  this->HueInitHueLightStatesDtls(); 

  // Successfully enabled DTLS Streaming!
  return true; 
}
void HueHelper::HueStopDtls() {
  char szHueQueryUrl[MAX_HUE_URLSIZE]; 
  char szHuePutData[MAX_HUE_PUTSIZE]; 

  if (WiFi.isConnected() == false) {
    global::errprintf_P(PSTR("HueHelper: HueStopDtls: Wifi Not Connected - SKIP HTTP Stuff\n"));
  } else {
  
  if (pHueHelper->GetHueDtlsPrepared()) {
    // Helper variables for DTLS Packet
    byte bufPacket[HUE_DTLS_MAX_PACKET_LENGTH]; 
    size_t bufLen = HUE_DTLS_HEADER_LENGTH;
    byte* ptr = bufPacket;
    // Clear Packet Data 
    memset(bufPacket, 0, HUE_DTLS_MAX_PACKET_LENGTH * sizeof(byte));
    // Create Header 
    *ptr++ = 'H';   // 1
    *ptr++ = 'u';   // 2
    *ptr++ = 'e';   // 3
    *ptr++ = 'S';   // 4
    *ptr++ = 't';   // 5
    *ptr++ = 'r';   // 6
    *ptr++ = 'e';   // 7
    *ptr++ = 'a';   // 8
    *ptr++ = 'm';   // 9
    *ptr++ = 0x01;  // Version Major 
    *ptr++ = 0x00;  // Version Minor
    *ptr++ = 0x00;  // Sequence ID (currently ignored)
    *ptr++ = 0x00;  // Reserved Byte
    *ptr++ = 0x00;  // Reserved Byte 
    *ptr++ = 0x00;  // 0x00 = RGB, 0x01 = XY Brightness
    *ptr++ = 0x00;  // Reserved Byte
  
    // Append Lights to DTLS Packet
    for(int i=0; i < m_hue_devices_count; i++) {
      // int iCurrentLightState = GetLightState(m_hue_devices[i]);
      if ( m_hue_device_state[i] /*iCurrentLightState */ == 1 ) {
        *ptr++ = 0x00;  // Type 0x00 = Light
        *ptr++ = 0x00;  // Unique ID of Light - HIGH BYTE
        *ptr++ = (byte) m_hue_device_ids[i];  // Unique ID of Light - LOW BYTE
        *ptr++ = 0x00; 
        *ptr++ = 0x00;
        *ptr++ = 0x00;
        *ptr++ = 0x00;
        *ptr++ = 0x00;
        *ptr++ = 0x00;
      }
      // Set lightState to OFF!
      m_hue_device_state[i] = 0; 
    }

    _DBGPRINTF(PSTR("HueHelper: HueStopDtls: Send ZERO Color to Hue Bridge FIRST\n"));
  
    // Calc BufLen
    bufLen = ptr-bufPacket; 
    // Finally send to client
    pHueHelper->SendToSecureClient(bufPacket, bufLen);
  } else {
    _DBGPRINTF(PSTR("HueHelper: HueStopDtls: not connected anymore, disable streaming flag\n"));
  }

  // --- Disable streaming FLAG 
  // Create URL
  snprintf_P(szHueQueryUrl, MAX_HUE_URLSIZE, PSTR("http://%s/api/%s/groups/%d"),
                              GetBridgeIP(), 
                              GetBridgeUser(), 
                              m_iHueEntertainmentGroupID);
  // Create PUT Data
  size_t stPutLen = snprintf_P(szHuePutData, MAX_HUE_PUTSIZE, PSTR( "{\"stream\":{\"active\":false}}" )); 

  {
    String payload = this->HueWebCommand(&szHueQueryUrl[0], &szHuePutData[0], stPutLen, HTTP_PUT); 
    if (payload.length() > 0) {
      // Parse JSON
      cJSON *json = cJSON_Parse(payload.c_str());
      if (json != NULL) {
        // Loop through devices
        cJSON *json_success = cJSON_GetObjectItemCaseSensitive(json->child, "success");
        if (json_success != NULL) {
          _DBGPRINTF(PSTR("HueHelper: HueStopDtls: SUCCESSFULLY send disable streaming flag: %s\n"), (json_success->string != NULL ? json_success->string : "NULL"));
        } else {
          global::errprintf_P(PSTR("HueHelper: HueStopDtls: FAILED disable streaming flag: %s\n"), payload.c_str());
        }
        // Free JSON
        cJSON_Delete(json);
      } else {
        global::errprintf_P(PSTR("HueHelper: HueStopDtls: FAILED disable streaming flag #1\n") );
      }
    } else {
      global::errprintf_P(PSTR("HueHelper: HueStopDtls: FAILED disable streaming flag #2\n"));
    }
  }
  // ---

  }

  // End DTLS Stream 
    // Set DTLS Connect State 
  if (this->GetHueDtlsConnected()) {
    _DBGPRINTF(PSTR("HueHelper: HueStopDtls: DisconnectSecureClient\n"));
    this->DisconnectSecureClient();
  }

  // Set DTLS Prepared State
  this->SetHueDtlsPrepared(false);

  if (WiFi.isConnected()) {
  
    // --- Disable Group ACTION
    // Get Groups URL
    snprintf_P(szHueQueryUrl, MAX_HUE_URLSIZE, PSTR("http://%s/api/%s/groups/%d/action"),
                                GetBridgeIP(), 
                                GetBridgeUser(),
                                m_iHueEntertainmentGroupID);
    // Create PUT Data
    size_t stPostLen = snprintf_P(szHuePutData, MAX_HUE_PUTSIZE, 
      PSTR( "{\n"
            "\"on\": false,"
            "\"effect\": \"none\""
            "}\n")); 
                                
    { // Use Block due duplicated local vars                
      String payload = this->HueWebCommand(&szHueQueryUrl[0], &szHuePutData[0], stPostLen, HTTP_PUT); 
      if (payload.length() > 0) {
        // Parse JSON
        cJSON *json = cJSON_Parse(payload.c_str());
        if (json != NULL) {
          // Loop through devices
          cJSON *json_success = cJSON_GetObjectItemCaseSensitive(json->child, "success");
          if (json_success != NULL) {
            _DBGPRINTF(PSTR("HueHelper: HueStopDtls: SUCCESSFULLY set Action of Entertainment group to OFF with Status: %s\n"), (json_success->string != NULL ? json_success->string : "NULL"));
          } else {
            global::errprintf_P(PSTR("HueHelper: HueStopDtls: FAILED to set Action of Entertainment group to OFF with response: %s\n"), payload.c_str());
          }
          // Free JSON
          cJSON_Delete(json);
        } else {
          global::errprintf_P(PSTR("HueHelper: HueStopDtls: FAILED to set Action to Entertainment group to OFF #1\n") );
        }
      } else {
        global::errprintf_P(PSTR("HueHelper: HueStopDtls: FAILED to set Action to Entertainment group to OFF #2\n") );
      }
    }
    // --- 
  }
}
void HueHelper::HueInitHueLightStatesDtls() {
  for(int i=0; i < m_hue_devices_count; i++) {
    m_hue_device_state[i] = GetLightState(m_hue_devices[i]);
    /*
    // Only disable configure lights
    if (m_hue_device_state[i] == 0) {
      _DBGPRINTF(PSTR("HueHelper: HueInitHueLightStatesDtls: Disable Light %s\n"), m_hue_devices[i]);
      pHueHelper->HueLightOff(i); 
    }
    */
  }
}

void TaskHueLoop(void * pvParameters) {
  String strResult; 
  char* pszHueBridgeIP = pHueHelper->GetBridgeIP() ; 
  char* pszHueBridgeUser = pHueHelper->GetBridgeUser() ; 
  _DBGPRINTF(PSTR("HueHelper: TaskHueLoop: ENTER\n"));

  do {
    // Task is running!
    pHueHelper->SetTaskRunning(true);

    if ( pHueHelper->GetTaskMode() == HUE_TASK_MODE_INIT) {
      _DBGPRINTF(PSTR("HueHelper: TaskHueLoop: HUE_TASK_MODE_INIT\n"));
      bool bAbort = false; 

// TODO: CHECK API LEVEL 
// Verify that the bridge has API version 1.22 or higher (check “apiversion” in the returned parameter when performing a GET on http://<ip-address>/api/config )

      // Do the work
      if (  pszHueBridgeIP != NULL && 
            pszHueBridgeUser != NULL && 
            *pszHueBridgeIP != '\0' && 
            *pszHueBridgeUser != '\0' && 
            pHueHelper->GetSupport() == 1
          ) {

        char szHueQuryUrl[MAX_HUE_URLSIZE]; 
        size_t stLen = snprintf_P(szHueQuryUrl, MAX_HUE_URLSIZE, PSTR("http://%s/api/%s/lights"),
                                    pszHueBridgeIP, 
                                    pszHueBridgeUser);

        String payload = pHueHelper->HueWebCommand(&szHueQuryUrl[0], NULL, 0, HTTP_GET); 
        if (payload.length() > 0) {
          _DBGPRINTF(PSTR("HueHelper: TaskHueLoop: HUE_TASK_MODE_INIT SUCCESSFULLY received hue lights!\n"));

          // Use direct string parsing, save memory!
          char* pszBegin = (char*) payload.c_str(); // Skip first opening braket
          char* pszWorker = (char*) payload.c_str();
          const char* pszSearch = "\"swversion\"";      // Search string
          char  szResult[MAX_HUE_MAXNAME];        // Result string
          size_t stLenPayload = payload.length(); // length of string to be searched
          size_t stLenSearch = strlen(pszSearch); // length of word to be searched
          size_t stLenWorker = 0;                 // length of partitonal string to be searched

          _DBGPRINTF(PSTR("HueHelper: TaskHueLoop: HUE_TASK_MODE_INIT Received lights string (%d), FreeMEM: %dKB, do char based search\n"), stLenPayload, ESP.getFreeHeap() / 1024);
          // reset devices count
          m_hue_devices_count = 0; 

          while ((pszWorker = strstr(pszWorker, pszSearch)) != NULL) {
            // Found UniqueID string, copy search for beginning ID
            pszWorker += stLenSearch; // Move ahead of search str
            
            // Try to parse unique ID from string by search for next occurrence of the double quote 
            while (*pszWorker) {
              if (*pszWorker == '}') {
                pszWorker++; // Found Closing braket ID, next char
                stLenWorker = pszWorker - pszBegin;
*pszWorker = '}'; //close braket 
*(pszWorker+1) = '\0'; //Terminate at this position

                // Parse JSON
                cJSON *json = cJSON_Parse((const char*)pszBegin);
                if (json != NULL) {
                  const cJSON *json_light = NULL;
                  int iID = HUE_NOT_INITIALIZED; 
                  
                  _DBGPRINTF_ULTRA( PSTR("HueHelper: TaskHueLoop: HUE_TASK_MODE_INIT Found valid JSON TREE: '%s ...'\n"), pszBegin /*json_print*/ ); 

                  // Loop through devices
                  cJSON_ArrayForEach(json_light, json) {
                    
                    // Get Hue device ID from ROOT
                    if (json_light->string != NULL)
                      iID = atoi(json_light->string);

                    cJSON *json_name = cJSON_GetObjectItemCaseSensitive(json_light, "name");
                    cJSON *json_uniqueid = cJSON_GetObjectItemCaseSensitive(json_light, "uniqueid");
                
                    if (json_uniqueid != NULL && iID >= 0) {
                      _DBGPRINTF(PSTR("HueHelper: TaskHueLoop: HUE_TASK_MODE_INIT Found Hue Device '%s' with ID '%d', Unqiue ID '%s' \n"), json_name->valuestring, iID, json_uniqueid->valuestring);
                
                      // add to deteced hue devices
                      size_t stLen = strlen(json_uniqueid->valuestring); 
                      snprintf_P(m_hue_devices[m_hue_devices_count], MAX_HUE_MAXID, PSTR("%s"),
                        (stLen <= 10 ? json_uniqueid->valuestring : json_uniqueid->valuestring + strlen(json_uniqueid->valuestring) - 10)
                      );
                      
                      // Internal HUE Number starts with 1, init variables
                      m_hue_device_ids[m_hue_devices_count] = iID; // m_hue_devices_count+1;  
                      m_hue_device_state[m_hue_devices_count] = 0;  
                      
                      // Increment Hue Number
                      m_hue_devices_count++; 
                    } else {
                      global::errprintf_P(PSTR("HueHelper: TaskHueLoop: JSON PARSE LIGHT NODE FAILED for ID '%d'\n"), iID);
                    }
                  }
                
                  // Free JSON
                  cJSON_Delete(json); 
                } else {
                  global::errprintf_P(PSTR("HueHelper: TaskHueLoop: JSON PARSE FAILED at %d\n"), pszBegin - payload.c_str() );
                
                  // Connect was unsuccessfull!
                  pHueHelper->SetHueConnected(false); 

                  // Abort further processing
                  bAbort = true; 

                  break; 
                }
                
*pszWorker  = '{'; //open braket 
*(pszWorker+1)= '"'; // FIX JSON Begin

                // Set new Begin Pointer
                pszBegin = pszWorker; 
    
                // Break the sub while loop
                break; 
              } else {
                // Go to next char
                pszWorker++; 
              }
            }

            // Check if LIMIT is reached!
            if (m_hue_devices_count >= MAX_HUE_DEVICES) {
              _DBGPRINTF(PSTR("HueHelper: TaskHueLoop: HUE_TASK_MODE_INIT MAX(%d) HUE DEVICES REACHED , can not add more devices\n"), m_hue_devices_count);
              break; 
            }
          }

          if (bAbort == false) {
            // Connect was successfull!
            pHueHelper->SetHueConnected(true); 
  
            if (global::m_pLedHelper->GetLedMode() != LED_MODE_OFF) {
              // Continue to run TASK with LOOP Mode
              pHueHelper->SetTaskMode(HUE_TASK_MODE_LOOP); 
            }
          }
        } else {
          global::errprintf_P(PSTR("HueHelper: TaskHueLoop: FAILED to receive hue lights!\n"));
  
          // Connect was unsuccessfull!
          pHueHelper->SetHueConnected(false); 
        }
      } else {
        _DBGPRINTF(PSTR("HueHelper: TaskHueLoop: HueIP/HueUser is NULL, or HueSupport disabled - nothing we can do...\n"));
  
        // Connect was unsuccessfull!
        pHueHelper->SetHueConnected(false); 
      }
      if (!pHueHelper->GetHueConnected()) {
        // Remove HTTP Helper from memory
        for(int i=0; i < MAX_HUE_HTTPHELPERS; i++)
          pHueHelper->DeInitHttpHelper(i); 
      }

      if (pHueHelper->GetTaskMode() != HUE_TASK_MODE_LOOP) {
        // Suspend ourselves.
        _DBGPRINTF(PSTR("HueHelper: TaskHueLoop: Suspend TASK after HUE_TASK_MODE_INIT - handle %d \n"), handleTaskHueLoop);
        pHueHelper->SetTaskRunning(false);
        
        vTaskSuspend(handleTaskHueLoop);

        // Break Loop and start from top
        continue;
      }
    } 

    if ( pHueHelper->GetTaskMode() == HUE_TASK_MODE_LOOP) {
      // Do the work only if Hue Bridge and User are valid
      if (pszHueBridgeIP != NULL && 
          pszHueBridgeUser != NULL && 
          *pszHueBridgeIP != '\0' && 
          *pszHueBridgeUser != '\0'
          ) {

        // Reset Timer check helpers
        m_nHueTimeStart = 0;
        m_nHueTimeFPSCheck = millis() + 1000;
        m_nHueTimeDebug1 = 0;
        m_nHueTimeDebug2 = 0;
        m_nHueTimeFinish = 0;
        long nExtraSleep = 0;

        // Helper variables for DTLS Packet
        byte bufPacket[HUE_DTLS_MAX_PACKET_LENGTH]; 
        size_t bufLen = HUE_DTLS_HEADER_LENGTH;

        // Enter processing loop
        do {
          int iCurrenFrameLength = (!global::GetFirmwareRunning() ? (pHueHelper->GetHueUseDtls() ? pHueHelper->GetFrameLengthDtls() : m_nFrameLength) : 200 /* 5 FPS while OTA update */);
          int iLightsUpdated = 0; 

          if (  WiFi.isConnected() && 
                global::m_pLedHelper->GetLedMode() != LED_MODE_OFF && 
                pHueHelper->GetSupport() == 1 ) {
            // Handle DTLS Stuff
            if (pHueHelper->GetHueUseDtls()) {
              /* tell Hue Bridge that we are going to do DTLS */
              if (!pHueHelper->GetHueDtlsPrepared()) {
                // Disable fucking Watchdog!
                global::DisableSysCoreWatchDog();

                // Start prepared DTLS Streaming
                if (!pHueHelper->HueStartDtls()){
                  // Abort LOOP if DTLS not prepared
                  global::errprintf_P(PSTR("HueHelper: TaskHueLoop: DTLS Prepare failed, try again next LOOP ...\n"));
                  _taskDelay(1000);
                  // Enable fucking Watchdog!
                  global::EnableSysCoreWatchDog(); 
                  continue;  
                }
                // Enable fucking Watchdog!
                global::EnableSysCoreWatchDog(); 
                _DBGPRINTF(PSTR("HueHelper: TaskHueLoop: DTLS Prepare DONE ...\n"));
# if defined(ESP32_DEBUG_FREEMEM)
                global::printEspMemUsage("HueHelper", "TaskHueLoop"); 
# endif
                
/*              } else {
                _DBGPRINTF(PSTR("HueHelper: TaskHueLoop: DTLS Prepare already prepared ...\n"));*/
              }
  
              /* perform handshake stuff for DTLS */
              if (!pHueHelper->GetHueDtlsConnected()) {
                // Disable fucking Watchdog!
                global::DisableSysCoreWatchDog();
                
                // Start DTLS Handshake!
                if (!pHueHelper->ConnectSecureClient()){
                  // Stop DTLS
                  pHueHelper->HueStopDtls();
                  // Abort LOOP if DTLS not connected
                  global::errprintf_P(PSTR("HueHelper: TaskHueLoop: DTLS ConnectSecureClient failed, try again next LOOP ...\n"));
                  _taskDelay(1000);
                  // Enable fucking Watchdog!
                  global::EnableSysCoreWatchDog(); 
                  continue;  
                }
                // Enable fucking Watchdog!
                global::EnableSysCoreWatchDog(); 
                _DBGPRINTF(PSTR("HueHelper: TaskHueLoop: DTLS ConnectSecureClient DONE ...\n"));
# if defined(ESP32_DEBUG_FREEMEM)
                global::printEspMemUsage("HueHelper", "TaskHueLoop"); 
# endif

                // --- Cache Configs after successfull connect!
                for(int n=0; n < m_hue_devices_count; n++) {
                  m_hue_device_var_enabled[n] = pHueHelper->GetLightState(m_hue_devices[n]);
                  if ( m_hue_device_var_enabled[n] == 1 ) {
                    m_hue_device_var_max_brightness[n] = pHueHelper->GetLightColBrightness(m_hue_devices[n]);
                    m_hue_device_var_color_red[n] = pHueHelper->GetLightColCorRed(m_hue_devices[n]);
                    m_hue_device_var_color_green[n] = pHueHelper->GetLightColCorGreen(m_hue_devices[n]);
                    m_hue_device_var_color_blue[n] = pHueHelper->GetLightColCorBlue(m_hue_devices[n]);
                    // Get Zone Configs
                    for (int i = 0; i < MAX_HUE_LEDS; i++) {
                      m_hue_device_var_light_zones[n][i] = pHueHelper->GetLightZoneState( m_hue_devices[n], i );
                    }
                  }
                }
                // --- 
              }
            }

            if ( pHueHelper->getNewLedData() || pHueHelper->GetHueUseDtls() /*always on Dtls*/ ) {
              // Count time now!
              m_nHueTimeStart = millis();
              
              // Set NewLedData to FALSE !
              pHueHelper->setNewLedData(false);
  
              if (pHueHelper->GetHueUseDtls()) {
                byte* ptr = bufPacket;
                // Clear Packet Data 
                memset(bufPacket, 0, HUE_DTLS_MAX_PACKET_LENGTH * sizeof(byte));
                // Create Header 
                *ptr++ = 'H';   // 1
                *ptr++ = 'u';   // 2
                *ptr++ = 'e';   // 3
                *ptr++ = 'S';   // 4
                *ptr++ = 't';   // 5
                *ptr++ = 'r';   // 6
                *ptr++ = 'e';   // 7
                *ptr++ = 'a';   // 8
                *ptr++ = 'm';   // 9
                *ptr++ = 0x01;  // Version Major 
                *ptr++ = 0x00;  // Version Minor
                *ptr++ = 0x00;  // Sequence ID (currently ignored)
                *ptr++ = 0x00;  // Reserved Byte
                *ptr++ = 0x00;  // Reserved Byte 
                *ptr++ = 0x00;  // 0x00 = RGB, 0x01 = XY Brightness
                *ptr++ = 0x00;  // Reserved Byte

                // Calc Debug Time
                m_nHueTimeDebug1 = millis() - m_nHueTimeStart; 

                // Append Lights to DTLS Packet
                int iLightChanged = -1; 
                for(int i=0; i < m_hue_devices_count; i++) {
                  m_hue_device_var_enabled[i] = (m_nHueTimeFPSCheck == 0 || millis() > m_nHueTimeFPSCheck ? pHueHelper->GetLightState(m_hue_devices[i]) : m_hue_device_var_enabled[i] );
                  if (m_hue_device_state[i] != m_hue_device_var_enabled[i] ) {
                    iLightChanged = i; 
                    break; 
                  }
                  
                  // Continue with packet
                  if ( m_hue_device_var_enabled[i] == 1 ) {
                    // Calc Final RGB Color
                    RgbColor finalRgbColor = pHueHelper->HueLightCalcColor(i);
                    short shRed = finalRgbColor.r*256;
                    short shGreen = finalRgbColor.g*256;
                    short shBlue = finalRgbColor.b*256;

                    *ptr++ = 0x00;  // Type 0x00 = Light
                    *ptr++ = 0x00;  // Unique ID of Light - HIGH BYTE
                    *ptr++ = (byte) m_hue_device_ids[i];  // Unique ID of Light - LOW BYTE
                    *ptr++ = ((shRed & 0xFF00) >> 8); 
                    *ptr++ = (shRed & 0xFF);
                    *ptr++ = ((shGreen & 0xFF00) >> 8); 
                    *ptr++ = (shGreen & 0xFF);
                    *ptr++ = ((shBlue & 0xFF00) >> 8); 
                    *ptr++ = (shBlue & 0xFF);

                    iLightsUpdated++;

                    // --- Perform other tasks 
                    _taskYIELD(); 
                    // --- 
                  }
                }

                // Calc BufLen
                bufLen = ptr-bufPacket; 

                m_nHueTimeDebug2 = millis() - m_nHueTimeStart; // Calc Debug Time

                // Finally send to client
                pHueHelper->SendToSecureClient(bufPacket, bufLen);

                // --- Perform other tasks 
                _taskYIELD(); 
                // --- 

                if (iLightChanged >= 0) {
                  _DBGPRINTF(PSTR("HueHelper: TaskHueLoop: Hue LightState %d changed, Restarting DTLS ...\n"), iLightChanged);
                  
                  // ReStart prepared DTLS Streaming | LightState is fixed there!
                  pHueHelper->HueStopDtls(); 
                  continue;
                }
              } else {
                // --- WAIT if LED Update is in process!
                while( global::m_pLedHelper->GetShowLedActive() ) {
                  _taskDelay(1);
                }
                // ---
                
                // Slow HTTP Mode 
                for(int i=0; i < m_hue_devices_count; i++) {
                  if ( pHueHelper->GetLightState(m_hue_devices[i]) == 1) {
                    if (m_hue_device_state[i] == 0) {
                      _DBGPRINTF(PSTR("HueHelper: TaskHueCode: Enable Light %s\n"), m_hue_devices[i]);
                      pHueHelper->HueLightOn(i); 
                    }
                    // Set Hue Color
                    m_nHueTimeDebug1 = millis() - m_nHueTimeStart; // Calc Debug Time
                    pHueHelper->HueLightSetColor(i); 
                    m_nHueTimeDebug2 = millis() - m_nHueTimeStart; // Calc Debug Time
                    iLightsUpdated++; 
                  } else {
                    if (m_hue_device_state[i] == 1) {
                      _DBGPRINTF(PSTR("HueHelper: TaskHueCode: Disable Light %s\n"), m_hue_devices[i]);
                      pHueHelper->HueLightOff(i); 
                    }
                  }
                }
              }
  
              // Calc Processed time
              m_nHueTimeFinish = millis() - m_nHueTimeStart;

              if (iLightsUpdated > 0) {
                // Increment Framecounter
                pHueHelper->IncrementCurrentFramesCtr(); 

                  // Calc extra Sleep time    
                nExtraSleep = iCurrenFrameLength - m_nHueTimeFinish;

                /* Output in ULTRA Debug, or every second only! */
                if (m_nHueTimeFPSCheck == 0 || millis() > m_nHueTimeFPSCheck ) {
                  pHueHelper->SetCurrentFps(pHueHelper->GetCurrentFramesCtr()); 
                  pHueHelper->SetCurrentFpsSkip(pHueHelper->GetCurrentFramesSkipCtr()); 

                  _DBGPRINTF(PSTR("HueHelper: TaskHueLoop: PROCESSED in %d/%d/%d ms, Lights Updated %d, Fps %d, Skip %d, CurrenFrameLength %d, ExtraSleep %d ms\n"), 
                      m_nHueTimeDebug1, 
                      m_nHueTimeDebug2, 
                      m_nHueTimeFinish, 
                      iLightsUpdated, 
                      pHueHelper->GetCurrentFps(), 
                      pHueHelper->GetCurrentFpsSkip(), 
                      iCurrenFrameLength, 
                      nExtraSleep);

                  // Reset back to 0
                  pHueHelper->SetCurrentFramesCtr(0); 
                  pHueHelper->SetCurrentFramesSkipCtr(0); 
                  m_nHueTimeFPSCheck = millis() + 1000;
                }
                
                /* Sleep only if necessary */
                if (  m_nHueTimeFinish < iCurrenFrameLength &&
                      nExtraSleep > 0 ) {
                  _taskDelay(nExtraSleep);
                } else if (nExtraSleep < 0) {
                  // SKIP FRAME!
                  nExtraSleep = iCurrenFrameLength + nExtraSleep; 
                  if (nExtraSleep > 0) {
                    // _DBGPRINTF_ULTRA(PSTR("HueHelper: TaskHueLoop: SKIP FRAME %d/%d\n"), iCurrenFrameLength - m_nHueTimeFinish, nExtraSleep);
                    _taskDelay(nExtraSleep);
                  } else {
                    _DBGPRINTF_ULTRA(PSTR("HueHelper: TaskHueLoop: SKIP MORE THAN ONE FRAME %d\n"), nExtraSleep);
                  }
                  pHueHelper->IncrementCurrentFramesSkipCtr(); 
                }
              } else {
                 _DBGPRINTF_ULTRA(PSTR("HueHelper: TaskHueLoop: NO LIGHTS UPDATED, sleep for a while ...\n"));
                _taskDelay(1000);
              }
            } else {
              _DBGPRINTF_ULTRA(PSTR("HueHelper: TaskHueLoop: NO LED DATA...\n"));
              _taskDelay(iCurrenFrameLength);
              continue; 
            }
          } else {
            _DBGPRINTF(PSTR("HueHelper: TaskHueLoop: LEDMode %d, HueSupport %s, WiFi Connected %s\n"), 
              global::m_pLedHelper->GetLedMode(),
              (pHueHelper->GetSupport() == 1 ? "ENABLED" : "DISABLED"),
              (WiFi.isConnected() ? "YES" : "NO"));

            if (pHueHelper->GetHueUseDtls() && pHueHelper->GetHueDtlsConnected()) {
              // Stop DTLS
              pHueHelper->HueStopDtls(); 
            } else {
              for(int i=0; i < m_hue_devices_count; i++) {
                if ( pHueHelper->GetLightState(m_hue_devices[i]) == 1) {
                  if (m_hue_device_state[i] == 1) {
                    _DBGPRINTF(PSTR("HueHelper: TaskHueCode: Disable Light  %s\n"), m_hue_devices[i]);
                    pHueHelper->HueLightOff(i); 
                  }
                } 
              }
            }
            
            break; 
          }
        } while(true);

        // Stop DTLS
        if (pHueHelper->GetHueUseDtls() && pHueHelper->GetHueDtlsConnected()) {
          pHueHelper->HueStopDtls();
        }
                  
      } else {
        _DBGPRINTF(PSTR("HueHelper: TaskHueLoop: HueIP or HueUser is NULL, nothing we can do...\n"));
        // break;
      }
  
      // Remove HTTP Helper from memory
      for(int i=0; i < MAX_HUE_HTTPHELPERS; i++)
        pHueHelper->DeInitHttpHelper(i); 
    }

    if (pHueHelper->GetTaskMode() != HUE_TASK_MODE_INIT) {
      // Suspend ourselves.
      _DBGPRINTF(PSTR("HueHelper: TaskHueLoop: Suspend TASK after HUE_TASK_MODE_LOOP - handle %d \n"), handleTaskHueLoop);
      pHueHelper->SetTaskRunning(false);
      vTaskSuspend(NULL);

      // Break Loop and start from top
      continue; 
    }

    // --- Perform other tasks 
    _taskYIELD(); 
    // --- 
  } while(true);

  global::errprintf_P(PSTR("HueHelper: TaskHueLoop: EXIT\n"));    
}


// --- Color Convert helpers
RgbColor HsvToRgb(HsvColor hsv)
{
    RgbColor rgb;
    unsigned char region, remainder, p, q, t;

    if (hsv.s == 0)
    {
        rgb.r = hsv.v;
        rgb.g = hsv.v;
        rgb.b = hsv.v;
        return rgb;
    }

    region = hsv.h / 43;
    remainder = (hsv.h - (region * 43)) * 6; 

    p = (hsv.v * (255 - hsv.s)) >> 8;
    q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
    t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
        case 0:
            rgb.r = hsv.v; rgb.g = t; rgb.b = p;
            break;
        case 1:
            rgb.r = q; rgb.g = hsv.v; rgb.b = p;
            break;
        case 2:
            rgb.r = p; rgb.g = hsv.v; rgb.b = t;
            break;
        case 3:
            rgb.r = p; rgb.g = q; rgb.b = hsv.v;
            break;
        case 4:
            rgb.r = t; rgb.g = p; rgb.b = hsv.v;
            break;
        default:
            rgb.r = hsv.v; rgb.g = p; rgb.b = q;
            break;
    }

    return rgb;
}
HsvColor RgbToHsv(RgbColor rgb)
{
    HsvColor hsv;
    unsigned char rgbMin, rgbMax;

    rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
    rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);

    hsv.v = rgbMax;
    if (hsv.v == 0)
    {
        hsv.h = 0;
        hsv.s = 0;
        return hsv;
    }

    hsv.s = 255 * long(rgbMax - rgbMin) / hsv.v;
    if (hsv.s == 0)
    {
        hsv.h = 0;
        return hsv;
    }

    if (rgbMax == rgb.r)
        hsv.h = 0 + 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
    else if (rgbMax == rgb.g)
        hsv.h = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
    else
        hsv.h = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);

    return hsv;
}
// ---

#endif
