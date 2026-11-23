#ifdef ENABLE_HUE
# ifndef huehelper_h
# define huehelper_h

#define MAX_HUE_URLSIZE 128
#define MAX_HUE_MAXNAME 128
#define MAX_HUE_PUTSIZE 256
#define MAX_HUE_MAXLIGHTIDS 64
#define MAX_HUE_MAXID 32
#define MAX_HUE_DEVICES 50  // Maximum number Hue Bridge Supports: https://huehomelighting.com/many-lights-can-1-hue-bridge-support/
#define MAX_HUE_LEDS 12
#define MAX_HUE_HTTPHELPERS 1
#define HUE_LED_TOP_LEFT 0
#define HUE_LED_TOP_CENTER1 1
#define HUE_LED_TOP_CENTER2 2
#define HUE_LED_TOP_CENTER3 3
#define HUE_LED_TOP_RIGHT 4
#define HUE_LED_MIDDLE_LEFT 5
#define HUE_LED_MIDDLE_RIGHT 6
#define HUE_LED_BOTTOM_LEFT 7
#define HUE_LED_BOTTOM_CENTER1 8
#define HUE_LED_BOTTOM_CENTER2 9
#define HUE_LED_BOTTOM_CENTER3 10
#define HUE_LED_BOTTOM_RIGHT 11

#define HUE_LED_TOP_SECTORS 5
#define HUE_LED_BOTTOM_SECTORS 5
#define HUE_LED_LEFT_SECTORS 3
#define HUE_LED_RIGHT_SECTORS 3

#define HUE_DTLS_HEADER_LENGTH 16
#define HUE_DTLS_MAX_PACKET_LENGTH 106  // 16 Bytes header + 10 x 9Bytes per LIGHT

#define MAX_HUE_HTTP_RETRY 5            // Max HTTP retries
#define MAX_HUE_HTTP_RETRY_WAIT 1000    // ms how long to wait

#define HUE_NOT_INITIALIZED -1

// Bridge/config buffer sizes (match global.h when ENABLE_HUE)
#define HUE_STR_IP_MAX 65
#define HUE_BRIDGE_USER_MAX 129

// --- RGB to HSV converter helpers
typedef struct RgbColor
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
} RgbColor;

typedef struct HsvColor
{
    unsigned char h;
    unsigned char s;
    unsigned char v;
} HsvColor;
// ---

class HueHelper {
public:
    // Constructors
    HueHelper();

    // Declare public helper functions
    void InitHttpHelper(int iDeviceID);
    void DeInitHttpHelper(int iDeviceID);
    void InitSecureClient();
    void DeInitSecureClient();
    bool ConnectSecureClient();
    void DisconnectSecureClient(); 
    size_t SendToSecureClient(const uint8_t*, size_t stBufLen);

    void InitHueLights();
    void InitHueTask();

    inline bool getNewLedData() { return m_bNewLedData; }
    inline void setNewLedData(bool bNewData) { m_bNewLedData = bNewData; }

    inline bool getUpdateHueConfig() { return m_bUpdateHueConfig; }
    inline void setUpdateHueConfig(bool bUpdateHueConfig) { m_bUpdateHueConfig = bUpdateHueConfig; }

    // Get/Set Helpers
    void SetHueUseDtls(bool bNew) {m_bHueUseDtls = bNew; } 
    inline bool GetHueUseDtls() {return m_bHueUseDtls; }
    void SetHueConnected(bool bNew) {m_bHueConnected = bNew; } 
    inline bool GetHueConnected() {return m_bHueConnected; }
    void SetTaskMode(int iNew) {m_iTaskMode = iNew; } 
    inline int GetTaskMode() {return m_iTaskMode; }
    void SetTaskRunning(bool bNew) {m_bTaskRunning = bNew; } 
    inline bool GetTaskRunning() {return m_bTaskRunning; }
    void SetHueDtlsConnected(bool bNew) {m_bHueDtlsConnected = bNew; } 
    inline bool GetHueDtlsConnected() {return m_bHueDtlsConnected; }
    void SetHueDtlsPrepared(bool bNew) {m_bHueDtlsPrepared = bNew; } 
    inline bool GetHueDtlsPrepared() {return m_bHueDtlsPrepared; }
    byte GetCurrentFps() { return m_bCurrentFps; }
    void SetCurrentFps(byte bNew) { m_bCurrentFps = bNew; }
    byte GetCurrentFramesCtr() { return m_bCurrentFramesCtr; }
    void SetCurrentFramesCtr(byte bNew) { m_bCurrentFramesCtr = bNew; }
    void IncrementCurrentFramesCtr() { m_bCurrentFramesCtr++; }
    byte GetCurrentFpsSkip() { return m_bCurrentFpsSkip; }
    void SetCurrentFpsSkip(byte bNew) { m_bCurrentFpsSkip = bNew; }
    byte GetCurrentFramesSkipCtr() { return m_bCurrentFramesSkipCtr; }
    void SetCurrentFramesSkipCtr(byte bNew) { m_bCurrentFramesSkipCtr = bNew; }
    void IncrementCurrentFramesSkipCtr() { m_bCurrentFramesSkipCtr++; }
    
    void* getLedsHuePtr();
    byte getLedsMax();

    RgbColor HueLightCalcColor(int iDeviceID); 
    String HueWebCommand(char* pszHueUpdateUrl, char* pszHuePutData, size_t stPutLen, int httpMethod); 
    void HueLightOn(int iDeviceID); 
    void HueLightOff(int iDeviceID); 
    void HueLightSetColor(int iDeviceID); 
    bool HueStartDtls(); 
    void HueStopDtls(); 
    void HueInitHueLightStatesDtls(); 

