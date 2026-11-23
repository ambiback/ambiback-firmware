#ifndef ledhelper_h
#define ledhelper_h

#include <stdint.h>
#include "sectorgeometry.h"

/* LEDHelper: LED strip control (FastLED, modes, brightness, color processing) and cam-to-LED mapping
   only. Camera image -> sector colors is in CamProcessor (global::m_pCamProcessor). */

// Define the color order, can be different from stripe to stripe!
#define COLOR_ORDER RGB

/* LED strip direction (setup). Used by cam-to-LED mapping and setup. */
#define Right_Cclock_bottom_right_up 0x01
#define Top_Cclock_top_right_left 0x02
#define Left_Cclock_top_left_down 0x03
#define Bottom_Cclock_bottom_left_right 0x04
#define Right_Clock_top_right_down 0x05
#define Top_Clock_top_left_right 0x06
#define Left_Clock_bottom_left_up 0x07
#define Bottom_Clock_bottom_right_left 0x08

#if defined(ENABLE_RELAY)
# include <WiFi.h>
#endif

#define LED_DEMO_DEFAULT    0

class LEDHelper {
public:
    
    // Constructors
    LEDHelper();

    // Declare helper functions
    void InitLeds();
    inline unsigned long GetLastUpdate() { return m_lLastUpdate; }
    inline void SetLastUpdate() { m_lLastUpdate = millis(); }

    void LedDemoMode(byte bLeds, byte bLedMode, unsigned long lTimeDuration);

    unsigned long GetVideoTimeout() { return m_lVideoTimeout; }
    void SetVideoTimeout(unsigned long lNew) { m_lVideoTimeout = lNew; }
    inline byte GetCurrentFps() { return m_bCurrentFps; }
    inline void SetCurrentFps(byte bNew) { m_bCurrentFps = bNew; }
    inline byte GetCurrentFramesCtr() { return m_bCurrentFramesCtr; }
    inline void SetCurrentFramesCtr(byte bNew) { m_bCurrentFramesCtr = bNew; }
    inline void IncrementCurrentFramesCtr() { m_bCurrentFramesCtr++; }
    void SetLedColorByIdx(int iIndex, byte bWhite, byte bRed, byte bGreen, byte bBlue); 
    void ZeroConfiguredLeds(); 
    void SetCurrentLedCount(uint16_t count);
    void UpdateCurrentLedCount(); 
    void ResetLedColors(); 
    void ResetNextLedColors(); 
    void ShowLedColors(); 
    
    void PreProcessLedColors(void* pLeds);
    void PreProcessColorBoost(int iIndex, int* piRed, int* piGreen, int* piBlue);
    void PreProcessValueGain(int iIndex, int* piRed, int* piGreen, int* piBlue); 
    void PreProcessMinLuminosity(int iIndex, int* piRed, int* piGreen, int* piBlue); 
    void PreProcessFrameSmoothing(int iIndex, int* piRed, int* piGreen, int* piBlue, void* pVoidLedFrames);
    void PreProcessLowColorSmoothing(int iIndex, byte bLowColorSmoothing, int* piRed, int* piGreen, int* piBlue); 

    void PostProcessFrameSmoothing(void* pVoidLedFrames, void* pVoidLedsNext, int numLeds); 
    void PostProcessLedColors(void* pLeds);
    void ProcessLedNextStep();
    void FadeOut(int iFadeOutLength);
    void EnableTaskFadeOut();
#if defined(ESP32)
    void EnableTaskProcessNextStep();
#endif
    void HandleLeds(); 
    
    inline bool GetIsNewLedData() { return m_bNewLedData; }
    inline void SetIsNewLedData(bool bNew) { m_bNewLedData = bNew; }
    inline bool GetShowLedActive() { return m_bShowLedActive; }
    inline void SetShowLedActive(bool bNew) { m_bShowLedActive = bNew; }

    /* Get/Set Functions */
    inline byte GetLedMode() { return m_bLedMode; }
    
    inline const char* GetLedModeStr() { return GetLedModeStr(m_bLedMode); }
    const char* GetLedModeStr(byte bLedMode);
    bool SetLedMode(byte bNew); 
    inline byte GetChangeLedMode() { return m_bNewLedMode; }
    inline void SetChangeLedMode(byte bNewMode) {m_bNewLedMode = bNewMode; m_bChangeLedMode = true; };

    /* Get/Set configureable properties */
    inline byte GetFrameLength() { return m_bFrameLength; }
    inline int GetDelayLength() { return m_iDelayLength; }
    void SetFrameLength(byte bNew, bool bEpprom); 
    void SetFrameLength(byte bNew) { this->SetFrameLength(bNew, true); }
    void SetDelayLength(int iNew, bool bEpprom); 
    void SetDelayLength(int iNew) { this->SetDelayLength(iNew, true); }

