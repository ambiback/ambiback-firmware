#include "globalincludes.h"
#include "global.h"
#if defined(ENABLE_CAM)
#include "camprocessor.h"
#endif

void global::SetMotionPreset(int iIndex, bool bEpprom) {
  global::m_iMotionPreset = iIndex;
  // Write to EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingInt("MotionPreset", iIndex);

  // Set ColorBoost Based on Presets
  if (iIndex == 1 /* enummotionpresets.direct */) {
    global::m_pLedHelper->SetFrameSmoothing(0, false);
    global::m_pLedHelper->SetFrameLength(DEFAULT_FRAME_LENGTH, false);
    global::m_pLedHelper->SetDelayLength(DEFAULT_FRAME_LENGTH, false);
  } else if (iIndex == 2 /* enummotionpresets.normal */) {
    global::m_pLedHelper->SetFrameSmoothing(1, false);
    global::m_pLedHelper->SetFrameLength(DEFAULT_FRAME_LENGTH, false);
    global::m_pLedHelper->SetDelayLength(DEFAULT_DELAY_LENGTH, false);
  } else if (iIndex == 3 /* enummotionpresets.smooth */) {
    global::m_pLedHelper->SetFrameSmoothing(2, false);
    global::m_pLedHelper->SetFrameLength(DEFAULT_FRAME_LENGTH, false);
    global::m_pLedHelper->SetDelayLength(DEFAULT_DELAY_LENGTH + DEFAULT_FRAME_LENGTH, false);
  } else if (iIndex == 4 /* enummotionpresets.extrasmooth */) {
    global::m_pLedHelper->SetFrameSmoothing(2, false);
    global::m_pLedHelper->SetFrameLength(DEFAULT_FRAME_LENGTH, false);
    global::m_pLedHelper->SetDelayLength(DEFAULT_DELAY_LENGTH + (DEFAULT_FRAME_LENGTH * 2), false);
  } else if (iIndex == 5 /* enummotionpresets.extremesmooth */) {
    global::m_pLedHelper->SetFrameSmoothing(3, false);
    global::m_pLedHelper->SetFrameLength(DEFAULT_FRAME_LENGTH, false);
    global::m_pLedHelper->SetDelayLength(DEFAULT_DELAY_LENGTH + (DEFAULT_FRAME_LENGTH * 3), false);
  } else {
    // Load from store EEPROM
    global::m_pLedHelper->SetFrameSmoothing(global::m_pEepromHelper->GetSettingByte("FrameSmoothing", DEFAULT_FRAME_SMOOTHING), false);
    global::m_pLedHelper->SetFrameLength(global::m_pEepromHelper->GetSettingByte("FrameLength", DEFAULT_FRAME_LENGTH), false);
    global::m_pLedHelper->SetDelayLength(global::m_pEepromHelper->GetSettingInt("DelayLength", DEFAULT_DELAY_LENGTH), false);
  }

  _DBGPRINTF(PSTR("global:SetMotionPreset to %d\n"), global::m_iMotionPreset);
}
void global::SetBrightnessPreset(int iIndex, bool bEpprom) {
  global::m_iBrightnessPreset = iIndex;
  // Write to EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingInt("BrightnsPreset", iIndex);

  // Set ColorBoost Based on Presets
  if (iIndex == 1 /* enumbrightnesspresets.low */) {
    global::m_pLedHelper->SetBrightness(255, false);
    global::m_pLedHelper->SetColorValueGain(50, false);
  } else if (iIndex == 2 /* enumbrightnesspresets.medium */) {
    global::m_pLedHelper->SetBrightness(255, false);
    global::m_pLedHelper->SetColorValueGain(75, false);
  } else if (iIndex == 3 /* enumbrightnesspresets.normal */) {
    global::m_pLedHelper->SetBrightness(255, false);
    global::m_pLedHelper->SetColorValueGain(100, false);
  } else if (iIndex == 4 /* enumbrightnesspresets.high */) {
    global::m_pLedHelper->SetBrightness(255, false);
    global::m_pLedHelper->SetColorValueGain(150, false);
  } else if (iIndex == 5 /* enumbrightnesspresets.extrahigh */) {
    global::m_pLedHelper->SetBrightness(255, false);
    global::m_pLedHelper->SetColorValueGain(200, false);
  } else if (iIndex == 6 /* enumbrightnesspresets.extremehigh */) {
    global::m_pLedHelper->SetBrightness(255, false);
    global::m_pLedHelper->SetColorValueGain(500, false);
  } else {
    // Load from store EEPROM
    global::m_pLedHelper->SetBrightness(global::m_pEepromHelper->GetSettingByte("Brightness", 255), false);
    global::m_pLedHelper->SetColorValueGain(global::m_pEepromHelper->GetSettingByte("ColorValueGain", 100), false);
  }

  _DBGPRINTF(PSTR("global:SetBrightnessPreset to %d\n"), global::m_iBrightnessPreset);
}
void global::SetColorPreset(int iIndex, bool bEpprom) {
  global::m_iColorPreset = iIndex;
  // Write to EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingInt("ColorPreset", iIndex);

  // Set ColorBoost Based on Presets
  if (iIndex == 1 /* enumcolorpresets.normal */) {
    global::m_pLedHelper->SetColorBoost(0, false);
    global::m_pLedHelper->SetColorBoostMin(0, false);
    global::m_pLedHelper->SetColorBoostThreshold(0, false);
  } else if (iIndex == 2 /* enumcolorpresets.colorful */) {
    global::m_pLedHelper->SetColorBoost(20, false);
    global::m_pLedHelper->SetColorBoostMin(12, false);
    global::m_pLedHelper->SetColorBoostThreshold(8, false);
  } else if (iIndex == 3 /* enumcolorpresets.extracolorful */) {
    global::m_pLedHelper->SetColorBoost(50, false);
    global::m_pLedHelper->SetColorBoostMin(16, false);
    global::m_pLedHelper->SetColorBoostThreshold(8, false);
  } else if (iIndex == 4 /* enumcolorpresets.extremecolorful */) {
    global::m_pLedHelper->SetColorBoost(100, false);
    global::m_pLedHelper->SetColorBoostMin(16, false);
    global::m_pLedHelper->SetColorBoostThreshold(8, false);
  } else {
    // Load from store EEPROM
    global::m_pLedHelper->SetColorBoost(global::m_pEepromHelper->GetSettingByte("ColorBoost", 0), false);
    global::m_pLedHelper->SetColorBoostMin(global::m_pEepromHelper->GetSettingByte("ColorBoostMin", 0), false);
    global::m_pLedHelper->SetColorBoostThreshold(global::m_pEepromHelper->GetSettingByte("ColorBoostThres", 0), false);
  }

  _DBGPRINTF(PSTR("global:SetColorPreset to %d\n"), global::m_iColorPreset);
}

