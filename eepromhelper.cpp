// Include all needed files
#include "globalincludes.h"
#include "eepromhelper.h"
#if defined(ENABLE_CAM)
# include "camprocessor.h"
#endif

Preferences g_Preferences;

EEPROMHelper::EEPROMHelper(){
  m_iEepromSize = LAST_CONFIG;
  m_lLastUpdate = 0;
  g_Preferences.begin("ambiback", false);
  _DBGPRINTF(PSTR("EEPROMHelper: Constructor\n"));
}

EEPROMHelper::~EEPROMHelper(){
  // Writes changes into EEPROM as well.
  _DBGPRINTF(PSTR("EEPROMHelper: Destructor"));
  g_Preferences.end();
}

void EEPROMHelper::LoadSettings() {
  float flValue;
  byte bValue;
  int iValue; 
  String strTmp; 
  
  /* Read WLAN SSID */
  strTmp = g_Preferences.getString("WifiSSID", "");
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings Set WLAN SSID=%s\n"), strTmp.c_str());
  global::SetWifiSSID((char*) strTmp.c_str(), false);

  /* Read WLAN KEY */
  strTmp = g_Preferences.getString("WifiKEY", "");
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings Set WLAN KEY=%s\n"), strTmp.c_str());
  global::SetWifiPassword((char*) strTmp.c_str(), false);

  /* Read AmbiBackUseAP */
  bValue = (byte) g_Preferences.getChar("AmbiBackUseAP", (char)1);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings AmbiBackUseAP=%d\n"), bValue);
  global::SetWifiAPEnable(bValue, false);

  /* Read AP WLAN SSID */
  strTmp = g_Preferences.getString("APSSID", "");
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings Set AP WLAN SSID=%s\n"), strTmp.c_str());
  global::SetDeviceAPssid((char*) strTmp.c_str(), false);

  /* Read AP WLAN KEY */
  strTmp = g_Preferences.getString("APKEY", "");
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings Set AP WLAN KEY=%s\n"), strTmp.c_str());
  global::SetDeviceAPpass((char*) strTmp.c_str(), false);

#if defined(ENABLE_WIFI_AP)
  /* Read boot provisioning enable */
  bValue = (byte) g_Preferences.getChar("ProvEnabled", (char)1);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings ProvEnabled=%d\n"), bValue);
  global::SetWifiProvisioningEnabled(bValue, false);

  /* Read boot provisioning timeout in minutes */
  bValue = (byte) g_Preferences.getChar("ProvToutMin", (char)WIFI_PROVISIONING_BOOT_WINDOW_DEFAULT_MINUTES);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings ProvToutMin=%d\n"), bValue);
  global::SetWifiProvisioningTimeoutMinutes(bValue, false);
#endif

  /* Read DEVICE Name */
  strTmp = g_Preferences.getString("DeviceName", DEVICE_NAME);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings Set DEVICE NAME=%s\n"), strTmp.c_str());
  global::SetDeviceName((char*) strTmp.c_str(), false);

  /* Read SysBtnFunc */
  bValue = (byte) g_Preferences.getChar("SysBtnFunc", (char)255);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings SysBtnFunc=%d\n"), bValue);
  global::SetButtonFunction(bValue, false); 

  /* Read SysWifiFail */
  bValue = (byte) g_Preferences.getChar("SysWifiFail", (char)WIFI_FAIL_RECONNECT);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings SysWifiFail=%d\n"), bValue);
  global::SetWifiFailHandling(bValue, false); 

  /* Read restore last LED mode after reboot */
  bValue = (byte) g_Preferences.getChar("LedModeRest", (char)0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings LedModeRest=%d\n"), bValue);
  global::SetRestoreLastLedMode(bValue, false);

  /* Read ColorCorRed */
  bValue = (byte) g_Preferences.getChar("ColorCorRed", (char)255);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings ColorCorRed=%d\n"), bValue);
  global::m_pLedHelper->SetColorCorRed(bValue, false); 
  
  /* Read ColorCorGreen */
  bValue = (byte) g_Preferences.getChar("ColorCorGreen", (char)255);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings ColorCorGreen=%d\n"), bValue);
  global::m_pLedHelper->SetColorCorGreen(bValue, false); 

  /* Read ColorCorBlue */
  bValue = (byte) g_Preferences.getChar("ColorCorBlue", (char)255);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings ColorCorBlue=%d\n"), bValue);
  global::m_pLedHelper->SetColorCorBlue(bValue, false); 

  /* Read Brightness */
  iValue = g_Preferences.getInt("Brightness", 255);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings Brightness=%d\n"), iValue);
  global::m_pLedHelper->SetBrightness(iValue, false); 

  /* Read MinimumLuminosity */
  bValue = (byte) g_Preferences.getChar("MinLuminosity", (char) LED_DEF_MINLUMINOSITY);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings MinLuminosity=%d\n"), bValue);
  global::m_pLedHelper->SetMinimumLuminosity(bValue, false); 

  /* Read FrameLength */
  bValue = (byte) g_Preferences.getChar("FrameLength", (char)DEFAULT_FRAME_LENGTH);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings FrameLength=%d\n"), bValue);
  global::m_pLedHelper->SetFrameLength(bValue, false); 

  /* Read DelayLength */
  iValue = g_Preferences.getInt("DelayLength", DEFAULT_DELAY_LENGTH);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings DelayLength=%d\n"), iValue);
  global::m_pLedHelper->SetDelayLength(iValue, false); 

  /* Read FrameSmoothing */
  bValue = (byte) g_Preferences.getChar("FrameSmoothing", (char)DEFAULT_FRAME_SMOOTHING);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings FrameSmoothing=%d\n"), bValue);
  global::m_pLedHelper->SetFrameSmoothing(bValue, false); 

  /* Read SetupVeritcal */
  bValue = (byte) g_Preferences.getChar("SetupVertical", (char)18);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings SetupVertical=%d\n"), bValue);
  global::m_pLedHelper->SetSetupVertical(bValue, false);

  /* Read SetupHorizontal */
  bValue = (byte) g_Preferences.getChar("SetupHorizontal", (char)32);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings SetupHorizontal=%d\n"), bValue);
  global::m_pLedHelper->SetSetupHorizontal(bValue, false);

  /* Read SetupDirection */
  bValue = (byte) g_Preferences.getChar("SetupDirection", (char)0x01 /*Right_Cclock_bottom_right_up*/);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings SetupDirection=%d\n"), bValue);
  global::m_pLedHelper->SetSetupDirection(bValue, false);

  /* Read SetupSideLeft */
  bValue = (byte) g_Preferences.getChar("SetupSideLeft", (char)0x01 /* enabled */);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings SetupSideLeft=%d\n"), bValue);
  global::m_pLedHelper->SetSetupSideLeft(bValue, false);

  /* Read SetupSideTop */
  bValue = (byte) g_Preferences.getChar("SetupSideTop", (char)0x01 /* enabled */);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings SetupSideTop=%d\n"), bValue);
  global::m_pLedHelper->SetSetupSideTop(bValue, false);

  /* Read SetupSideRight */
  bValue = (byte) g_Preferences.getChar("SetupSideRight", (char)0x01 /* enabled */);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings SetupSideRight=%d\n"), bValue);
  global::m_pLedHelper->SetSetupSideRight(bValue, false);

  /* Read SetupSideBottom */
  bValue = (byte) g_Preferences.getChar("SetupSideBottom", (char)0x01 /* enabled */);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings SetupSideBottom=%d\n"), bValue);
  global::m_pLedHelper->SetSetupSideBottom(bValue, false);

  /* Read SideLeftBright */
  bValue = (byte) g_Preferences.getChar("SideLeftBright", (char)0x64 /* 100% */);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings SideLeftBright=%d\n"), bValue);
  global::m_pLedHelper->SetSideLeftBrightness(bValue, false);

  /* Read SideTopBright */
  bValue = (byte) g_Preferences.getChar("SideTopBright", (char)0x64 /* 100% */);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings SideTopBright=%d\n"), bValue);
  global::m_pLedHelper->SetSideTopBrightness(bValue, false);

  /* Read SideRightBright */
  bValue = (byte) g_Preferences.getChar("SideRightBright", (char)0x64 /* 100% */);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings SideRightBright=%d\n"), bValue);
  global::m_pLedHelper->SetSideRightBrightness(bValue, false);

  /* Read SideBtmBright */
  bValue = (byte) g_Preferences.getChar("SideBtmBright", (char)0x64 /* 100% */);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings SideBtmBright=%d\n"), bValue);
  global::m_pLedHelper->SetSideRightBrightness(bValue, false);

  /* Read ColorBoost */
  bValue = (byte) g_Preferences.getChar("ColorBoost", (char)0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings ColorBoost=%d\n"), bValue);
  global::m_pLedHelper->SetColorBoost(bValue, false);

  /* Read ColorBoostBalance */
  bValue = (byte) g_Preferences.getChar("ColorBoostBala", (char)50);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings ColorBoostBala=%d\n"), bValue);
  global::m_pLedHelper->SetColorBoostBalance(bValue, false);

  /* Read ColorBoostMin */
  bValue = (byte) g_Preferences.getChar("ColorBoostMin", (char)0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings ColorBoostMin=%d\n"), bValue);
  global::m_pLedHelper->SetColorBoostMin(bValue, false);

  /* Read ColorBoostRed */
  bValue = (byte) g_Preferences.getChar("ColorBoostRed", (char)100);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings ColorBoostRed=%d\n"), bValue);
  global::m_pLedHelper->SetColorBoostRed(bValue, false);

  /* Read ColorBoostGreen */
  bValue = (byte) g_Preferences.getChar("ColorBoostGreen", (char)100);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings ColorBoostGreen=%d\n"), bValue);
  global::m_pLedHelper->SetColorBoostGreen(bValue, false);

  /* Read ColorBoostBlue */
  bValue = (byte) g_Preferences.getChar("ColorBoostBlue", (char)100);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings ColorBoostBlue=%d\n"), bValue);
  global::m_pLedHelper->SetColorBoostBlue(bValue, false);

  /* Read ColorBoostThres */
  bValue = (byte) g_Preferences.getChar("ColorBoostThres", (char)0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings ColorBoostThres=%d\n"), bValue);
  global::m_pLedHelper->SetColorBoostThreshold (bValue, false);

  /* Read ColorValueGain */
  iValue = g_Preferences.getInt("ColorValueGain", 100);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings ColorValueGain=%d\n"), iValue);
  global::m_pLedHelper->SetColorValueGain(iValue, false);

  /* Read ColorPreset */
  iValue = g_Preferences.getInt("ColorPreset", 1);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings ColorPreset=%d\n"), iValue);
  global::SetColorPreset(iValue, false);

  /* Read BrightnsPreset */
  iValue = g_Preferences.getInt("BrightnsPreset", 3);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings BrightnsPreset=%d\n"), iValue);
  global::SetBrightnessPreset(iValue, false);

  /* Read MotionPreset */
  iValue = g_Preferences.getInt("MotionPreset", 2);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings MotionPreset=%d\n"), iValue);
  global::SetMotionPreset(iValue, false);

  /* Read AmbientPreset, AmbientSpeed, AmbientIntensity */
  iValue = g_Preferences.getInt("AmbientPreset", 0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings AmbientPreset=%d\n"), iValue);
  global::SetAmbientPreset(iValue, false);
  iValue = g_Preferences.getInt("AmbientSpeed", 5);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings AmbientSpeed=%d\n"), iValue);
  global::SetAmbientSpeed(iValue, false);
  iValue = g_Preferences.getInt("AmbientIntensity", 80);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings AmbientIntensity=%d\n"), iValue);
  global::SetAmbientIntensity(iValue, false);
  iValue = g_Preferences.getInt("AmbientHIntens", 80);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings AmbientHealthIntensity=%d\n"), iValue);
  global::SetAmbientHealthIntensity(iValue, false);

# if defined(ENABLE_CAM)
  short x = 25;
  short y = 25;

  /* Read CamPoint 1-4 */
  iValue = g_Preferences.getInt("CamPoint1", -1);
  if (iValue != -1){
    x = (short)(iValue>>16);
    y = (short)(iValue & 0xffff);
  }
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamPoint 1 x=%d y=%d\n"), x, y);
  global::m_pCamProcessor->setCameraPoint(1, x, y, false);

  iValue = g_Preferences.getInt("CamPoint2", -1);
  if (iValue != -1){
    x = (short)(iValue>>16);
    y = (short)(iValue & 0xffff);
  } else {
    x = 75; 
    y = 25; 
  }
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamPoint 2 x=%d y=%d\n"), x, y);
  global::m_pCamProcessor->setCameraPoint(2, x, y, false);

  iValue = g_Preferences.getInt("CamPoint3", -1);
  if (iValue != -1){
    x = (short)(iValue>>16);
    y = (short)(iValue & 0xffff);
  } else {
    x = 25; 
    y = 75; 
  }
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamPoint 3 x=%d y=%d\n"), x, y);
  global::m_pCamProcessor->setCameraPoint(3, x, y, false);

  iValue = g_Preferences.getInt("CamPoint4", -1);
  if (iValue != -1){
    x = (short)(iValue>>16);
    y = (short)(iValue & 0xffff);
  } else {
    x = 75; 
    y = 75; 
  }
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamPoint 4 x=%d y=%d\n"), x, y);
  global::m_pCamProcessor->setCameraPoint(4, x, y, false);

  // Load Camera properties
  iValue = g_Preferences.getInt("CamIndex", 0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamIndex=%d\n"), iValue);
  global::SetCamResolution(iValue, false);

  // Set Cam Frequence
# if defined(CAM_XCLK_FREQ)
  iValue = g_Preferences.getInt("CamFreq", CAM_XCLK_FREQ);
# else
  iValue = g_Preferences.getInt("CamFreq", FREQ_10);
# endif
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamFreq=%d\n"), iValue);
  global::SetCamFreq(iValue, false);

  iValue = g_Preferences.getInt("CamPerf", FPS_40);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamPerf=%d\n"), iValue);
  global::SetCamPerformance(iValue, false);

  iValue = g_Preferences.getInt("CamFbBuf", CAM_BUFFER_COUNT);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamFbBuf=%d\n"), iValue);
  global::SetCamFrameBuffer(iValue, false);

  iValue = g_Preferences.getInt("CamFlicker", FLICKER_AUTO);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamFlicker=%d\n"), iValue);
  global::SetCamFlicker(iValue, false);

# if CAM_SENSOR == CAM_SENSOR_OV3660
  bValue = (byte) g_Preferences.getChar("CamOverclock", (char) 0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamOverclock=%d\n"), bValue);
  global::SetCamOverclock(bValue, false);
# endif
  iValue = g_Preferences.getInt("CamContrast", 0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamContrast=%d\n"), iValue);
  global::SetCamContrast(iValue, false);

  iValue = g_Preferences.getInt("CamBrightness", CAM_ADPT_BRIGHTNESS);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamBrightness=%d\n"), iValue);
  global::SetCamBrightness(iValue, false);

  iValue = g_Preferences.getInt("CamSaturation", 0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamSaturation=%d\n"), iValue);
  global::SetCamSaturation(iValue, false);

  iValue = g_Preferences.getInt("CamSpecialEff", 0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamSpecialEff=%d\n"), iValue);
  global::SetCamSpecialEffect(iValue, false);

  iValue = g_Preferences.getInt("CamWbMode", 2);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamWbMode=%d\n"), iValue);
  global::SetCamWbMode(iValue, false);

  iValue = g_Preferences.getInt("CamAelevels", 0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamAelevels=%d\n"), iValue);
  global::SetCamAeLevels(iValue, false);

  iValue = g_Preferences.getInt("CamQuality", (psramFound() ? 6 : 12) );
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamQuality=%d\n"), iValue);
  global::SetCamQuality(iValue, false);

  iValue = g_Preferences.getInt("CamAec2", 0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamAec2=%d\n"), iValue);
  global::SetCamAec2(iValue, false);

  iValue = g_Preferences.getInt("CamAecValue", 0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamAecValue=%d\n"), iValue);
  global::SetCamAecValue(iValue, false);

  iValue = g_Preferences.getInt("CamAwb", 1);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamAwb=%d\n"), iValue);
  global::SetCamWhitebalance(iValue, false);

  iValue = g_Preferences.getInt("CamGain", 1);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamGain=%d\n"), iValue);
  global::SetCamGaincontrol(iValue, false);

  iValue = g_Preferences.getInt("CamAgcGain", 0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamAgcGain=%d\n"), iValue);
  global::SetCamAgcGain(iValue, false);

  iValue = g_Preferences.getInt("CamAwbGain", 1);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamAwbGain=%d\n"), iValue);
  global::SetCamAwbGainControl(iValue, false);

  iValue = g_Preferences.getInt("CamHMirror", CAM_HMIRROR);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamHMirror=%d\n"), iValue);
  global::SetCamHMirror(iValue, false);

  iValue = g_Preferences.getInt("CamVFlip", CAM_VFLIP);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamVFlip=%d\n"), iValue);
  global::SetCamVFlip(iValue, false);

  /* Read CamBPC */
  iValue = g_Preferences.getInt("CamBPC", 1);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamBPC=%d\n"), iValue);
  global::SetCamBPC(iValue, false);
  
  /* Read CamWPC */
  iValue = g_Preferences.getInt("CamWPC", 1);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamWPC=%d\n"), iValue);
  global::SetCamWPC(iValue, false);

  /* Read CamDenoise */
  iValue = g_Preferences.getInt("CamDenoise", 1);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamDenoise=%d\n"), iValue);
  global::SetCamDenoise(iValue, false);
  
  /* Read CamLenc */
  iValue = g_Preferences.getInt("CamLenc", 0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamLenc=%d\n"), iValue);
  global::SetCamLenc(iValue, false);

  /* Read CamRawGMA */
  iValue = g_Preferences.getInt("CamRawGMA", 1);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamRawGMA=%d\n"), iValue);
  global::SetCamRawGMA(iValue, false);

  /* Read CamAec */
  iValue = g_Preferences.getInt("CamAec", 0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamAec=%d\n"), iValue);
  global::SetCamAutoExposurecontrol(iValue, false);

  /* Read CamAdptExp */
  iValue = g_Preferences.getInt("CamAdptExp", 0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamAdptExp=%d\n"), iValue);
  global::SetCamAdaptiveExposurecontrol(iValue, false);

  /* Read CamAecMinVal */
  iValue = g_Preferences.getInt("CamAecMinVal", CAM_ADPT_AEC_MIN);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamAecMinVal=%d\n"), iValue);
  global::SetCamAecMinValue(iValue, false);

  /* Read CamAecMaxVal */
  iValue = g_Preferences.getInt("CamAecMaxVal", CAM_ADPT_AEC_MAX);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamAecMaxVal=%d\n"), iValue);
  global::SetCamAecMaxValue(iValue, false);

  /* Read CamWhiteThres */
  bValue = (byte) g_Preferences.getChar("CamWhiteThres", (char) WHITEACTTHRESHOLD_DEFAULT);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamWhiteThres=%d\n"), bValue);
  global::SetCamWhiteActivationTreshold(bValue, false);

  /* Read CamAdptChPer */
  bValue = (byte) g_Preferences.getChar("CamAdptChPer", (char) CAM_WHITE_AEC_CHANGEPERC);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamAdptChPer=%d\n"), bValue);
  global::SetCamAdaptiveChangeDelay(bValue, false);

  /* Read CamBeWhiteThr */
  bValue = (byte) g_Preferences.getChar("CamBeWhiteThr", (char) CAM_ADPT_BEFORE_ACT);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamBeWhiteThr=%d\n"), bValue);
  global::SetCamBeforeWhiteActivationTreshold(bValue, false);

  /* Read CamWhiteMinA */
  bValue = (byte) g_Preferences.getChar("CamWhiteMinA", (char) CAM_ADPT_WHITE_MIN);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamWhiteMinA=%d\n"), bValue);
  global::SetCamWhiteMinimumAmount(bValue, false);

  /* Read CamDcw */
  iValue = g_Preferences.getInt("CamDcw", 1);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamDcw=%d\n"), iValue);
  global::SetCamDcw(iValue, false);

  /* Read Cam Mode */
  bValue = (byte) g_Preferences.getChar("CamMode", (char) LED_MODE_CAM_DETAIL);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamMode=%d\n"), bValue);
  global::m_pCamProcessor->SetCamMode(bValue, false);

  /* Read CamDetailLevel */
  bValue = (byte) g_Preferences.getChar("CamDetailLevel", (char) (psramFound() ? CAM_DEFAULT_CALC_LEDS_SIDE : CAM_MIN_CALC_LEDS_SIDE));
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamDetailLevel=%d\n"), bValue);
  global::m_pCamProcessor->SetCamDetailLevel(bValue, false);

  /* DONE IN SetCamInnerPreset
   *  Read CamInnerHori 
  flValue = g_Preferences.getFloat("CamInnerHori", CAM_MAX_CALC_INNER_IMAGE_HORI);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamInnerHori=%fl\n"), flValue);
  global::m_pCamProcessor->SetCamInnerHori(flValue, false);
  /* Read CamInnerVert 
  flValue = g_Preferences.getFloat("CamInnerVert", CAM_MAX_CALC_INNER_IMAGE_VERT);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamInnerVert=%fl\n"), flValue);
  global::m_pCamProcessor->SetCamInnerVert(flValue, false);
  */

  /* Read Cam Detail Smooth */
  bValue = (byte) g_Preferences.getChar("CamDetailSmooth", (char) 0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamDetailSmooth=%d\n"), bValue);
  global::m_pCamProcessor->SetCamDetailSmooth(bValue, false);

  /* Read Cam Smooth Mode LAST, this can overrwrite other Options! */
  bValue = (byte) g_Preferences.getChar("CamSmoothMode", (char) 2 /*Default Medium*/);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamSmoothMode=%d\n"), bValue);
  global::m_pCamProcessor->SetCamSmooth(bValue, false);

  /* Read CamColCorRed*/
  bValue = (byte) g_Preferences.getChar("CamColCorRed", (char)100);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamColCorRed=%d\n"), bValue);
  global::SetCamColorCorRed(bValue, false);
  
  /* Read CamColCorGreen */
  bValue = (byte) g_Preferences.getChar("CamColCorGreen", (char)100);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamColCorGreen=%d\n"), bValue);
  global::SetCamColorCorGreen(bValue, false);

  /* Read CamColCorBlue  */
  bValue = (byte) g_Preferences.getChar("CamColCorBlue", (char)100);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamColCorBlue=%d\n"), bValue);
  global::SetCamColorCorBlue(bValue, false);

  /* Read CamActThreshold  */
  bValue = (byte) g_Preferences.getChar("CamActThreshold", (char)BLACKACTTHRESHOLD_DEFAULT);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamActThreshold=%d\n"), bValue);
  global::SetCamActivationThreshold(bValue, false);

  /* Read CamBlackSmooth  */
  bValue = (byte) g_Preferences.getChar("CamBlackSmooth", (char)BLACKDETECT_DEFAULT);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamBlackSmooth=%d\n"), bValue);
  global::m_pCamProcessor->SetCamBlackThresholdSmoothing(bValue, false);

  /* Read CamBlackRmThres */
  bValue = (byte) g_Preferences.getChar("CamBlackRmThres", (char)BLACKREMTHRESHOLD_DEFAULT);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamBlackRmThres=%d\n"), bValue);
  global::SetCamBlackRemoveThreshold(bValue, false);

  /* Read CamMinPixelAmnt */
  bValue = (byte) g_Preferences.getChar("CamMinPixelAmnt", (char)MIN_DEFAULT_PIXEL_AMOUNT);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamMinPixelAmnt=%d\n"), bValue);
  global::SetCamMinPixelAmount(bValue, false);

  /* Read CamAutoOnOff */
  bValue = (byte) g_Preferences.getChar("CamAutoOnOff", (char)0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamAutoOnOff=%d\n"), bValue);
  global::SetCamAutoOnOff(bValue, false);

  /* Read CamAutoOODelay */
  iValue = g_Preferences.getInt("CamAutoOODelay", 15);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamAutoOODelay=%d\n"), iValue);
  global::SetCamAutoOnOffDelay(iValue, false);

  /* Read CamHighQuality */
  bValue = (byte) g_Preferences.getChar("CamHighQuality", (char)0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamHighQuality=%d\n"), bValue);
  global::SetCamHighQuality(bValue, false);

  /* Read CamPreset */
  iValue = g_Preferences.getInt("CamPreset", CAM_auto_cloudy /*CAM_auto_default*/);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamPreset=%d\n"), iValue);
  global::SetCamPreset(iValue, false);

  /* Read CamExpPreset 
   *  LOADED IN SetCamPreset
  iValue = g_Preferences.getInt("CamExpPreset", CAM_exp_adaptive );
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamExpPreset=%d\n"), iValue);
  global::SetExposurePreset(iValue, false);
   */

  /* Read CamInnPreset */
  iValue = g_Preferences.getInt("CamInnPreset", 3 );
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings CamInnPreset=%d\n"), iValue);
  global::SetCamInnerPreset(iValue, false);

# endif
# if defined(ENABLE_HUE)
  /* Read HueSupport*/
  bValue = (byte) g_Preferences.getChar("HueSupport", (char)0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings HueSupport=%d\n"), bValue);
  global::SetHueSupport(bValue, false, false); // Init is done within LedHelper if mode is changed

  /* Read HueFrmLen*/
  bValue = (byte) g_Preferences.getChar("HueFrmLen", (char)28);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings HueFrmLen=%d\n"), bValue);
  global::SetFrameLengthDtls(bValue, false); 

  /* Read HueHndTout*/
  bValue = (byte) g_Preferences.getChar("HueHndTout", (char)10);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings HueHndTout=%d\n"), bValue);
  global::SetHandshakeTimeout(bValue, false); 

  /* Read HuePreset*/
  iValue = g_Preferences.getInt("HuePreset", 2);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings HuePreset=%d\n"), iValue);
  global::SetHuePreset(iValue, false);

  /* Read HueBridgeIP */
  strTmp = g_Preferences.getString("HueBridgeIP", "");
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings Set HueBridgeIP=%s\n"), strTmp.c_str());
  global::SetHueBridgeIP((char*) strTmp.c_str(), false);

  /* Read HueBridgeUser */
  strTmp = g_Preferences.getString("HueBridgeUser", "");
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings Set HueBridgeUser=%s\n"), strTmp.c_str());
  global::SetHueBridgeUser((char*) strTmp.c_str(), false);

  /* Read HueBridgeKey */
  strTmp = g_Preferences.getString("HueBridgeKey", "");
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings Set HueBridgeKey=%s\n"), strTmp.c_str());
  global::SetHueBridgeClientkey((char*) strTmp.c_str(), false);
# endif
# if defined(ENABLE_DREAMSCREEN)
  /* Read DreamScreenSupport */
  bValue = (byte) g_Preferences.getChar("DsSupport", (char)0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings DreamScreenSupport=%d\n"), bValue);
  global::SetDreamScreenSupport(bValue, false, false); // Init is done somewhwere ?!

  /* Read DreamScreenEmulation */
  bValue = (byte) g_Preferences.getChar("DsEmulation", (char)0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings DreamScreenEmulation=%d\n"), bValue);
  global::SetDreamScreenEmulation(bValue, false); 

  /* Read DreamScreenGroup */
  bValue = (byte) g_Preferences.getChar("DsGroup", (char)0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings DreamScreenGroup=%d\n"), bValue);
  global::SetDreamScreenGroup(bValue, false);

  /* Read DreamScreenLowColorSmoothing */
  bValue = (byte) g_Preferences.getChar("DsClrSmt", (char)0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings DreamScreenLowColorSmoothing=%d\n"), bValue);
  global::SetDreamScreenLowColorSmoothing(bValue, false);
# endif
# if defined(ENABLE_RELAY)
  /* Read AmbiBackSelectedDevice*/
  strTmp = g_Preferences.getString("AmbiBackSelDev", "");
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings Set AmbiBackSelDev=%s\n"), strTmp.c_str());
  global::SetAmbibackSelectedDevice((char*) strTmp.c_str(), false);

  /* Read AmbiBackRelDir */
  bValue = (byte) g_Preferences.getChar("AmbiBackRelDir", (char)1);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings AmbiBackRelDir=%d\n"), bValue);
  global::SetAmbibackRelayModeDirect(bValue, false);

  /* Read AmbiBackRelMde */
  bValue = (byte) g_Preferences.getChar("AmbiBackRelMde", (char) RELAY_UDP);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings AmbiBackRelMde=%d\n"), bValue);
  global::SetAmbibackRelayMode(bValue, false);

  /* Read AmbiBackRelLIP */
  strTmp = g_Preferences.getString("AmbiBackSelLIP", "");
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings Set AmbiBackRelLIP=%s\n"), strTmp.c_str());
  global::SetAmbibackRelayIPAddr((char*) strTmp.c_str(), false);

  /* Read AmbiBackWifiDi */
  bValue = (byte) g_Preferences.getChar("AmbiBackWifiDi", (char)0);
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings AmbiBackWifiDi=%d\n"), bValue);
  global::SetAmbibackWifiDirectMode(bValue, false);

  /* Read AmbiBackSeSSID */
  strTmp = g_Preferences.getString("AmbiBackSeSSID", "");
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings Set AmbiBackSeSSID=%s\n"), strTmp.c_str());
  global::SetAmbibackRelaySsid((char*) strTmp.c_str(), false);

  /* Read AmbiBackSeKEY */
  strTmp = g_Preferences.getString("AmbiBackSeKEY", "");
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper: LoadSettings Set AmbiBackSeKEY=%s\n"), strTmp.c_str());
  global::SetAmbibackRelayPass((char*) strTmp.c_str(), false);
# endif
}

void EEPROMHelper::FactoryReset() {
  // Remove all preferences under the opened namespace
  g_Preferences.clear();

  // Writes changes into EEPROM 
  this->CommitChanges(); 

  // Settings saved
  global::SetSettingsChanged(false); 

  // Factory reset performed
  _DBGPRINTF(PSTR("EEPROMHelper: FactoryReset done!\n"));
}

void EEPROMHelper::CommitChanges() {
  // Settings saved
  global::SetSettingsChanged(false); 
}

void EEPROMHelper::SetSettingString(int startaddress, char* psz, size_t stMaxLength){
  // Wrap to this function
  return this->SetSettingString(startaddress, psz, strlen(psz), stMaxLength); 
}
void EEPROMHelper::SetSettingString(int startaddress, String str, size_t stMaxLength){
  // Wrap to this function
  return this->SetSettingString(startaddress, (char*) str.c_str(), str.length(), stMaxLength); 
}
void EEPROMHelper::SetSettingString(int startaddress, char* psz, size_t  stLength, size_t stMaxLength){
  switch(startaddress){
    case WLAN_SSID_START: 
      g_Preferences.putString("WifiSSID", psz);
      break; 
    case WLAN_KEY_START: 
      g_Preferences.putString("WifiKEY", psz);
      break; 
    case DEVICE_NAME_START: 
      g_Preferences.putString("DeviceName", psz);
      break; 
    default: 
      global::errprintf_P(PSTR("EEPROMHelper:SetSettingString=UNKNOWN PARAMETER: %d/%s)\n"), startaddress, psz);
      break; 
  }

  // Set Settings Changed Property!
  global::SetSettingsChanged(true); 

  // Set last Update timestamp!
  this->SetLastUpdate(); 

  // Debug output
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper:SetSettingString=%s(%d/%d)\n"), psz, stLength, stMaxLength);
}

void EEPROMHelper::SetSettingFloat(const char* pszSettingID, float value) {
  // Kindly put FLOAT value into Preferences
  int iRet = g_Preferences.putFloat(pszSettingID, value); 
  if (iRet != 0) {
    // Set Settings Changed Property!
    global::SetSettingsChanged(true); 
  
    // Set last Update timestamp!
    this->SetLastUpdate(); 
  
    // Debug output
    _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper:SetSettingFloat SettingID=%s Value=%f\n"), pszSettingID, value);
  } else {
    // Error output
    global::errprintf_P(PSTR("EEPROMHelper:SetSettingFloat FAILED for SettingID=%s Value=%f\n"), 
      pszSettingID, value);
  }
}

void EEPROMHelper::SetSettingInt(const char* pszSettingID, int value) {
  // Kindly put INT value into Preferences
  int iRet = g_Preferences.putInt(pszSettingID, value); 
  if (iRet != 0) {
    // Set Settings Changed Property!
    global::SetSettingsChanged(true); 
  
    // Set last Update timestamp!
    this->SetLastUpdate(); 
  
    // Debug output
    _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper:SetSettingInt SettingID=%s Value=%d\n"), pszSettingID, value);
  } else {
    // Error output
    global::errprintf_P(PSTR("EEPROMHelper:SetSettingInt FAILED for SettingID=%s Value=%d\n"), 
      pszSettingID, value);
  }
}
byte EEPROMHelper::GetSettingByte(const char* pszSettingID, byte bDefault, bool bShowDebug) {
  byte bValue = g_Preferences.getChar(pszSettingID, bDefault);
  if (bShowDebug) {
    _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper:GetSettingByte: SettingID=%s Value=%d/%d\n"), pszSettingID, bValue, bDefault);
  }
  return bValue;
}
int EEPROMHelper::GetSettingInt(const char* pszSettingID, int iDefault, bool bShowDebug) {
  int iValue = g_Preferences.getInt(pszSettingID, iDefault);
  if (bShowDebug) {
    _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper:GetSettingInt: SettingID=%s Value=%d/%d\n"), pszSettingID, iValue, iDefault);
  }
  return iValue;
}
float EEPROMHelper::GetSettingFloat(const char* pszSettingID, float flDefault, bool bShowDebug) {
  float flValue = g_Preferences.getFloat(pszSettingID, flDefault);
  if (bShowDebug) {
    _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper:GetSettingFloat: SettingID=%s Value=%fl/%fl\n"), pszSettingID, flValue, flDefault);
  }
  return flValue;
}

void EEPROMHelper::SetSettingByte(const char* pszSettingID, byte value) {
  // Kindly put INT value into Preferences
  int iRet = g_Preferences.putChar(pszSettingID, (int8_t)value);
  if (iRet != 0) {
    // Set Settings Changed Property!
    global::SetSettingsChanged(true); 
  
    // Set last Update timestamp!
    this->SetLastUpdate(); 
  
    // Debug output
    _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper:SetSettingByte SettingID=%s Value=%d\n"), pszSettingID, value);
  } else {
    // Error output
    global::errprintf_P(PSTR("EEPROMHelper:SetSettingByte FAILED for SettingID=%s Value=%d\n"), 
      pszSettingID, value);
  }
}
void EEPROMHelper::SetSettingString(const char* pszSettingID, char* psz, size_t stMaxLength){
  // Wrap to this function
  return this->SetSettingString(pszSettingID, psz, strlen(psz), stMaxLength); 
}
void EEPROMHelper::SetSettingString(const char* pszSettingID, String str, size_t stMaxLength){
  // Wrap to this function
  return this->SetSettingString(pszSettingID, (char*) str.c_str(), str.length(), stMaxLength); 
}
void EEPROMHelper::SetSettingString(const char* pszSettingID, char* psz, size_t  stLength, size_t stMaxLength){
  int iRet = g_Preferences.putString(pszSettingID, psz);
  if (iRet != 0) {
    // Set Settings Changed Property!
    global::SetSettingsChanged(true); 
  
    // Set last Update timestamp!
    this->SetLastUpdate(); 
  
    // Debug output
    _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper:SetSettingString SettingID=%s Value=%s (%d/%d)\n"), pszSettingID, psz, stLength, stMaxLength);
  } else {
    // Error output
    global::errprintf_P(PSTR("EEPROMHelper:SetSettingString FAILED for SettingID=%s Value=%s (%d/%d)\n"), 
      pszSettingID, psz, stLength, stMaxLength);
  }
}

void EEPROMHelper::SetSettingInt(int startaddress, int value) {
  switch(startaddress){
    case DELAY_LENGTH_START: 
      g_Preferences.putInt("DelayLength", value);
      break; 
    case COLOR_BRIGHTNESS_START: 
      g_Preferences.putInt("Brightness", value);
      break; 
    case COLOR_VALUE_GAIN_START: 
      g_Preferences.putInt("ColorValueGain", value);
      break; 
#if defined(ENABLE_CAM)
    case CAM_POINT_1_START: 
      g_Preferences.putInt("CamPoint1", value);
      break; 
    case CAM_POINT_2_START: 
      g_Preferences.putInt("CamPoint2", value);
      break; 
    case CAM_POINT_3_START: 
      g_Preferences.putInt("CamPoint3", value);
      break; 
    case CAM_POINT_4_START: 
      g_Preferences.putInt("CamPoint4", value);
      break; 
    case CAM_INDEX_START: 
      g_Preferences.putInt("CamIndex", value);
      break; 
    case CAM_PRESET_START: 
      g_Preferences.putInt("CamPreset", value);
      break; 
    case CAM_CONTRAST_START: 
      g_Preferences.putInt("CamContrast", value);
      break; 
    case CAM_BRIGHTNESS_START: 
      g_Preferences.putInt("CamBrightness", value);
      break; 
    case CAM_SATURATION_START: 
      g_Preferences.putInt("CamSaturation", value);
      break; 
    case CAM_SPECIALEFFECT_START: 
      g_Preferences.putInt("CamSpecialEff", value);
      break; 
    case CAM_WBMODE_START: 
      g_Preferences.putInt("CamWbMode", value);
      break; 
    case CAM_AELEVELS_START: 
      g_Preferences.putInt("CamAelevels", value);
      break;       
    case CAM_QUALITY_START: 
      g_Preferences.putInt("CamQuality", value);
      break;       
    case CAM_AEC2_START: 
      g_Preferences.putInt("CamAec2", value);
      break;
    case CAM_AECVALUE_START: 
      g_Preferences.putInt("CamAecValue", value);
      break;
    case CAM_AWBGAIN_START: 
      g_Preferences.putInt("CamAwbGain", value);
      break;
    case CAM_HMIRROR_START: 
      g_Preferences.putInt("CamHMirror", value);
      break;
    case CAM_VVLIP_START: 
      g_Preferences.putInt("CamVFlip", value);
      break;
#endif      
    default: 
      global::errprintf_P(PSTR("EEPROMHelper:SetSettingInt=UNKNOWN PARAMETER: %d/%d)\n"), startaddress, value);
      break; 
  }
  
  // Set Settings Changed Property!
  global::SetSettingsChanged(true); 

  // Set last Update timestamp!
  this->SetLastUpdate(); 

  // Debug output
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper:SetSettingInt=%d\n"), value);
}
void EEPROMHelper::SetSettingUInt(int startaddress, unsigned int value){
  /* Wrapper for uint function */
  this->SetSettingInt(startaddress, (int) value); 
}
void EEPROMHelper::SetSettingByte(int startaddress, byte value){
  switch(startaddress){
    case COLOR_COR_RED_START: 
      g_Preferences.putChar("ColorCorRed", (int8_t)value);
      break; 
    case COLOR_COR_GREEN_START: 
      g_Preferences.putChar("ColorCorGreen", (int8_t)value);
      break; 
    case COLOR_COR_BLUE_START: 
      g_Preferences.putChar("ColorCorBlue", (int8_t)value);
      break; 
    case COLOR_BRIGHTNESS_START: 
      g_Preferences.putChar("Brightness", (int8_t)value);
      break; 
    case COLOR_MIN_LUMIN_START: 
      g_Preferences.putChar("MinLuminosity", (int8_t)value);
      break; 
    case FRAME_LENGTH_START: 
      g_Preferences.putChar("FrameLength", (int8_t)value);
      break; 
    case FRAME_SMOOTHING_START: 
      g_Preferences.putChar("FrameSmoothing", (int8_t)value);
      break; 
    case COLOR_BOOST_START: 
      g_Preferences.putChar("ColorBoost", (int8_t)value);
      break; 
    case COLOR_BOOST_MIN_START: 
      g_Preferences.putChar("ColorBoostMin", (int8_t)value);
      break; 
    case SETUP_VERTICAL_START: 
      g_Preferences.putChar("SetupVertical", (int8_t)value);
      break; 
    case SETUP_HORIZONTAL_START: 
      g_Preferences.putChar("SetupHorizontal", (int8_t)value);
      break; 
    case SETUP_DIRECTION_START: 
      g_Preferences.putChar("SetupDirection", (int8_t)value);
      break; 
    case SETUP_SIDELEFT_START: 
      g_Preferences.putChar("SetupSideLeft", (int8_t)value);
      break; 
    case SETUP_SIDETOP_START: 
      g_Preferences.putChar("SetupSideTop", (int8_t)value);
      break; 
    case SETUP_SIDERIGHT_START: 
      g_Preferences.putChar("SetupSideRight", (int8_t)value);
      break; 
    case SETUP_SIDEBOTTOM_START: 
      g_Preferences.putChar("SetupSideBottom", (int8_t)value);
      break; 
# if defined(ENABLE_CAM)
    /* Write Cam Mode */
    case CAM_MODE_START: 
      g_Preferences.putChar("CamMode", (int8_t)value);
      break;       
    case CAM_DETAILLEV_START: 
      g_Preferences.putChar("CamDetailLevel", (int8_t)value);
      break;       
    case CAM_COLOR_RED_START: 
      g_Preferences.putChar("CamColCorRed", (int8_t)value);
      break;       
    case CAM_COLOR_GREEN_START: 
      g_Preferences.putChar("CamColCorGreen", (int8_t)value);
      break;       
    case CAM_COLOR_BLUE_START: 
      g_Preferences.putChar("CamColCorBlue", (int8_t)value);
      break;       
    case CAM_COLOR_ACTRSHLD_START: 
      g_Preferences.putChar("CamActThreshold", (int8_t)value);
      break;       
# endif
    default: 
      global::errprintf_P(PSTR("EEPROMHelper:SetSettingByte=UNKNOWN PARAMETER: %d/%d)\n"), startaddress, value);
      break; 
  }

  // Set Settings Changed Property!
  global::SetSettingsChanged(true); 

  // Set last Update timestamp!
  this->SetLastUpdate(); 

  // Debug output
  _DBGPRINTF_VERBOSE(PSTR("EEPROMHelper:SetSettingByte(%d)=%d\n"), startaddress, value);
}