    inline byte GetFrameSmoothing() { return m_bFrameSmoothing; }
    void SetFrameSmoothing(byte bNew, bool bEpprom); 
    void SetFrameSmoothing(byte bNew) { this->SetFrameSmoothing(bNew, true); }
    void* GetVoidLedFramesPtr();

    /* Get/Set Color properties */
    inline byte GetColorCorRed() { return m_bColorCorRed; }
    inline byte GetColorCorGreen() { return m_bColorCorGreen; }
    inline byte GetColorCorBlue() { return m_bColorCorBlue; }
    inline byte GetStoredAmbientRed() { return m_bRed; }
    inline byte GetStoredAmbientGreen() { return m_bGreen; }
    inline byte GetStoredAmbientBlue() { return m_bBlue; }
    inline int GetBrightness() { return m_iBrightness; }
    inline byte GetMinimumLuminosity() { return  m_bMinimumLuminosity; }
    inline byte GetColorBoost() { return  m_bColorBoost; }
    inline byte GetColorBoostBalance() { return  m_bColorBoostBalance; }
    inline byte GetColorBoostRed() { return  m_bColorBoostRed; }
    inline byte GetColorBoostGreen() { return  m_bColorBoostGreen; }
    inline byte GetColorBoostBlue() { return  m_bColorBoostBlue; }
    byte GetColorBoostMin() { return m_bColorBoostMin; };
    byte GetColorBoostThreshold() { return m_bColorBoostThreshold; };
    inline int GetColorValueGain() { return m_iColorValueGain; }
    
    void SetColorCorRed(byte bNew, bool bEpprom);
    void SetColorCorRed(byte bNew) { this->SetColorCorRed(bNew, true); }
    void SetColorCorGreen(byte bNew, bool bEpprom);
    void SetColorCorGreen(byte bNew) { this->SetColorCorGreen(bNew, true); }
    void SetColorCorBlue(byte bNew, bool bEpprom);
    void SetColorCorBlue(byte bNew) { this->SetColorCorBlue(bNew, true); }
    void SetBrightness(int iNew, bool bEpprom);
    void SetBrightness(int iNew) { this->SetBrightness(iNew, true); }
    void SetMinimumLuminosity(byte bNew, bool bEpprom);
    void SetMinimumLuminosity(byte bNew) { this->SetMinimumLuminosity(bNew, true); }
    void SetColorBoost(byte bNew, bool bEpprom);
    void SetColorBoost(byte bNew) { this->SetColorBoost(bNew, true); }
    void SetColorBoostBalance(byte bNew, bool bEpprom);
    void SetColorBoostBalance(byte bNew) { this->SetColorBoostBalance(bNew, true); }
    void SetColorBoostMin(byte bNew, bool bEpprom); 
    void SetColorBoostMin(byte bNew) { this->SetColorBoostMin(bNew, true); }
    void SetColorBoostRed(byte bNew, bool bEpprom);
    void SetColorBoostRed(byte bNew) { this->SetColorBoostRed(bNew, true); }
    void SetColorBoostGreen(byte bNew, bool bEpprom);
    void SetColorBoostGreen(byte bNew) { this->SetColorBoostGreen(bNew, true); }
    void SetColorBoostBlue(byte bNew, bool bEpprom);
    void SetColorBoostBlue(byte bNew) { this->SetColorBoostBlue(bNew, true); }
    void SetColorBoostThreshold(byte bNew, bool bEpprom);
    void SetColorBoostThreshold(byte bNew) { this->SetColorBoostThreshold(bNew, true); }
    void SetColorValueGain(int iNew, bool bEpprom);
    void SetColorValueGain(int iNew) { this->SetColorValueGain(iNew, true); }

    /* Get/Set Setup properties */
    byte GetSetupVertical() { return m_bSetupVertical;}
    byte GetSetupHorizontal() { return m_bSetupHorizontal;}
    byte GetSetupDirection() { return m_bSetupDirection;}
    byte GetSetupSideLeft() { return m_bSetupSideLeft;}
    byte GetSetupSideTop() { return m_bSetupSideTop;}
    byte GetSetupSideRight() { return m_bSetupSideRight;}
    byte GetSetupSideBottom() { return m_bSetupSideBottom;}
    byte GetSideLeftBrightness() { return m_bSideLeftBrightness;}
    byte GetSideTopBrightness() { return m_bSideTopBrightness;}
    byte GetSideRightBrightness() { return m_bSideRightBrightness;}
    byte GetSideBottomBrightness() { return m_bSideBottomBrightness;}
    