void global::SetAmbientPreset(int iIndex, bool bEpprom) {
  if (iIndex < 0) iIndex = 0;
  if (iIndex > 15) iIndex = 15;
  global::m_iAmbientPreset = iIndex;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingInt("AmbientPreset", iIndex);
  _DBGPRINTF(PSTR("global:SetAmbientPreset to %d\n"), global::m_iAmbientPreset);
}

void global::SetAmbientSpeed(int iNew, bool bEpprom) {
  if (iNew < 1) iNew = 1;
  if (iNew > 10) iNew = 10;
  global::m_iAmbientSpeed = iNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingInt("AmbientSpeed", iNew);
  _DBGPRINTF_ULTRA(PSTR("global:SetAmbientSpeed to %d\n"), global::m_iAmbientSpeed);
}

void global::SetAmbientIntensity(int iNew, bool bEpprom) {
  if (iNew < 1) iNew = 1;
  if (iNew > 100) iNew = 100;
  global::m_iAmbientIntensity = iNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingInt("AmbientIntensity", iNew);
  _DBGPRINTF_ULTRA(PSTR("global:SetAmbientIntensity to %d\n"), global::m_iAmbientIntensity);
}

void global::SetAmbientHealthIntensity(int iNew, bool bEpprom) {
  if (iNew < 1) iNew = 1;
  if (iNew > 100) iNew = 100;
  global::m_iAmbientHealthIntensity = iNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingInt("AmbientHIntens", iNew);
  _DBGPRINTF_ULTRA(PSTR("global:SetAmbientHealthIntensity to %d\n"), global::m_iAmbientHealthIntensity);
}