    /* Bridge/config state and persistence (moved from global) */
    byte GetSupport() const { return m_bHueSupport; }
    void SetSupport(byte bNew, bool bEpprom, bool bInitHue);
    void SetSupport(byte bNew, bool bEpprom) { SetSupport(bNew, bEpprom, true); }
    void SetSupport(byte bNew) { SetSupport(bNew, true); }
    byte GetFrameLengthDtls() const { return m_nFrameLengthDtls; }
    void SetFrameLengthDtls(byte bNew, bool bEpprom);
    void SetFrameLengthDtls(byte bNew) { SetFrameLengthDtls(bNew, true); }
    byte GetHandshakeTimeout() const { return m_nHandshakeTimeout; }
    void SetHandshakeTimeout(byte bNew, bool bEpprom);
    void SetHandshakeTimeout(byte bNew) { SetHandshakeTimeout(bNew, true); }
    char* GetBridgeIP() { return m_huebridgeip; }
    void SetBridgeIP(char* pszNew, bool bEpprom);
    void SetBridgeIP(char* pszNew) { SetBridgeIP(pszNew, true); }
    char* GetBridgeUser() { return m_huebridgeuser; }
    void SetBridgeUser(char* pszNew, bool bEpprom);
    void SetBridgeUser(char* pszNew) { SetBridgeUser(pszNew, true); }
    char* GetBridgeClientkey() { return m_huebridgeclientkey; }
    void SetBridgeClientkey(char* pszNew, bool bEpprom);
    void SetBridgeClientkey(char* pszNew) { SetBridgeClientkey(pszNew, true); }
    int GetPreset() const { return m_iHuePreset; }
    void SetPreset(int iIndex, bool bEpprom);
    void SetPreset(int iIndex) { SetPreset(iIndex, true); }
    int GetLightState(char* pszHueID, bool bCalcHudID) const;
    int GetLightState(char* pszHueID) const { return GetLightState(pszHueID, true); }
    void SetLightState(char* pszHueID, int iNew, bool bEpprom);
    void SetLightState(char* pszHueID, int iNew) { SetLightState(pszHueID, iNew, true); }
    int GetLightZoneState(char* pszHueID, int iZone, bool bCalcHudID) const;
    int GetLightZoneState(char* pszHueID, int iZone) const { return GetLightZoneState(pszHueID, iZone, true); }
    void SetLightZoneState(char* pszHueID, int iZone, int iNew, bool bEpprom);
    void SetLightZoneState(char* pszHueID, int iZone, int iNew) { SetLightZoneState(pszHueID, iZone, iNew, true); }
    int GetLightColCorRed(char* pszHueID, bool bCalcHudID) const;
    int GetLightColCorRed(char* pszHueID) const { return GetLightColCorRed(pszHueID, true); }
    void SetLightColCorRed(char* pszHueID, int iNew, bool bEpprom);
    void SetLightColCorRed(char* pszHueID, int iNew) { SetLightColCorRed(pszHueID, iNew, true); }
    int GetLightColCorGreen(char* pszHueID, bool bCalcHudID) const;
    int GetLightColCorGreen(char* pszHueID) const { return GetLightColCorGreen(pszHueID, true); }
    void SetLightColCorGreen(char* pszHueID, int iNew, bool bEpprom);
    void SetLightColCorGreen(char* pszHueID, int iNew) { SetLightColCorGreen(pszHueID, iNew, true); }
    int GetLightColCorBlue(char* pszHueID, bool bCalcHudID) const;
    int GetLightColCorBlue(char* pszHueID) const { return GetLightColCorBlue(pszHueID, true); }
    void SetLightColCorBlue(char* pszHueID, int iNew, bool bEpprom);
    void SetLightColCorBlue(char* pszHueID, int iNew) { SetLightColCorBlue(pszHueID, iNew, true); }
    int GetLightColBrightness(char* pszHueID, bool bCalcHudID) const;
    int GetLightColBrightness(char* pszHueID) const { return GetLightColBrightness(pszHueID, true); }
    void SetLightColBrightness(char* pszHueID, int iNew, bool bEpprom);
    void SetLightColBrightness(char* pszHueID, int iNew) { SetLightColBrightness(pszHueID, iNew, true); }
    static void CreateHueIDWithNum(char* pszHueID, char* pszBuf, size_t stBufLen, int iNum);
    static void CreateHueIDWithStr(char* pszHueID, char* pszBuf, size_t stBufLen, const char* pszExtra);

private:
    // Declare private helper variables
    bool m_bHueConnected; 
    bool m_bNewLedData;
    bool m_bUpdateHueConfig;
    bool m_bTaskRunning; 
    int m_iTaskMode; 

    // Hue Options
    bool m_bHueUseDtls;

    // Bridge/config state (moved from global)
    byte m_bHueSupport;
    byte m_nFrameLengthDtls;
    byte m_nHandshakeTimeout;
    char m_huebridgeip[HUE_STR_IP_MAX];
    char m_huebridgeuser[HUE_BRIDGE_USER_MAX];
    char m_huebridgeclientkey[HUE_BRIDGE_USER_MAX];
    int m_iHuePreset;

    // Helper variables
    bool m_bHueDtlsPrepared; 
    bool m_bHueDtlsConnected; 
    byte m_bCurrentFps;           // Save current FPS 
    byte m_bCurrentFramesCtr;     // Counter for current FPS
    byte m_bCurrentFpsSkip;       // Save Frame Skip FPS
    byte m_bCurrentFramesSkipCtr; // Counter for Skip FPS
    
    
};

# endif
#endif 