    void SetSetupVertical(byte bNew, bool bEpprom);
    void SetSetupVertical(byte bNew) { this->SetSetupVertical(bNew, true); }
    void SetSetupHorizontal(byte bNew, bool bEpprom);
    void SetSetupHorizontal(byte bNew) { this->SetSetupHorizontal(bNew, true); }
    void SetSetupDirection(byte bNew, bool bEpprom);
    void SetSetupDirection(byte bNew) { this->SetSetupDirection(bNew, true); }
    void SetSetupSideLeft(byte bNew, bool bEpprom);
    void SetSetupSideLeft(byte bNew) { this->SetSetupSideLeft(bNew, true); }
    void SetSetupSideTop(byte bNew, bool bEpprom);
    void SetSetupSideTop(byte bNew) { this->SetSetupSideTop(bNew, true); }
    void SetSetupSideRight(byte bNew, bool bEpprom);
    void SetSetupSideRight(byte bNew) { this->SetSetupSideRight(bNew, true); }
    void SetSetupSideBottom(byte bNew, bool bEpprom);
    void SetSetupSideBottom(byte bNew) { this->SetSetupSideBottom(bNew, true); }

    void SetSideLeftBrightness(byte bNew, bool bEpprom);
    void SetSideLeftBrightness(byte bNew) { this->SetSideLeftBrightness(bNew, true); }
    void SetSideTopBrightness(byte bNew, bool bEpprom);
    void SetSideTopBrightness(byte bNew) { this->SetSideTopBrightness(bNew, true); }
    void SetSideRightBrightness(byte bNew, bool bEpprom);
    void SetSideRightBrightness(byte bNew) { this->SetSideRightBrightness(bNew, true); }
    void SetSideBottomBrightness(byte bNew, bool bEpprom);
    void SetSideBottomBrightness(byte bNew) { this->SetSideBottomBrightness(bNew, true); }

    /* LED Functions */    
    void colorBlink(bool bRed, bool bGreen, bool bBlue, byte bSteps, bool bOff); 
    void colorBlink(byte bRed, byte bGreen, byte bBlue, byte bSteps, bool bOff); 
    void colorBlink(int iControllerIndex, int iBrightNess, byte bWhite, byte bRed, byte bGreen, byte bBlue, byte bSteps); 
    void colorBlinkUp(int iControllerIndex, int iBrightNess, byte bWhite, byte bRed, byte bGreen, byte bBlue, byte bSteps); 
    void colorBlinkDown(int iControllerIndex, int iBrightNess, byte bWhite, byte bRed, byte bGreen, byte bBlue, byte bSteps); 
    void bootColorChase(byte bRed, byte bGreen, byte bBlue, byte bLedsPerStep, unsigned long lStepDelayMs, bool bOffAfter);
    void setFullColor(int iColorCode); 
    void setFullColor(byte bWhite, byte bRed, byte bGreen, byte bBlue); 
    void setFullColor(int iControllerIndex, int iBrightNess, byte bWhite, byte bRed, byte bGreen, byte bBlue); 
    void effectFadeToBlack(int iControllerIndex, int iBrightNess); 
    void toggleLed(int iControllerIndex, int iBrightNess, byte bWhite, byte bRed, byte bGreen, byte bBlue, byte bSteps, bool bLedState);

    void processIRCommand(unsigned long uilastir); 
    bool getPowerLedOn() { return m_bPowerLedOn; } 
    bool getControlLedOn() { return m_bControlLedOn; } 
    void setPowerLedOn(bool bNew) { m_bPowerLedOn = bNew; } 
    void setControlLedOn(bool bNew) { m_bControlLedOn = bNew; } 
    uint16_t getNumLeds() { return (uint16_t)(m_numleds + 1); } 
    bool getProcessingLedNextStep() { return m_bProcessingLedNextStep; }
    void setProcessingLedNextStep(bool bNew) { m_bProcessingLedNextStep = bNew; } 
    bool getFadeOutRunning() { return m_bFadeOutRunning; }
    void setFadeOutRunning(bool bNew) { m_bFadeOutRunning = bNew; } 
    bool getBlinkTaskRunning() { return m_bBlinkTaskRunning; }
    void setBlinkTaskRunning(bool bNew) { m_bBlinkTaskRunning = bNew; } 