#if defined(ENABLE_CAM)
void global::SetExposurePreset(int iIndex, void* voidresponse, bool bEpprom, bool bReadOnly) {
  global::m_iExposurePreset = iIndex;
  // Write to EEPROM
  if (bEpprom) {
    // Save custom Exposure Preset
    if (global::m_iCamPreset != CAM_custom) {
      char szSettingID[ESP32_PREF_KEYMAX];
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamExpPreset%d"), global::m_iCamPreset);
      global::m_pEepromHelper->SetSettingInt(&szSettingID[0], iIndex);
    } else {
      global::m_pEepromHelper->SetSettingInt("CamExpPreset", iIndex);
    }
  }

  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetExposurePreset:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }

  // Cast Response Pointer if not NULL
  AsyncResponseStream* response = (voidresponse != NULL ? (AsyncResponseStream*) voidresponse : NULL);

  // Custom Var Helper
  char szSettingID[ESP32_PREF_KEYMAX];

  // Set default (custom) values
  int iCamAec = 0;
  int iCamAec2 = 0;
  int iCamAelevels = -2;
  int iCamAecValue = 240;
  int iCamGain = 0;
  int iCamAgcGain = 1;
  int iCamRawGMA = 0;
  int iCamLenc = 1;

  int iCamAdaptiveExposurecontrol = 0;
  int iCamAecMinValue = CAM_ADPT_AEC_MIN;
  int iCamAecMaxValue = CAM_ADPT_AEC_MAX;
  int iCamAdaptiveChangePercent = CAM_WHITE_AEC_CHANGEPERC;
  int iCamAdaptiveChangeDelay = CAM_ADPT_CHANGE_DELAY;
  byte bCamWhiteActivationTreshold = WHITEACTTHRESHOLD_DEFAULT;
  byte bCamBeforeWhiteActivationTreshold = CAM_ADPT_BEFORE_ACT;
  byte bCamWhiteMinimumAmount = CAM_ADPT_WHITE_MIN;

  int iCamAdaptiveGaincontrol = 0;
  byte bCamAdaptiveGainDown = CAM_ADPT_GAIN_DOWN;
  byte bCamAdaptiveGainUp = CAM_ADPT_GAIN_UP;
  byte bCamAdaptiveGainMax = CAM_ADPT_GAIN_MAX;

  // 1. set default values!
  if (iIndex == CAM_exp_normal /* enumexposurepresets.normal */) {
    iCamAec = 0;            // set_exposure_ctrl |  Auto exposure off (1 or 0)
    iCamAec2 = 0;           // set_aec2 | Auto exposure sensor? (0 or 1)
    iCamAecValue = 216;     // set_aec_value | Auto Exposure Value
    iCamAelevels = -2;      // set_ae_level |
    iCamGain = 0;           // set_gain_ctrl | Enable Auto Gain Control
    iCamAgcGain = 1;        // set_agc_gain | Manual Gain Control Level
    iCamRawGMA = 1;         // set_raw_gma
    iCamLenc = 0;           // set_lenc
  } else if (iIndex == CAM_exp_bright /* enumexposurepresets.bright */) {
    iCamAec = 0;            // set_exposure_ctrl |  Auto exposure off (1 or 0)
    iCamAec2 = 0;           // set_aec2 | Auto exposure sensor? (0 or 1)
    iCamAecValue = 300;     // set_aec_value | Auto Exposure Value
    iCamAelevels = -2;      // set_ae_level |
    iCamGain = 0;           // set_gain_ctrl | Enable Auto Gain Control
    iCamAgcGain = 1;        // set_agc_gain | Manual Gain Control Level
    iCamRawGMA = 1;         // set_raw_gma
    iCamLenc = 0;           // set_lenc
  } else if (iIndex == CAM_exp_verybright /* enumexposurepresets.verybright */) {
    iCamAec = 0;            // set_exposure_ctrl |  Auto exposure off (1 or 0)
    iCamAec2 = 0;           // set_aec2 | Auto exposure sensor? (0 or 1)
    iCamAecValue = 480;     // set_aec_value | Auto Exposure Value
    iCamAelevels = -2;      // set_ae_level |
    iCamGain = 0;           // set_gain_ctrl | Enable Auto Gain Control
    iCamAgcGain = 1;        // set_agc_gain | Manual Gain Control Level
    iCamRawGMA = 1;         // set_raw_gma
    iCamLenc = 0;           // set_lenc
  } else if (iIndex == CAM_exp_autocontrol /* enumexposurepresets.auto */) {
    iCamAec = 1;            // set_exposure_ctrl |  Auto exposure off (1 or 0)
    iCamAec2 = 1;           // set_aec2 | Auto exposure sensor? (0 or 1)
    iCamAecValue = 1200;    // set_aec_value | Auto Exposure Value
    iCamAelevels = 0;       // set_ae_level |
    iCamGain = 1;           // set_gain_ctrl | Enable Auto Gain Control
    iCamAgcGain = 0;        // set_agc_gain | Manual Gain Control Level
    iCamRawGMA = 1;         // set_raw_gma
    iCamLenc = 0;           // set_lenc
  } else if (iIndex == CAM_exp_adaptiveraw /* enumexposurepresets.adaptiveraw */) {
    iCamAec = 0;            // set_exposure_ctrl |  Auto exposure off (1 or 0)
    iCamAec2 = 0;           // set_aec2 | Auto exposure sensor? (0 or 1)
    iCamAecValue = CAM_ADPT_AEC_MAX;     // set_aec_value | Auto Exposure Value
    iCamAelevels = 0;       // set_ae_level |
    iCamGain = 0;           // set_gain_ctrl | Enable Auto Gain Control
    iCamAgcGain = 2;        // set_agc_gain | Manual Gain Control Level
    iCamRawGMA = 1;         // set_raw_gma
    iCamLenc = 0;           // set_lenc

    iCamAdaptiveExposurecontrol = 1;                          // Enable adaptive exposure control
    iCamAecMinValue = CAM_ADPT_AEC_MIN;                       // Minimum AEC Value to set
    iCamAecMaxValue = CAM_ADPT_AEC_MAX;                       // Maximum AEC Value to set
    iCamAdaptiveChangePercent = CAM_WHITE_AEC_CHANGEPERC;     // Default steps when changing AEC Value
    iCamAdaptiveChangeDelay = CAM_ADPT_CHANGE_DELAY;          // Wait delay before going UP/DOWN with AEC
    bCamWhiteActivationTreshold = WHITEACTTHRESHOLD_DEFAULT;  // HIGH ColorLight Threshold Value that triggers AEC DOWNSCALE
    bCamBeforeWhiteActivationTreshold = CAM_ADPT_BEFORE_ACT;  // LOW ColorLight Threshold Value that triggers AEC UPSCALE
    bCamWhiteMinimumAmount = CAM_ADPT_WHITE_MIN;              // HIGH ColorLight minimum Amount in percent of Sectors that have to be above Threshold value

    iCamAdaptiveGaincontrol = 0;                              // Enable our own auto Gain Control!
    bCamAdaptiveGainDown = CAM_ADPT_GAIN_DOWN;                // Threshold value for outside pixels, if outside pixels fall below this value, Gainlevel is INCREMENED by ONE.
    bCamAdaptiveGainUp = CAM_ADPT_GAIN_UP;                    // Threshold value for inside pixels, if outside pixels fall below this value, Gainlevel is DECREMENTED bny one.
    bCamAdaptiveGainMax = CAM_ADPT_GAIN_MAX;                  // Max Gain Level for adaptive gain control mode
  } else if (iIndex == CAM_exp_adaptivelens /* enumexposurepresets.adaptivelens */) {
    iCamAec = 0;            // set_exposure_ctrl |  Auto exposure off (1 or 0)
    iCamAec2 = 0;           // set_aec2 | Auto exposure sensor? (0 or 1)
    iCamAecValue = CAM_ADPT_AEC_MAX;     // set_aec_value | Auto Exposure Value
    iCamAelevels = -2;      // set_ae_level |
    iCamGain = 0;           // set_gain_ctrl | Enable Auto Gain Control
    iCamAgcGain = 2;        // set_agc_gain | Manual Gain Control Level
    iCamRawGMA = 0;         // set_raw_gma
    iCamLenc = 1;           // set_lenc

    iCamAdaptiveExposurecontrol = 1;                              // Enable adaptive exposure control
    iCamAecMinValue = CAM_ADPT_AEC_MIN;                           // Minimum AEC Value to set
    iCamAecMaxValue = CAM_ADPT_AEC_MAX;                           // Maximum AEC Value to set
    iCamAdaptiveChangePercent = CAM_WHITE_AEC_CHANGEPERC;         // Default steps when changing AEC Value
    iCamAdaptiveChangeDelay = CAM_ADPT_CHANGE_DELAY;              // Wait delay before going UP/DOWN with AEC
    bCamWhiteActivationTreshold = WHITEACTTHRESHOLD_DEFAULT;      // HIGH ColorLight Threshold Value that triggers AEC DOWNSCALE
    bCamBeforeWhiteActivationTreshold = CAM_ADPT_BEFORE_ACT;      // LOW ColorLight Threshold Value that triggers AEC UPSCALE
    bCamWhiteMinimumAmount = CAM_ADPT_WHITE_MIN;                  // HIGH ColorLight minimum Amount in percent of Sectors that have to be above Threshold value

    iCamAdaptiveGaincontrol = 0;                                  // Enable our own auto Gain Control!
    bCamAdaptiveGainDown = CAM_ADPT_GAIN_DOWN;                    // Threshold value for outside pixels, if outside pixels fall below this value, Gainlevel is INCREMENED by ONE.
    bCamAdaptiveGainUp = CAM_ADPT_GAIN_UP;                        // Threshold value for inside pixels, if outside pixels fall below this value, Gainlevel is DECREMENTED bny one.
    bCamAdaptiveGainMax = CAM_ADPT_GAIN_MAX;                      // Max Gain Level for adaptive gain control mode
  } else if (iIndex == CAM_exp_adaptive /* enumexposurepresets.adaptive */) {
    iCamAec = 0;                          // set_exposure_ctrl |  Auto exposure off (1 or 0)
    iCamAec2 = 0;                         // set_aec2 | Auto exposure sensor? (0 or 1)
    iCamAecValue = CAM_ADPT_AEC_MAX;      // set_aec_value | Auto Exposure Value
    iCamAelevels = -2;                    // set_ae_level |
    iCamGain = 0;                         // set_gain_ctrl | Enable Auto Gain Control
    iCamAgcGain = CAM_ADPT_GAIN_DEFAULT;  // set_agc_gain | Manual Gain Control Level
    iCamRawGMA = 0;                       // set_raw_gma
    iCamLenc = 0;                         // set_lenc

    iCamAdaptiveExposurecontrol = 1;                              // Enable adaptive exposure control
    iCamAecMinValue = CAM_ADPT_AEC_MIN;                           // Minimum AEC Value to set
    iCamAecMaxValue = CAM_ADPT_AEC_MAX;                           // Maximum AEC Value to set
    iCamAdaptiveChangePercent = CAM_WHITE_AEC_CHANGEPERC;         // Default steps when changing AEC Value
    iCamAdaptiveChangeDelay = CAM_ADPT_CHANGE_DELAY;              // Wait delay before going UP/DOWN with AEC
    bCamWhiteActivationTreshold = WHITEACTTHRESHOLD_DEFAULT;      // HIGH ColorLight Threshold Value that triggers AEC DOWNSCALE
    bCamBeforeWhiteActivationTreshold = CAM_ADPT_BEFORE_ACT;      // LOW ColorLight Threshold Value that triggers AEC UPSCALE
    bCamWhiteMinimumAmount = CAM_ADPT_WHITE_MIN;                  // HIGH ColorLight minimum Amount in percent of Sectors that have to be above Threshold value

    iCamAdaptiveGaincontrol = 1;                                  // Enable our own auto Gain Control!
    bCamAdaptiveGainDown = CAM_ADPT_GAIN_DOWN;                    // Threshold value for outside pixels, if outside pixels fall below this value, Gainlevel is INCREMENED by ONE.
    bCamAdaptiveGainUp = CAM_ADPT_GAIN_UP;                        // Threshold value for inside pixels, if outside pixels fall below this value, Gainlevel is DECREMENTED bny one.
    bCamAdaptiveGainMax = CAM_ADPT_GAIN_MAX_HIGH;                 // Max Gain Level for adaptive gain control mode
  } else if (iIndex == CAM_exp_adaptivelensraw /* enumexposurepresets.adaptivelensraw */) {
    iCamAec = 0;            // set_exposure_ctrl |  Auto exposure off (1 or 0)
    iCamAec2 = 0;           // set_aec2 | Auto exposure sensor? (0 or 1)
    iCamAecValue = CAM_ADPT_AEC_MAX;     // set_aec_value | Auto Exposure Value
    iCamAelevels = -2;      // set_ae_level |
    iCamGain = 0;           // set_gain_ctrl | Enable Auto Gain Control
    iCamAgcGain = 2;        // set_agc_gain | Manual Gain Control Level
    iCamRawGMA = 1;         // set_raw_gma
    iCamLenc = 1;           // set_lenc

    iCamAdaptiveExposurecontrol = 1;                              // Enable adaptive exposure control
    iCamAecMinValue = CAM_ADPT_AEC_MIN;                           // Minimum AEC Value to set
    iCamAecMaxValue = CAM_ADPT_AEC_MAX;                           // Maximum AEC Value to set
    iCamAdaptiveChangePercent = CAM_WHITE_AEC_CHANGEPERC;         // Default steps when changing AEC Value
    iCamAdaptiveChangeDelay = CAM_ADPT_CHANGE_DELAY;              // Wait delay before going UP/DOWN with AEC
    bCamWhiteActivationTreshold = WHITEACTTHRESHOLD_LOW_DEFAULT;  // HIGH ColorLight Threshold Value that triggers AEC DOWNSCALE
    bCamBeforeWhiteActivationTreshold = CAM_ADPT_BEFORE_ACT;      // LOW ColorLight Threshold Value that triggers AEC UPSCALE
    bCamWhiteMinimumAmount = CAM_ADPT_WHITE_MIN;                  // HIGH ColorLight minimum Amount in percent of Sectors that have to be above Threshold value

    iCamAdaptiveGaincontrol = 0;                                  // Enable our own auto Gain Control!
    bCamAdaptiveGainDown = CAM_ADPT_GAIN_DOWN;                    // Threshold value for outside pixels, if outside pixels fall below this value, Gainlevel is INCREMENED by ONE.
    bCamAdaptiveGainUp = CAM_ADPT_GAIN_UP;                        // Threshold value for inside pixels, if outside pixels fall below this value, Gainlevel is DECREMENTED bny one.
    bCamAdaptiveGainMax = CAM_ADPT_GAIN_MAX_LOW;                  // Max Gain Level for adaptive gain control mode
  }

  // 2. Set webui default variables
  if (response != NULL) {
    response->printf_P(PSTR("window.CamAec = %d;\n"), iCamAec );
    response->printf_P(PSTR("window.CamAec2 = %d;\n"), iCamAec2 );
    response->printf_P(PSTR("window.CamAelevels = %d;\n"), iCamAelevels );
    response->printf_P(PSTR("window.CamAecValue = %d;\n"), iCamAecValue );
    response->printf_P(PSTR("window.CamGain = %d;\n"), iCamGain );
    response->printf_P(PSTR("window.CamAgcGain = %d;\n"), iCamAgcGain );
    response->printf_P(PSTR("window.CamRawGMA = %d;\n"), iCamRawGMA );
    response->printf_P(PSTR("window.CamLenc = %d;\n"), iCamLenc );

    response->printf_P(PSTR("window.CamAdaptiveExposurecontrol = %d;\n"), iCamAdaptiveExposurecontrol );
    response->printf_P(PSTR("window.CamAecMinValue = %d;\n"), iCamAecMinValue );
    response->printf_P(PSTR("window.CamAecMaxValue = %d;\n"), iCamAecMaxValue );
    response->printf_P(PSTR("window.CamAdaptiveChangePercent = %d;\n"), iCamAdaptiveChangePercent );
    response->printf_P(PSTR("window.CamAdaptiveChangeDelay = %d;\n"), iCamAdaptiveChangeDelay );
    response->printf_P(PSTR("window.CamWhiteActivationTreshold = %d;\n"), bCamWhiteActivationTreshold );
    response->printf_P(PSTR("window.CamBeforeWhiteActivationTreshold = %d;\n"), bCamBeforeWhiteActivationTreshold );
    response->printf_P(PSTR("window.CamWhiteMinimumAmount = %d;\n"), bCamWhiteMinimumAmount );

    response->printf_P(PSTR("window.CamAdaptiveGaincontrol = %d;\n"), iCamAdaptiveGaincontrol );
    response->printf_P(PSTR("window.CamAdaptiveGainDown = %d;\n"), bCamAdaptiveGainDown );
    response->printf_P(PSTR("window.CamAdaptiveGainUp = %d;\n"), bCamAdaptiveGainUp );
    response->printf_P(PSTR("window.CamAdaptiveGainMax = %d;\n"), bCamAdaptiveGainMax );

    response->printf_P(PSTR("window.CamHMirror = %d;\n"), global::m_iCamHMirror );
    response->printf_P(PSTR("window.CamVFlip = %d;\n"), global::m_iCamVFlip );
  }

  // 3. Apply values
  if ( bReadOnly == false ) {
    if ( iIndex == CAM_exp_custom) {
      // Load CUSTOM from store EEPROM
      global::SetCamAutoExposurecontrol(global::m_pEepromHelper->GetSettingInt("CamAec", iCamAec), false);
      global::SetCamAec2(global::m_pEepromHelper->GetSettingInt("CamAec2", iCamAec2), false);
      global::SetCamAeLevels(global::m_pEepromHelper->GetSettingInt("CamAelevels", iCamAelevels), false);
      global::SetCamAecValue(global::m_pEepromHelper->GetSettingInt("CamAecValue", iCamAecValue), false);
      global::SetCamGaincontrol(global::m_pEepromHelper->GetSettingInt("CamGain", iCamGain), false);
      global::SetCamAgcGain(global::m_pEepromHelper->GetSettingInt("CamAgcGain", iCamAgcGain), false);
      global::SetCamRawGMA(global::m_pEepromHelper->GetSettingInt("CamRawGMA", iCamRawGMA), false);
      global::SetCamLenc(global::m_pEepromHelper->GetSettingInt("CamLenc", iCamLenc), false);

      global::SetCamAecMinValue(global::m_pEepromHelper->GetSettingInt("CamAecMinVal", iCamAecMinValue), false);
      global::SetCamAecMaxValue(global::m_pEepromHelper->GetSettingInt("CamAecMaxVal", iCamAecMaxValue), false);
      global::SetCamAdaptiveChangePercent(global::m_pEepromHelper->GetSettingInt("CamAdptChPer", iCamAdaptiveChangePercent), false);
      global::SetCamAdaptiveChangeDelay(global::m_pEepromHelper->GetSettingInt("CamAdptChDly", iCamAdaptiveChangeDelay), false);
      global::SetCamWhiteActivationTreshold(global::m_pEepromHelper->GetSettingByte("CamWhiteThres", bCamWhiteActivationTreshold), false);
      global::SetCamBeforeWhiteActivationTreshold(global::m_pEepromHelper->GetSettingByte("CamBeWhiteThr", bCamBeforeWhiteActivationTreshold), false);
      global::SetCamWhiteMinimumAmount(global::m_pEepromHelper->GetSettingByte("CamWhiteMinA", bCamWhiteMinimumAmount), false);

      global::SetCamAdaptiveGainDown(global::m_pEepromHelper->GetSettingByte("CamAdptGaDown", bCamAdaptiveGainDown), false);
      global::SetCamAdaptiveGainUp(global::m_pEepromHelper->GetSettingByte("CamAdptGaUp", bCamAdaptiveGainUp), false);
      global::SetCamAdaptiveGainMax(global::m_pEepromHelper->GetSettingByte("CamAdptGaMx", bCamAdaptiveGainMax), false);

      global::SetCamAdaptiveExposurecontrol(global::m_pEepromHelper->GetSettingInt("CamAdptExp", iCamAdaptiveExposurecontrol), false);
      global::SetCamAdaptiveGaincontrol(global::m_pEepromHelper->GetSettingInt("CamAdptGaCtrl", iCamAdaptiveGaincontrol), false);
    } else {
      // Load Preset from store EEPROM (With customized values)!
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAec_%d"), global::m_iExposurePreset);        global::SetCamAutoExposurecontrol(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamAec), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAec2_%d"), global::m_iExposurePreset);       global::SetCamAec2(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamAec2), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAelevels%d"), global::m_iExposurePreset);    global::SetCamAeLevels(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamAelevels), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAecValue%d"), global::m_iExposurePreset);    global::SetCamAecValue(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamAecValue), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamGain%d"), global::m_iExposurePreset);        global::SetCamGaincontrol(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamGain), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAgcGain%d"), global::m_iExposurePreset);     global::SetCamAgcGain(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamAgcGain), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamRawGMA%d"), global::m_iExposurePreset);      global::SetCamRawGMA(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamRawGMA), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamLenc%d"), global::m_iExposurePreset);        global::SetCamLenc(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamLenc), false);

      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAecMinVal%d"), global::m_iExposurePreset);   global::SetCamAecMinValue(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamAecMinValue), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAecMaxVal%d"), global::m_iExposurePreset);   global::SetCamAecMaxValue(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamAecMaxValue), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAdptChPer%d"), global::m_iExposurePreset);   global::SetCamAdaptiveChangePercent(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamAdaptiveChangePercent), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAdptChDly%d"), global::m_iExposurePreset);   global::SetCamAdaptiveChangeDelay(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamAdaptiveChangeDelay), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamWhiteThres%d"), global::m_iExposurePreset);  global::SetCamWhiteActivationTreshold(global::m_pEepromHelper->GetSettingByte(&szSettingID[0], bCamWhiteActivationTreshold), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamBeWhiteThr%d"), global::m_iExposurePreset);  global::SetCamBeforeWhiteActivationTreshold(global::m_pEepromHelper->GetSettingByte(&szSettingID[0], bCamBeforeWhiteActivationTreshold), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamWhiteMinA%d"), global::m_iExposurePreset);   global::SetCamWhiteMinimumAmount(global::m_pEepromHelper->GetSettingByte(&szSettingID[0], bCamWhiteMinimumAmount), false);

      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAdptGaDown%d"), global::m_iExposurePreset);  global::SetCamAdaptiveGainDown(global::m_pEepromHelper->GetSettingByte(&szSettingID[0], bCamAdaptiveGainDown), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAdptGaUp%d"), global::m_iExposurePreset);    global::SetCamAdaptiveGainUp(global::m_pEepromHelper->GetSettingByte(&szSettingID[0], bCamAdaptiveGainUp), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAdptGaMx%d"), global::m_iExposurePreset);    global::SetCamAdaptiveGainMax(global::m_pEepromHelper->GetSettingByte(&szSettingID[0], bCamAdaptiveGainMax), false);

      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAdptExp%d"), global::m_iExposurePreset);     global::SetCamAdaptiveExposurecontrol(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamAdaptiveExposurecontrol), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAdptGaCtrl%d"), global::m_iExposurePreset);  global::SetCamAdaptiveGaincontrol(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamAdaptiveGaincontrol), false);
    }

    // Reset some value after change if our own CamAdaptiveExposurecontrol is enabled!
    if (iCamAdaptiveExposurecontrol == 1) {
      global::SetCamAecValue(global::m_iCamAecMaxValue, false);
    }
    // Reset some value after change if our own CamAdaptiveGaincontrol is enabled!
    if (iCamAdaptiveGaincontrol == 1) {
      global::SetCamAgcGain(CAM_ADPT_GAIN_DEFAULT, false);
    }

    // Reset Cam HMirror and VFLIP!
    global::SetCamHMirror(global::m_iCamHMirror, false);
    global::SetCamVFlip(global::m_iCamVFlip, false);

    _DBGPRINTF(PSTR("global:SetExposurePreset to %d\n"), global::m_iExposurePreset);
  } else {
    _DBGPRINTF(PSTR("global:SetExposurePreset to %d READONLY CALL\n"), global::m_iExposurePreset);
  }
}