    // Sequence Number handling
    inline unsigned short GetSequenceNumber(void) { return m_shSequence; };
    inline void IncrementSequence() { m_shSequence++; }
    inline void SetSequenceNumber(unsigned short shNew) { m_shSequence = shNew; }

#if defined(ESP32)
    // Helper to delete Cam Task
    void deleteTaskCam(); 
#endif

#if defined(ENABLE_CAM)
    // Cam2LED mapping (uses global::m_pCamProcessor for GetCamMode/GetCamDetailLevel)
    bool SetLedBufferFromCamSectors(void* pVoidCamleds);
    bool camCalcNextLedStep(void* pVoidCamleds); 
    void camCalcNextLedStepLeft(void* pVoidCamleds, int* piLast, bool bReverse, bool bLast);
    void camCalcNextLedStepTop(void* pVoidCamleds, int* piLast, bool bReverse, bool bLast);
    void camCalcNextLedStepRight(void* pVoidCamleds, int* piLast, bool bReverse, bool bLast);
    void camCalcNextLedStepBottom(void* pVoidCamleds, int* piLast, bool bReverse, bool bLast);
#endif

#if defined(ENABLE_HUE)
    void hueCalcSectorsLeft(void* pLeds, void* pVoidLedsHue, int iStartIdx, int iEndIdx, bool bReverse, bool bFirstTop, bool bFirstBottom);
    void hueCalcSectorsTop(void* pLeds, void* pVoidLedsHue, int iStartIdx, int iEndIdx, bool bReverse, bool bFirstLeft, bool bFirstRight);
    void hueCalcSectorsRight(void* pLeds, void* pVoidLedsHue, int iStartIdx, int iEndIdx, bool bReverse, bool bFirstTop, bool bFirstBottom);
    void hueCalcSectorsBottom(void* pLeds, void* pVoidLedsHue, int iStartIdx, int iEndIdx, bool bReverse, bool bFirstLeft, bool bFirstRight);
#endif

#if defined(ENABLE_RELAY)
    size_t CreateLedNetPacket(byte* pbBuf, size_t stBufLen);
    void RelayLedData(void);
    void RelayPrepareDirectLedData(void);
    bool RelayConnect(void);
    void RelayDisconnect(void);
    inline bool GetRelayClientConnected() { return m_bClientConnected; }
#endif
    // Helper functions to set LEDs for a single side to BLACK
    void handleSideDisableVertical(void* pVoidleds, int* piLast);
    void handleSideDisableHorizontal(void* pVoidleds, int* piLast);
    // Helper functions to calc LED side brightness
    void handleSideBrightnessVertical(void* pVoidleds, int* piLast, byte bBrightnessLevel);
    void handleSideBrightnessHorizontal(void* pVoidleds, int* piLast, byte bBrightnessLevel);

private:
#if defined(ENABLE_RELAY)
    WiFiClient* pTcpClient;
    bool m_bClientConnected;
#endif
    // Declare helper variables
    byte m_bRed;
    byte m_bGreen;
    byte m_bBlue;
    byte m_bRedStep;
    byte m_bGreenStep;
    byte m_bBlueStep;

    // Stored as count-1 (0=1 LED .. 255=256 LEDs). Decode with getNumLeds() = m_numleds+1.
    byte m_numleds;
    bool m_bNewLedData;
    bool m_bShowLedActive; 
    
    bool m_bPowerLedOn; 
    bool m_bControlLedOn; 

  /* Possible LED Modes right now are: 
   *  0 = OFF
   *  1 = VIDEO
   *  2 = AMBIENT
   *  3 = CAMERA
   *  4 = ...
   *  m_bLedMode
   */
    byte m_bLedMode;
    bool m_bChangeLedMode;
    byte m_bNewLedMode;

    // Helpers to handle Task Loops
    bool m_bProcessingLedNextStep; 
    bool m_bFadeOutRunning; 
    bool m_bBlinkTaskRunning; 

    // Color related 
    byte m_bColorCorRed;
    byte m_bColorCorGreen;
    byte m_bColorCorBlue;
    int m_iBrightness; 
    byte m_bMinimumLuminosity; 
    byte m_bColorBoost;       // Colorboost Value!
    byte m_bColorBoostBalance;// Colorboost Balance!
    byte m_bColorBoostMin;    // Colorboost Smoothing!
    byte m_bColorBoostRed;
    byte m_bColorBoostGreen;
    byte m_bColorBoostBlue;
    byte m_bColorBoostThreshold;
    int m_iColorValueGain;

    // Other Options
    byte m_bFrameLength; 
    int m_iDelayLength;
    byte m_bFrameSmoothing; 

    // LED Setup Options
    byte m_bSetupVertical;
    byte m_bSetupHorizontal;
    byte m_bSetupDirection;
    byte m_bSetupSideLeft;
    byte m_bSetupSideTop;
    byte m_bSetupSideRight;
    byte m_bSetupSideBottom;
    byte m_bSideLeftBrightness;
    byte m_bSideTopBrightness;
    byte m_bSideRightBrightness;  
    byte m_bSideBottomBrightness;

    unsigned long m_lLastUpdate;
    unsigned long m_lVideoTimeout;// When to turn off Lights after no new data received in VIDEO Mode
    byte m_bCurrentFps;           // Save current FPS 
    byte m_bCurrentFramesCtr;     // Counter for current FPS

    /* Extra helper value to implemented basic Frame sequence handling
    * 0 means sequence handling is disabled
    * 1-65535 is a valid sequence
     */
    unsigned short m_shSequence;

};

#endif