void global::SetCamPreset(int iIndex, void* voidresponse, bool bEpprom, bool bReadOnly) {
  global::m_iCamPreset = iIndex;
  // Write to EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingInt("CamPreset", iIndex);

  if (!global::m_bCameraOK) {
    _DBGPRINTF_VERBOSE(PSTR("global:SetCamPreset:Value set but CAMERA NOT INITIATED YET.\n"));
    return;
  }

  // Cast Response Pointer if not NULL
  AsyncResponseStream* response = (voidresponse != NULL ? (AsyncResponseStream*) voidresponse : NULL);

  // Custom Var Helper
  char szSettingID[ESP32_PREF_KEYMAX];

  // Set default values
  int iCamWbMode = 2;
  byte bCamColCorRed = 100;
  byte bCamColCorGreen = 100;
  byte bCamColCorBlue = 100;
  int iCamContrast = 0;
  int iCamBrightness = CAM_ADPT_BRIGHTNESS;
  int iCamSaturation = 0;
  int iCamAwb = 1;
  int iCamAwbGain = 1;
  int iCamExposureMode = 7; /* enumexposurepresets.adaptive */

  // 1. set default values!
  if (iIndex == CAM_auto_default /* enumcampresets.autodefault */) {
    iCamWbMode = 0;
    bCamColCorRed = 100;
    bCamColCorGreen = 85;
    bCamColCorBlue = 100;
    iCamContrast = 0;
    iCamBrightness = CAM_ADPT_BRIGHTNESS;
    iCamSaturation = 0;
    iCamAwb = 1;            // set_whitebal | Enable Auto White Balance Control
    iCamAwbGain = 1;        // set_awb_gain | Enable Auto White Balance Gain Control
    iCamExposureMode = CAM_ADPT_DEFAULT_MODE; /* enumexposurepresets.adaptiveraw */
  } else if (iIndex == CAM_auto_sunny /* enumcampresets.autosunny */) {
    iCamWbMode = 1;
    bCamColCorRed = 100;
    bCamColCorGreen = 100;
    bCamColCorBlue = 80;
    iCamContrast = 0;
    iCamBrightness = CAM_ADPT_BRIGHTNESS;
    iCamSaturation = 0;
    iCamAwb = 1;            // set_whitebal | Enable Auto White Balance Control
    iCamAwbGain = 1;        // set_awb_gain | Enable Auto White Balance Gain Control
    iCamExposureMode = CAM_ADPT_DEFAULT_MODE; /* enumexposurepresets.adaptiveraw */
  } else if (iIndex == CAM_auto_cloudy /* enumcampresets.autocloudy */) {
    iCamWbMode = 2;
    bCamColCorRed = 100;
    bCamColCorGreen = 100;
    bCamColCorBlue = 100;
    iCamContrast = 0;
    iCamBrightness = CAM_ADPT_BRIGHTNESS;
    iCamSaturation = 0;
    iCamAwb = 1;            // set_whitebal | Enable Auto White Balance Control
    iCamAwbGain = 1;        // set_awb_gain | Enable Auto White Balance Gain Control
    iCamExposureMode = CAM_ADPT_DEFAULT_MODE; /* enumexposurepresets.adaptiveraw */
  } else if (iIndex == CAM_auto_colorful /* enumcampresets.autocolorful */) {
    iCamWbMode = 1;
    bCamColCorRed = 100;
    bCamColCorGreen = 100;
    bCamColCorBlue = 80;
    iCamContrast = 0;
    iCamBrightness = CAM_ADPT_BRIGHTNESS;
    iCamSaturation = 2;
    iCamAwb = 1;            // set_whitebal | Enable Auto White Balance Control
    iCamAwbGain = 1;        // set_awb_gain | Enable Auto White Balance Gain Control
    iCamExposureMode = CAM_ADPT_DEFAULT_MODE; /* enumexposurepresets.adaptiveraw */
  } else if (iIndex == CAM_auto_office /* enumcampresets.autooffice */) {
    iCamWbMode = 3;
    bCamColCorRed = 100;
    bCamColCorGreen = 90;
    bCamColCorBlue = 65;
    iCamContrast = 0;
    iCamBrightness = CAM_ADPT_BRIGHTNESS;
    iCamSaturation = -1;
    iCamAwb = 1;            // set_whitebal | Enable Auto White Balance Control
    iCamAwbGain = 1;        // set_awb_gain | Enable Auto White Balance Gain Control
    iCamExposureMode = CAM_ADPT_DEFAULT_MODE; /* enumexposurepresets.adaptiveraw */
  } else if (iIndex == CAM_manual_default /* enumcampresets.manualdefault */) {
    iCamWbMode = 0;
    bCamColCorRed = 140;
    bCamColCorGreen = 80;
    bCamColCorBlue = 100;
    iCamContrast = 0;
    iCamBrightness = CAM_ADPT_BRIGHTNESS;
    iCamSaturation = 0;
    iCamAwb = 0;            // set_whitebal | Enable Auto White Balance Control
    iCamAwbGain = 0;        // set_awb_gain | Enable Auto White Balance Gain Control
    iCamExposureMode = CAM_ADPT_DEFAULT_MODE; /* enumexposurepresets.adaptiveraw */
  }

  // 2. Set webui default variables
  if (response != NULL) {
    response->printf_P(PSTR("window.CamWbMode = %d;\n"), iCamWbMode );
    response->printf_P(PSTR("window.CamColCorRed = %d;\n"), bCamColCorRed );
    response->printf_P(PSTR("window.CamColCorGreen = %d;\n"), bCamColCorGreen );
    response->printf_P(PSTR("window.CamColCorBlue = %d;\n"), bCamColCorBlue );
    response->printf_P(PSTR("window.CamContrast = %d;\n"), iCamContrast );
    response->printf_P(PSTR("window.CamBrightness = %d;\n"), iCamBrightness );
    response->printf_P(PSTR("window.CamSaturation = %d;\n"), iCamSaturation );
    response->printf_P(PSTR("window.CamAwb = %d;\n"), iCamAwb );
    response->printf_P(PSTR("window.CamAwbGain = %d;\n"), iCamAwbGain );
    response->printf_P(PSTR("window.CamExposureMode = %d;\n"), iCamExposureMode );
  }

  // 3. Apply values
  if ( bReadOnly == false ) {
    if ( iIndex == CAM_custom) {
      // Load CUSTOM from store EEPROM
      global::SetCamWbMode( global::m_pEepromHelper->GetSettingInt("CamWbMode", iCamWbMode), false);
      global::SetCamColorCorRed(global::m_pEepromHelper->GetSettingByte("CamColCrRed", bCamColCorRed), false);
      global::SetCamColorCorGreen(global::m_pEepromHelper->GetSettingByte("CamColCrGreen", bCamColCorGreen), false);
      global::SetCamColorCorBlue(global::m_pEepromHelper->GetSettingByte("CamColCrBlue", bCamColCorBlue), false);
      global::SetCamContrast(global::m_pEepromHelper->GetSettingInt("CamContrast", iCamContrast), false);
      global::SetCamBrightness(global::m_pEepromHelper->GetSettingInt("CamBrightness", iCamBrightness), false);
      global::SetCamSaturation(global::m_pEepromHelper->GetSettingInt("CamSaturation", iCamSaturation), false);
      global::SetCamWhitebalance(global::m_pEepromHelper->GetSettingInt("CamAwb", iCamAwb), false);
      global::SetCamAwbGainControl(global::m_pEepromHelper->GetSettingInt("CamAwbGain", iCamAwbGain), false);
      global::SetExposurePreset(global::m_pEepromHelper->GetSettingInt("CamExpPreset", iCamExposureMode), voidresponse, false, false);
    } else {
      // Load Preset from store EEPROM (With customized values)!
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamWbMode%d"), global::m_iCamPreset);      global::SetCamWbMode( global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamWbMode), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamColCrRed%d"), global::m_iCamPreset);    global::SetCamColorCorRed(global::m_pEepromHelper->GetSettingByte(&szSettingID[0], bCamColCorRed), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamColCrGreen%d"), global::m_iCamPreset);  global::SetCamColorCorGreen(global::m_pEepromHelper->GetSettingByte(&szSettingID[0], bCamColCorGreen), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamColCrBlue%d"), global::m_iCamPreset);   global::SetCamColorCorBlue(global::m_pEepromHelper->GetSettingByte(&szSettingID[0], bCamColCorBlue), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamContrast%d"), global::m_iCamPreset);    global::SetCamContrast(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamContrast), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamBrightness%d"), global::m_iCamPreset);  global::SetCamBrightness(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamBrightness), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamSaturation%d"), global::m_iCamPreset);  global::SetCamSaturation(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamSaturation), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAwb%d"), global::m_iCamPreset);         global::SetCamWhitebalance(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamAwb), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamAwbGain%d"), global::m_iCamPreset);     global::SetCamAwbGainControl(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamAwbGain), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamExpPreset%d"), global::m_iCamPreset);   global::SetExposurePreset(global::m_pEepromHelper->GetSettingInt(&szSettingID[0], iCamExposureMode), voidresponse, false, false);
    }

    _DBGPRINTF(PSTR("global:SetCamPreset to %d\n"), global::m_iCamPreset);
  } else {
    _DBGPRINTF(PSTR("global:SetCamPreset to %d READONLY CALL\n"), global::m_iCamPreset);
  }
}

void global::SetCamInnerPreset(int iIndex, void* voidresponse, bool bEpprom, bool bReadOnly) {
  global::m_iCamInnerPreset = iIndex;
  // Write to EEPROM
  if (bEpprom) {
    global::m_pEepromHelper->SetSettingInt("CamInnPreset", iIndex);
  }

  // Cast Response Pointer if not NULL
  AsyncResponseStream* response = (voidresponse != NULL ? (AsyncResponseStream*) voidresponse : NULL);

  // Custom Var Helper
  char szSettingID[ESP32_PREF_KEYMAX];

  // Set default (custom) values
  float flCamInnerHori = CAM_MAX_CALC_INNER_IMAGE_HORI;
  float flCamInnerVert = CAM_MAX_CALC_INNER_IMAGE_VERT;

  // 1. set default values!
  if (iIndex == CAM_inn_veryminimal /* enumcaminnerpresets.veryminimal */) {
    flCamInnerHori = 0.08;
    flCamInnerVert = 0.08;
  } else if (iIndex == CAM_inn_minimal /* enumcaminnerpresets.minimal */) {
    flCamInnerHori = 0.12;
    flCamInnerVert = 0.12;
  } else if (iIndex == CAM_inn_normal /* enumcaminnerpresets.normal */) {
    flCamInnerHori = CAM_MAX_CALC_INNER_IMAGE_HORI;
    flCamInnerVert = CAM_MAX_CALC_INNER_IMAGE_VERT;
  } else if (iIndex == CAM_inn_large /* enumcaminnerpresets.large */) {
    flCamInnerHori = 0.20;
    flCamInnerVert = 0.20;
  } else if (iIndex == CAM_inn_verylarge /* enumcaminnerpresets.verylarge */) {
    flCamInnerHori = 0.30;
    flCamInnerVert = 0.30;
  }

  // 2. Set webui default variables
  if (response != NULL) {
    response->printf_P(PSTR("window.CamInnerHori = %d;\n"), int(flCamInnerHori * 100) );
    response->printf_P(PSTR("window.CamInnerVert = %d;\n"), int(flCamInnerVert * 100) );
  }

  // 3. Apply values
  if ( bReadOnly == false ) {
    if ( iIndex == CAM_inn_custom) {
      // Load CUSTOM from store EEPROM
      global::m_pCamProcessor->SetCamInnerHori(global::m_pEepromHelper->GetSettingFloat("CamInnerHori", flCamInnerHori), false );
      global::m_pCamProcessor->SetCamInnerVert(global::m_pEepromHelper->GetSettingFloat("CamInnerVert", flCamInnerVert), false );
    } else {
      // Load Preset from store EEPROM (With customized values)!
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamInnerHori%d"), global::m_iCamInnerPreset);   global::m_pCamProcessor->SetCamInnerHori(global::m_pEepromHelper->GetSettingFloat(&szSettingID[0], flCamInnerHori), false);
      snprintf_P(szSettingID, ESP32_PREF_KEYMAX, PSTR("CamInnerVert%d"), global::m_iCamInnerPreset);   global::m_pCamProcessor->SetCamInnerVert(global::m_pEepromHelper->GetSettingFloat(&szSettingID[0], flCamInnerVert), false);
    }

    _DBGPRINTF(PSTR("global:SetCamInnerPreset to %d\n"), global::m_iCamInnerPreset);
  } else {
    _DBGPRINTF(PSTR("global:SetCamInnerPreset to %d READONLY CALL\n"), global::m_iCamInnerPreset);
  }
}
#endif
