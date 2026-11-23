// Uncomment/edit one of the following lines for your leds arrangement.
// FastLED.addLeds<WS2811, LEDCTRL_PIN, RGB>(leds, NUM_LEDS);
// FastLED.addLeds<WS2812, LEDCTRL_PIN, RGB>(leds, NUM_LEDS);
// FastLED.addLeds<NEOPIXEL, LEDCTRL_PIN>(leds, NUM_LEDS);
// FastLED.addLeds<WS2801, RGB>(leds, NUM_LEDS);
// FastLED.addLeds<WS2801, LEDCTRL_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
// FastLED.addLeds<LPD8806, LEDCTRL_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);

// Include all needed files
#include "globalincludes.h"
#include "esp_heap_caps.h"
// Include FastLED here only
#include "FastLED.h"
#include "ledhelper.h"

// Include libs that are required in code here
#include "udpserver.h"
#ifdef ENABLE_IR
# include "irhelper.h"
#endif

#include "global.h"
#include <math.h>
#if defined(ENABLE_CAM)
#  include "camprocessor.h"
#  include "camtask.h"
#endif
#define LED_NUMPORTS 4

// Static Array of all possible LEDs
static CRGB m_leds1[MAX_LEDS];
#if defined(USE_HAS_SECOND_PORT)
static CRGB m_leds2[MAX_LEDS];
#endif
static CRGB m_leds_next[MAX_LEDS];
static CRGB m_ledspwr[MAX_CONTROL_LEDS];
static CRGB m_ledsctrl[MAX_CONTROL_LEDS];
static CRGB* m_pLedFrames;        /* Multideminsional Array keeping last LED Frames for Frame Smoothing*/

static CLEDController *m_controllers[LED_NUMPORTS];
static CRGB* m_pLeds[4];
static int m_pLedCount[4];
static LEDHelper* pStaticLedhelper = NULL;       // Local LED Helper

// Task Handles
TaskHandle_t handleTaskFadeOut;
TaskHandle_t handleTaskProcessNextStep;
TaskHandle_t handleTaskDemo;
TaskHandle_t handleTaskAmbient;
// Task Helper Functions
void TaskFadeOutCode(void * pvParameters);
void TaskProcessNextStep(void * pvParameters);
void TaskDemoCode(void * pvParameters);
void TaskAmbientCode(void * pvParameters);

// General CAM Processing Timer helpers (TaskProcessNextStep and TaskDemoCode)
unsigned long m_nTimeStart = 0;
unsigned long m_nTimeFinish = 0;
unsigned long m_nTimeFPSCheck = 0;
#if defined(ENABLE_CAM)
unsigned long m_nTimeCapture = 0;
unsigned long m_nTimeJpg2Rgb = 0;
unsigned long m_nTimeAfterColorCalc = 0;
#endif

// Static helpers
unsigned long timeFPSCheck = 0;

LEDHelper::LEDHelper() {
#if defined(ENABLE_RELAY)
  // Init TCP Client
  pTcpClient = NULL;
  m_bClientConnected = false;
#endif
  m_bRed = 255;
  m_bGreen = 255;
  m_bBlue = 255;
  m_bRedStep = 8;
  m_bGreenStep = 8;
  m_bBlueStep = 8;

  // Set default (stored = count-1, so 49 = 50 LEDs)
  m_numleds = 49;
  m_bNewLedData = false;
  m_bShowLedActive = false;

  m_bPowerLedOn = false;
  m_bControlLedOn = false;

  // Mode to OFF
  m_bLedMode = LED_MODE_OFF;
  m_bChangeLedMode = false;
  m_bNewLedMode = LED_MODE_OFF;

  m_lLastUpdate = millis();
  m_lVideoTimeout = DEFAULT_LED_TIMEOUT; // 15 seconds default, shoult be configurable
  m_bCurrentFramesCtr = 0;
  m_bCurrentFps = 0;

  // Set configureable defaults
  m_bFrameLength = DEFAULT_FRAME_LENGTH;
  m_iDelayLength = DEFAULT_DELAY_LENGTH;
  m_bFrameSmoothing = DEFAULT_FRAME_SMOOTHING;
  m_pLedFrames = NULL;

  m_bColorCorRed = 255;
  m_bColorCorGreen = 255;
  m_bColorCorBlue = 255;
  m_iBrightness = 255;
  m_bMinimumLuminosity = 0;
  m_bColorBoost = 0;
  m_bColorBoostBalance = 50;
  m_bColorBoostRed = 128;
  m_bColorBoostGreen = 128;
  m_bColorBoostBlue = 128;
  m_bColorBoostMin = 0;
  m_iColorValueGain = 100;
  m_bColorBoostThreshold = 32;

  m_bSetupVertical = 18;
  m_bSetupHorizontal = 32;
  m_bSetupDirection = Right_Cclock_bottom_right_up;
  m_bSetupSideLeft = 1;
  m_bSetupSideTop = 1;
  m_bSetupSideRight = 1;
  m_bSetupSideBottom = 1;
  m_bSideLeftBrightness = 100;
  m_bSideTopBrightness = 100;
  m_bSideRightBrightness = 100;
  m_bSideBottomBrightness = 100;

  // Others helper Defaults
  m_bProcessingLedNextStep = false;
  m_bFadeOutRunning = false;
  m_bBlinkTaskRunning = false;

  /* Init LED Data Arrays */
  memset(m_leds1, 0, MAX_LEDS * sizeof(struct CRGB));
#if defined(USE_HAS_SECOND_PORT)
  memset(m_leds2, 0, MAX_LEDS * sizeof(struct CRGB));
#endif
  memset(m_leds_next, 0, MAX_LEDS * sizeof(struct CRGB));
  memset(m_ledspwr, 0, MAX_CONTROL_LEDS * sizeof(struct CRGB));
  memset(m_ledsctrl, 0, MAX_CONTROL_LEDS * sizeof(struct CRGB));

  /* Preinit LED Controllers to NULL */
  m_controllers[LED_PWRIDX] = NULL;
  m_controllers[LED_CTRLIDX] = NULL;
  m_controllers[LED_PRT1IDX] = NULL;
  m_controllers[LED_PRT2IDX] = NULL;
  /* Preinit LED Values to NULL */
  m_pLedCount[LED_PWRIDX] = 0;
  m_pLedCount[LED_CTRLIDX] = 0;
  m_pLedCount[LED_PRT1IDX] = 0;
  m_pLedCount[LED_PRT2IDX] = 0;

  /* Get pointers to helper array */
  m_pLeds[LED_PWRIDX] = &m_ledspwr[0];
  m_pLedCount[LED_PWRIDX] = MAX_CONTROL_LEDS;  /* Init LED Count */
#if CTLLED_PIN >= 0
  m_pLeds[LED_CTRLIDX] = &m_ledsctrl[0];
  m_pLedCount[LED_CTRLIDX] = MAX_CONTROL_LEDS; /* Init LED Count */
#else
  m_pLeds[LED_CTRLIDX] = NULL;
#endif
#if defined(ENABLE_VIDEO)
  m_pLeds[LED_PRT1IDX] = &m_leds1[0];
  m_pLedCount[LED_PRT1IDX] = MAX_LEDS;
# if defined(USE_HAS_SECOND_PORT)
  m_pLeds[LED_PRT2IDX] = &m_leds2[0];
  m_pLedCount[LED_PRT2IDX] = MAX_LEDS;
# else
#  if defined(CLONE_TO_SECOND_PORT)
  m_pLeds[LED_PRT2IDX] = &m_leds1[0];
  m_pLedCount[LED_PRT2IDX] = MAX_LEDS;
#  else
  m_pLeds[LED_PRT2IDX] = NULL;
#  endif
# endif
#else
  m_pLeds[LED_PRT1IDX] = NULL;
  m_pLeds[LED_PRT2IDX] = NULL;
#endif

  // Default 0 means disabled
  m_shSequence = 0;

#if defined(ESP32)
  // Init Task Handles
  handleTaskFadeOut = NULL;
  handleTaskProcessNextStep = NULL;
  handleTaskDemo = NULL;
#endif

  // Set static helper
  pStaticLedhelper = this;
}

void LEDHelper::InitLeds() {
  // Tell FastLED we are using WS2812B protocol - other protocols possible
  // FastLED.addLeds<WS2812B, LEDCTRL_PIN, GRB>(m_leds, MAX_LEDS /*NUM_LEDS*/);
  _DBGPRINTF(PSTR(PSTR_LED "InitLeds\n"));

  // Create LED Controllers
#ifdef ENABLE_VIDEO
  m_controllers[LED_PRT1IDX]  = &FastLED.addLeds<WS2812B, LEDCTRL_PIN, GRB>(m_leds1, MAX_LEDS);
# ifdef USE_HAS_SECOND_PORT
  if (LEDCTRL_PIN_SEC >= 0)
    m_controllers[LED_PRT2IDX]  = &FastLED.addLeds<WS2812B, LEDCTRL_PIN_SEC, GRB>(m_leds2, MAX_LEDS);
# else
#  if defined(CLONE_TO_SECOND_PORT)
  // Clone LEDPort 1
  if (LEDCTRL_PIN_SEC >= 0)
    m_controllers[LED_PRT2IDX]  = &FastLED.addLeds<WS2812B, LEDCTRL_PIN_SEC, GRB>(m_leds1, MAX_LEDS);
#  else
  m_controllers[LED_PRT2IDX]  = NULL;
#  endif
# endif
#else
  m_controllers[LED_PRT1IDX]  = NULL;
  m_controllers[LED_PRT2IDX]  = NULL;
#endif

#ifdef USE_WS2812_CONTROL_LEDS
  m_controllers[LED_PWRIDX]   = &FastLED.addLeds<WS2812B, PWRLED_PIN, GRB>(m_ledspwr, MAX_CONTROL_LEDS);
  //  m_controllers[LED_PWRIDX]   = &FastLED.addLeds<WS2812, PWRLED_PIN>(m_ledspwr, MAX_CONTROL_LEDS);
# if CTLLED_PIN >= 0
  m_controllers[LED_CTRLIDX]  = &FastLED.addLeds<WS2812B, CTLLED_PIN, GRB>(m_ledsctrl, MAX_CONTROL_LEDS);
# else
  m_controllers[LED_CTRLIDX]  = NULL;
# endif
#endif

}

void LEDHelper::LedDemoMode(byte bLeds, byte bLedMode, unsigned long lTimeDuration) {

  switch (bLedMode) {
    case LED_DEMO_DEFAULT:
    default:
      // Initial boot chase: fill the configured strip one LED at a time per primary color.
      this->bootColorChase(255, 0, 0, 10, 0, true);
      this->bootColorChase(0, 255, 0, 10, 0, true);
      this->bootColorChase(0, 0, 255, 10, 0, true);
      break;
  }

}

#if defined(ESP32)
void LEDHelper::EnableTaskProcessNextStep()
{
  // Use TASK for ESP32
  if (handleTaskProcessNextStep == NULL) {
    xTaskCreatePinnedToCore(&TaskProcessNextStep,
                            "TaskProcessNextStep",
                            4096,
                            this,
                            DEFAULT_TASK_PRIORITY + 2, /* +2 priority */
                            &handleTaskProcessNextStep,
                            ARDUINO_RUNNING_CORE); /* pin to internal core 1 */
    _DBGPRINTF(PSTR("EnableTaskProcessNextStep: Created TASK for TaskProcessNextStep with HANDLE %d\n"), handleTaskProcessNextStep);
  } else {
    int iStatus = eTaskGetState(handleTaskProcessNextStep);
    if (iStatus == eSuspended) {
      // _DBGPRINTF(PSTR("LEDHelper:EnableTaskProcessNextStep TASK for TaskProcessNextStep with HANDLE %d is suspended, resuming ... \n"), handleTaskProcessNextStep);
      vTaskResume(handleTaskProcessNextStep);
    } else {
      _DBGPRINTF_VERBOSE(PSTR("LEDHelper:EnableTaskProcessNextStep TASK for TaskProcessNextStep already running ... \n"));
    }
  }
}
#endif

void LEDHelper::EnableTaskFadeOut() {
  // Set FadeOut to work
  this->setFadeOutRunning( true );
  // Create Task for Fadeout if NULL
  if (handleTaskFadeOut == NULL) {
    // Create new Task
    xTaskCreatePinnedToCore(&TaskFadeOutCode,
                            "TaskFadeOut",
                            4096,
                            this,
                            DEFAULT_TASK_PRIORITY, /* Default priority */
                            &handleTaskFadeOut,
                            ARDUINO_RUNNING_CORE); /* pin task to core 1 */
    _DBGPRINTF(PSTR(PSTR_LED "TaskFadeOut created HANDLE %d\n"), handleTaskFadeOut);
  } else {
    int iStatus = eTaskGetState(handleTaskFadeOut);
    if (iStatus == eSuspended) {
      _DBGPRINTF(PSTR(PSTR_LED "TaskFadeOut suspended, resuming\n"));
      vTaskResume(handleTaskFadeOut);
    } else {
      _DBGPRINTF_VERBOSE(PSTR("LEDHelper: EnableTaskFadeOut: TASK for TaskFadeOut already running ... \n"));
    }
  }
}

void LEDHelper::HandleLeds() {
  unsigned long timeNow = 0;

  if (timeFPSCheck == 0) {
    timeFPSCheck = millis() + 1000;
  } else {
    /* Handle FPS counting after 1000ms period of time */
    if (millis() > timeFPSCheck ) {
      /* Update timeWIFICheck */
      _DBGPRINTF_ULTRA(PSTR("LEDHelper:HandleLeds: FPS: %d\n"), m_bCurrentFramesCtr);
      pStaticLedhelper->SetCurrentFps(m_bCurrentFramesCtr);

      // Reset back to 0
      m_bCurrentFramesCtr = 0;
      timeFPSCheck = millis() + 1000;
    }
  }

  if (m_bChangeLedMode) {
    // LED Mode Change was requested from outside (Like from WebServer)
    m_bChangeLedMode = false;

    // Perform the change
    this->SetLedMode(m_bNewLedMode);
  }

  // Handle Video/Cam/Ambient Modes
  if (this->m_bLedMode == LED_MODE_VIDEO) {
    if (this->GetIsNewLedData() == true) {
#if defined(ESP32)
      // Make sure we are in Processing now
      this->setProcessingLedNextStep(true);
      // Make sure our Process Task is running
      this->EnableTaskProcessNextStep();
#else
      // In Processing now
      this->setProcessingLedNextStep(true);
      /* Process LED Data Frame */
      this->ProcessLedNextStep();
      // Processing done
      this->setProcessingLedNextStep(false);
#endif
    }

    /* Check for Autoturnoff after period of time */
    timeNow = millis();
    if (  (
            ( global::GetClientConnected() == false && global::GetSerialConnected() == false) ||
            timeNow > (this->GetLastUpdate() + this->GetVideoTimeout())
          ) &&
          pStaticLedhelper->getFadeOutRunning() == false
       ) {
      _DBGPRINTF(PSTR(PSTR_LED "VIDEO Timeout %d ms, turn off\n"),
                 timeNow, this->GetLastUpdate(), this->GetVideoTimeout() );

      /* Start LED fadeOut */
      this->EnableTaskFadeOut();

      // Set Mode to OFF
      this->SetLedMode(LED_MODE_OFF);

      // Set Clients disconnected
      global::SetClientConnected(false);
      global::SetClientConnectedIpStr(NULL);
      global::SetSerialConnected(false);
    }

    // handle other tasks
    _yield();
  }
  else if (this->m_bLedMode == LED_MODE_RELAY) {
    /* Check for Autoturnoff after period of time */
    timeNow = millis();
    if ( timeNow > (this->GetLastUpdate() + this->GetVideoTimeout())) {
      _DBGPRINTF(PSTR(PSTR_LED "RELAY Timeout %d ms, turn off\n"),
                 timeNow, this->GetLastUpdate(), this->GetVideoTimeout() );
      /* Start LED fadeOut */
      this->EnableTaskFadeOut();

      // Set Mode to OFF
      this->SetLedMode(LED_MODE_OFF);

#if defined(ENABLE_DREAMSCREEN)
      // Disable DreamScreen Connected state to be on safe side
      global::m_pDreamScreen->SetConnected( false );
#endif
    }

    // handle other tasks
    _yield();
  }
#if defined(ESP32)
  else if (this->m_bLedMode == LED_MODE_DEMO) {
    // Create new TASKS if necessary
    if (handleTaskDemo == NULL) {
      // Create new Task
      xTaskCreatePinnedToCore(&TaskDemoCode,
                              "TaskDemo",
                              4096,
                              this,
                              DEFAULT_TASK_PRIORITY, /* +1 priority */
                              &handleTaskDemo,
                              ARDUINO_RUNNING_CORE /* SYSTEM_RUNNING_CORE */); /* pin task to core 1 */
      _DBGPRINTF(PSTR(PSTR_LED "TaskDemo created HANDLE %d\n"), handleTaskDemo);
    } else {
      int iStatus = eTaskGetState(handleTaskDemo);
      if (iStatus == eSuspended) {
        _DBGPRINTF(PSTR(PSTR_LED "TaskDemo suspended, resuming\n"));
        vTaskResume(handleTaskDemo);
      }
    }
  }
  else if (this->m_bLedMode == LED_MODE_AMBIENT) {
    if (global::GetAmbientPreset() != 0) {
      if (handleTaskAmbient == NULL) {
        xTaskCreatePinnedToCore(&TaskAmbientCode,
                                "TaskAmbient",
                                4096,
                                this,
                                DEFAULT_TASK_PRIORITY,
                                &handleTaskAmbient,
                                ARDUINO_RUNNING_CORE);
        _DBGPRINTF(PSTR(PSTR_LED "TaskAmbient created HANDLE %d\n"), handleTaskAmbient);
      } else {
        int iStatus = eTaskGetState(handleTaskAmbient);
        if (iStatus == eSuspended) {
          vTaskResume(handleTaskAmbient);
        }
      }
    } else {
      if (handleTaskAmbient != NULL) {
        int iStatus = eTaskGetState(handleTaskAmbient);
        if (iStatus != eSuspended) {
          vTaskSuspend(handleTaskAmbient);
        }
      }
    }
  }
  else if (this->m_bLedMode == LED_MODE_CAM) {
# if defined(ENABLE_CAM)
    CamTask_Start();
# endif
  }
#endif
}

bool LEDHelper::SetLedMode(byte bNew) {
  if (m_bLedMode != bNew) {
# if defined(ENABLE_DREAMSCREEN)
    if ( (bNew == LED_MODE_RELAY && m_bLedMode == LED_MODE_VIDEO) ||
         (bNew == LED_MODE_VIDEO && m_bLedMode == LED_MODE_RELAY) ) {
      // Allready in DREAMSCREEN or VIDEO Mode, SKIP!
      _DBGPRINTF_ULTRA(PSTR("LEDHelper:SetLedMode: Already in VIDEO/RELAY Mode\n"));
      return false;
    }
# endif

    // Set Mode if differs from current mode
    m_bLedMode = bNew;

    if (m_bLedMode != LED_MODE_OFF) {
# if defined(ENABLE_HUE)
      if (global::GetHueSupport() == 1) {
        _DBGPRINTF(PSTR("global:SetLedMode:Hue Enabled\n"));

        // ReInit Hue Lights on Mode change
        global::m_pHueHelper->InitHueLights();
        // Set to LOOP Mode
        global::m_pHueHelper->SetTaskMode(HUE_TASK_MODE_LOOP);
        // (Re)Start Hue Task if needed
        global::m_pHueHelper->InitHueTask();
      }
# endif

# if defined(ESP32_CPU_AUTOLOW)
      // Led Mode chaned
      if (getCpuFrequencyMhz() < 240) {
        _DBGPRINTF(PSTR("global:SetLedMode: LED MODE CHANGED, set CPU freq to 240 Mhz = %s!\n"),
                   (setCpuFrequencyMhz(240) ? "SUCCESS" : "FAIL") );
      }
# endif
    } else {
      // User switched to OFF: fade out LEDs instead of instant off
#if defined(ESP32)
      this->EnableTaskFadeOut();
#endif
# if defined(ESP32_CPU_AUTOLOW)
      // Led Mode turned off!
      if (global::GetCamAutoOnOff() == 1 && getCpuFrequencyMhz() > 80) {
        _DBGPRINTF(PSTR("global:SetLedMode: LED TURN OFF, reduce CPU freq to 80 Mhz = %s!\n"),
                   (setCpuFrequencyMhz(80) ? "SUCCESS" : "FAIL") );
      }
# endif

# if defined(ENABLE_RELAY)
      // Reconnect to original WIFI if in DIRECT MODE
      if (global::GetAmbibackWifiDirectMode() == 1 &&
          strcmp(WiFi.SSID().c_str(), global::GetWifiSSID()) != 0) {
        // Use this function to restore own WIFI
        global::ConnectWifiConfigured(global::GetWifiSSID(), global::GetWifiPassword(), false);
      }

      // Disable Relay INIT, force reinit next time!
      global::SetAmbibackRelaymodeInitialized(0);
# else
      /*
        // TODO: Handle different!
            // Make sure connected device is turned OFF as well!
            if (global::GetClientConnected()) {
              global::SetRemoteLEDMode(global::GetClientConnectedIpStr(), LED_MODE_OFF);
            }
      */
# endif
    }

    global::m_pEepromHelper->SetSettingByte("LedMode", m_bLedMode);
    _DBGPRINTF(PSTR(PSTR_LED "SetLedMode to %d\n"), m_bLedMode);

    // Do a little Sleep before BROADCAST!
    _taskDelay(500);

    // --- New send broadcast about Mode Change!
    char szBroadcast[512];
    size_t stLen = 512;
    stLen = global::GetDeviceInfo(&szBroadcast[0], stLen);

    // Send broadcast out
    _DBGPRINTF(PSTR("global:SetLedMode: Sending BROADCAST about Status change: %s\n"), szBroadcast, stLen );
    UdpServer::m_pUdpServer->SendUdpBroadcast((byte*) szBroadcast, stLen);
    // ---
  }
# if defined(ENABLE_HUE)
  else if (m_bLedMode != LED_MODE_OFF) {
    if (global::GetHueSupport() == 1) {
      // (Re)Start Hue Task if needed
      global::m_pHueHelper->InitHueTask();
    }
  }
# endif

  // Change Success
  return true;
}

const char* LEDHelper::GetLedModeStr(byte bLedMode) {
  switch (bLedMode) {
    case LED_MODE_OFF:
      return PSTR("Off");
      break;
    case LED_MODE_VIDEO:
      return PSTR("Video");
      break;
    case LED_MODE_AMBIENT:
      return PSTR("Ambient");
      break;
    case LED_MODE_CAM:
      return PSTR("Cam");
      break;
    case LED_MODE_RELAY:
      return PSTR("Relay");
      break;
    default:
      return PSTR("[UNKNOWN]");
      break;
  }
}

void LEDHelper::SetFrameLength(byte bNew, bool bEpprom) {
  // CHANGED: Framelength minimum to 20, we need about 16ms for 2x 250 LEDs
  if (  bNew >= DEFAULT_FRAME_LENGTH &&
        bNew <= MAX_FRAME_LENGTH &&
        bNew % 5 == 0) {
    m_bFrameLength = bNew;
    if (bEpprom)
      global::m_pEepromHelper->SetSettingByte(FRAME_LENGTH_START, bNew);
    _DBGPRINTF(PSTR(PSTR_LED "SetFrameLength to %d\n"), bNew);
  } else {
    _DBGPRINTF(PSTR(PSTR_LED "SetFrameLength invalid value %d\n"), bNew);
  }
}

void LEDHelper::SetDelayLength(int iNew, bool bEpprom) {
  if (  iNew >= DEFAULT_FRAME_LENGTH &&
        iNew <= MAX_DELAY_LENGTH &&
        iNew % 5 == 0 &&
        iNew >= m_bFrameLength ) {
    m_iDelayLength = iNew;
    if (bEpprom)
      global::m_pEepromHelper->SetSettingInt(DELAY_LENGTH_START, iNew);
    _DBGPRINTF_ULTRA(PSTR("LEDHelper: SetDelayLength to %d\n"), iNew);
  } else {
    _DBGPRINTF(PSTR(PSTR_LED "SetDelayLength invalid value %d\n"), iNew);
  }
}

void LEDHelper::SetFrameSmoothing(byte bNew, bool bEpprom) {
  if (  bNew >= DEFAULT_FRAME_SMOOTHING &&
        bNew <= DEFAULT_FRAME_MAX) {
    m_bFrameSmoothing = bNew;

    // Free memory first - crash happens before saving to EEPROM!
    if (m_pLedFrames != NULL) {
      _DBGPRINTF(PSTR(PSTR_LED "SetFrameSmoothing Delete old MEMORY\n"));
      free(m_pLedFrames);
      m_pLedFrames = NULL;
    }
    // Save to EEPROM
    if (bEpprom)
      global::m_pEepromHelper->SetSettingByte(FRAME_SMOOTHING_START, bNew);
    _DBGPRINTF_ULTRA(PSTR("LEDHelper: SetFrameSmoothing to %d\n"), bNew);

    // Allocate memory for LED Smoothing if enabled
    if (m_bFrameSmoothing > DEFAULT_FRAME_SMOOTHING && m_bFrameSmoothing <= DEFAULT_FRAME_MAX) {
      int iArraySize = (MAX_LEDS * sizeof(struct CRGB) * (m_bFrameSmoothing + 1));
      m_pLedFrames = (CRGB*) _malloc_psram( iArraySize ); // add some dummy reserve bytes
      if (m_pLedFrames) {
        memset(m_pLedFrames, 0, iArraySize);
        _DBGPRINTF(PSTR("LEDHelper: SetFrameSmoothing Create LedFrames Array with %d bytes\n"), iArraySize);
      }
    }
  } else {
    _DBGPRINTF(PSTR("LEDHelper: SetFrameSmoothing invalid value %d\n"), bNew);
  }
}
void* LEDHelper::GetVoidLedFramesPtr() {
  return m_pLedFrames;
}

void LEDHelper::SetBrightness(int iNew, bool bEpprom) {
  m_iBrightness = iNew;
  FastLED.setBrightness(m_iBrightness);
  if (bEpprom)
    global::m_pEepromHelper->SetSettingInt(COLOR_BRIGHTNESS_START, iNew);
  _DBGPRINTF(PSTR(PSTR_LED "SetBrightness to %d\n"), m_iBrightness);
}

void LEDHelper::SetMinimumLuminosity(byte bNew, bool bEpprom) {
  // MinimumLuminosity is handeled ProcessLedNextStep!
  m_bMinimumLuminosity = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte(COLOR_MIN_LUMIN_START, bNew);
  _DBGPRINTF(PSTR(PSTR_LED "SetMinimumLuminosity to %d\n"), m_bMinimumLuminosity);
}

void LEDHelper::SetColorBoost(byte bNew, bool bEpprom) {
  if (  bNew >= LED_MAX_COLORBOOST )
    m_bColorBoost  = LED_MAX_COLORBOOST;
  else
    m_bColorBoost  = bNew;

  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte(COLOR_BOOST_START, bNew);
  _DBGPRINTF(PSTR(PSTR_LED "SetColorBoost to %d\n"), m_bColorBoost);
}

void LEDHelper::SetColorBoostBalance(byte bNew, bool bEpprom) {
  if (  bNew >= LED_MAX_COLORBOOST )
    m_bColorBoostBalance  = LED_MAX_COLORBOOST;
  else
    m_bColorBoostBalance  = bNew;

  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("ColorBoostBala", bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetColorBoostBalance to %d\n"), m_bColorBoostBalance);
}

void LEDHelper::SetColorBoostMin(byte bNew, bool bEpprom) {
  if (  bNew >= LED_MAX_COLORBOOST )
    m_bColorBoostMin  = LED_MAX_COLORBOOST;
  else
    m_bColorBoostMin  = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte(COLOR_BOOST_MIN_START, bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetColorBoostMin to %d\n"), m_bColorBoostMin);
}

void LEDHelper::SetColorBoostThreshold(byte bNew, bool bEpprom) {
  // ColorBoostThreshold is handeled ProcessLedNextStep!
  m_bColorBoostThreshold = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("ColorBoostThres", bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetColorBoostThreshold to %d\n"), m_bColorBoostThreshold);
}

void LEDHelper::SetColorBoostRed(byte bNew, bool bEpprom) {
  if (  bNew >= LED_MAX_COLORBOOST )
    m_bColorBoostRed  = LED_MAX_COLORBOOST;
  else
    m_bColorBoostRed  = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("ColorBoostRed", bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetColorBoostRed to %d\n"), m_bColorBoostRed);
}
void LEDHelper::SetColorBoostGreen(byte bNew, bool bEpprom) {
  if (  bNew >= LED_MAX_COLORBOOST )
    m_bColorBoostGreen  = LED_MAX_COLORBOOST;
  else
    m_bColorBoostGreen  = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("ColorBoostGreen", bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetColorBoostGreen to %d\n"), m_bColorBoostGreen);
}
void LEDHelper::SetColorBoostBlue(byte bNew, bool bEpprom) {
  if (  bNew >= LED_MAX_COLORBOOST )
    m_bColorBoostBlue  = LED_MAX_COLORBOOST;
  else
    m_bColorBoostBlue  = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("ColorBoostBlue", bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetColorBoostBlue to %d\n"), m_bColorBoostBlue);
}

void LEDHelper::SetColorValueGain(int iNew, bool bEpprom) {
  // TODO: ColorValueGain is handeled ProcessLedNextStep!
  m_iColorValueGain = iNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingInt(COLOR_VALUE_GAIN_START, iNew);
  _DBGPRINTF(PSTR("LEDHelper: SetColorValueGain to %d\n"), m_iColorValueGain);
}

void LEDHelper::SetColorCorRed(byte bNew, bool bEpprom) {
  m_bColorCorRed = bNew;
  _DBGPRINTF(PSTR("LEDHelper: SetColorCorRed to %d\n"), m_bColorCorRed);
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte(COLOR_COR_RED_START, bNew);
  FastLED.setCorrection(CRGB( m_bColorCorRed, m_bColorCorGreen, m_bColorCorBlue));
}

void LEDHelper::SetColorCorGreen(byte bNew, bool bEpprom) {
  m_bColorCorGreen = bNew;
  _DBGPRINTF(PSTR("LEDHelper: SetColorCorGreen to %d\n"), m_bColorCorGreen);
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte(COLOR_COR_GREEN_START, bNew);
  FastLED.setCorrection(CRGB( m_bColorCorRed, m_bColorCorGreen, m_bColorCorBlue));
}

void LEDHelper::SetColorCorBlue(byte bNew, bool bEpprom) {
  m_bColorCorBlue = bNew;
  _DBGPRINTF(PSTR("LEDHelper: SetColorCorBlue to %d\n"), m_bColorCorBlue);
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte(COLOR_COR_BLUE_START, bNew);
  FastLED.setCorrection(CRGB( m_bColorCorRed, m_bColorCorGreen, m_bColorCorBlue));
}

void LEDHelper::SetSetupVertical(byte bNew, bool bEpprom) {
  m_bSetupVertical = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte(SETUP_VERTICAL_START, bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetSetupVertical to %d\n"), m_bSetupVertical);

  // Update LedCount
  this->UpdateCurrentLedCount();
}

void LEDHelper::SetSetupHorizontal(byte bNew, bool bEpprom) {
  m_bSetupHorizontal = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte(SETUP_HORIZONTAL_START, bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetSetupHorizontal to %d\n"), m_bSetupHorizontal);

  // Update LedCount
  this->UpdateCurrentLedCount();
}

void LEDHelper::SetSetupDirection(byte bNew, bool bEpprom) {
  m_bSetupDirection = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte(SETUP_DIRECTION_START, bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetSetupDirection to %d\n"), m_bSetupDirection);

  // Update LedCount
  this->UpdateCurrentLedCount();
}

void LEDHelper::SetSetupSideLeft(byte bNew, bool bEpprom) {
  m_bSetupSideLeft = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte(SETUP_SIDELEFT_START, bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetSetupSideLeft to %d\n"), m_bSetupSideLeft);
}

void LEDHelper::SetSetupSideTop(byte bNew, bool bEpprom) {
  m_bSetupSideTop = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte(SETUP_SIDETOP_START, bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetSetupSideTop to %d\n"), m_bSetupSideTop);
}

void LEDHelper::SetSetupSideRight(byte bNew, bool bEpprom) {
  m_bSetupSideRight = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte(SETUP_SIDERIGHT_START, bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetSetupSideRight to %d\n"), m_bSetupSideRight);
}

void LEDHelper::SetSetupSideBottom(byte bNew, bool bEpprom) {
  m_bSetupSideBottom = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte(SETUP_SIDEBOTTOM_START, bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetSetupSideBottom to %d\n"), m_bSetupSideBottom);
}

void LEDHelper::SetSideLeftBrightness(byte bNew, bool bEpprom) {
  m_bSideLeftBrightness = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("SideLeftBright", bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetSideLeftBrightness to %d\n"), m_bSideLeftBrightness);
}

void LEDHelper::SetSideTopBrightness(byte bNew, bool bEpprom) {
  m_bSideTopBrightness = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("SideTopBright", bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetSideTopBrightness to %d\n"), m_bSideTopBrightness);
}

void LEDHelper::SetSideRightBrightness(byte bNew, bool bEpprom) {
  m_bSideRightBrightness = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("SideRightBright", bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetSideRightBrightness to %d\n"), m_bSideRightBrightness);
}

void LEDHelper::SetSideBottomBrightness(byte bNew, bool bEpprom) {
  m_bSideBottomBrightness = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("SideBtmBright", bNew);
  _DBGPRINTF(PSTR("LEDHelper: SetSideBottomBrightness to %d\n"), m_bSideBottomBrightness);
}
// --- END PROPERTIES

void LEDHelper::SetLedColorByIdx(int iIndex, byte bWhite, byte bRed, byte bGreen, byte bBlue) {
  //      m_leds[iIndex].w = bWhite;
  m_leds_next[iIndex].r = bRed;
  m_leds_next[iIndex].g = bGreen;
  m_leds_next[iIndex].b = bBlue;
}

void LEDHelper::ZeroConfiguredLeds() {
  /* ZERO unconfigured LEDs */
  for (int idx = this->getNumLeds(); idx < MAX_LEDS; idx++) {
    m_leds_next[idx].r = 0;
    m_leds_next[idx].g = 0;
    m_leds_next[idx].b = 0;
  }
}

void LEDHelper::SetCurrentLedCount(uint16_t count) {
  // Encode as count-1 so 1..256 fits in byte (0=1 LED, 255=256 LEDs)
  if (count == 0)
    m_numleds = 0;   // decode as 1 LED
  else if (count > 256)
    m_numleds = 255; // 256 LEDs
  else
    m_numleds = (byte)(count - 1);
}

void LEDHelper::UpdateCurrentLedCount() {
  int total = (int)m_bSetupHorizontal * 2 + (int)m_bSetupVertical * 2;
  uint16_t capped = (uint16_t)(total > 256 ? 256 : (total < 1 ? 1 : total));
  this->SetCurrentLedCount(capped);
  _DBGPRINTF(PSTR("LEDHelper: UpdateCurrentLedCount to %u\n"), (unsigned)this->getNumLeds());
}

void LEDHelper::ResetLedColors() {
  memset(m_leds1, 0, MAX_LEDS * sizeof(struct CRGB));
#ifdef USE_HAS_SECOND_PORT
  memset(m_leds2, 0, MAX_LEDS * sizeof(struct CRGB));
#endif
}

void LEDHelper::ResetNextLedColors() {
  memset(m_leds_next, 0, MAX_LEDS * sizeof(struct CRGB));
}

void LEDHelper::ShowLedColors() {
  // Set last Update timestamp!
  this->SetLastUpdate();

#if defined(ENABLE_VIDEO)
  // shows new values
  _DBGPRINTF_ULTRA(PSTR("LEDHelper:ShowLedColors BEFORE showLeds\n"));
  this->SetShowLedActive(true);
  // Draw led data for the first strand into leds
  //  m_controllers[LED_PRT1IDX]->showLeds(m_iBrightness);
  FastLED.show(m_iBrightness);
# if defined(USE_HAS_SECOND_PORT) || defined(CLONE_TO_SECOND_PORT)
  dsfgsdfgrg TODO CHECK CODE, we may can ShowLeds at ONCE!
  // TODO Later, caused 8ms more processing time per frame
  if (m_controllers[LED_PRT2IDX] != NULL)
    //    m_controllers[LED_PRT2IDX]->showLeds(m_iBrightness);
    FastLED.show(m_iBrightness);
# endif
  this->SetShowLedActive(false);
  _DBGPRINTF_ULTRA(PSTR("LEDHelper:ShowLedColors AFTER showLeds\n"));
#else
  _DBGPRINTF_ULTRA(PSTR("LEDHelper:ShowLedColors VIDEO MODE DISABLED\n"));
#endif
}

void LEDHelper::FadeOut(int iFadeOutLength) {
  // Calc steps for turn off
  int iSteps = iFadeOutLength / m_bFrameLength;
  unsigned long nDelayLengthLeft = iFadeOutLength;
  unsigned long nTimeNow = 0;
  unsigned long nTimePassed = 0;
  long nExtraSleep = 0;

  /* Get copy of m_leds_next */
  CRGB leds_next[MAX_LEDS];
  memcpy(leds_next, m_leds_next, MAX_LEDS * sizeof(struct CRGB));

  for (int i = iSteps; i > 0; i--)
  {
    nTimeNow = millis();

    /* calc new LED interrim colors */
    for (int j = 0; j < MAX_LEDS /* ALL LEDs m_numleds */; j++)
    {
      /* Calc next Colors**/
      m_leds1[j].r = (m_leds1[j].r > 1) ? m_leds1[j].r -= 2 : 0;
      m_leds1[j].g = (m_leds1[j].g > 1) ? m_leds1[j].g -= 2 : 0;
      m_leds1[j].b = (m_leds1[j].b > 1) ? m_leds1[j].b -= 2 : 0;
    }

    /* Activate new LED Colors */
    this->ShowLedColors();

    /* Check how long we need to sleep for next frame */
    nTimePassed = millis() - nTimeNow;
    nDelayLengthLeft -= nTimePassed;
    nExtraSleep = m_bFrameLength - nTimePassed;

    /* Sleep only if necessary */
    if (  nTimePassed < m_bFrameLength &&
          nExtraSleep > 0 &&
          i > 1 ) {
      _taskDelay(nExtraSleep);
      nDelayLengthLeft -= nExtraSleep;
    } else
      nExtraSleep = 0;

    /* Debug Output */
    _DBGPRINTF_ULTRA(PSTR("LEDHelper: FadeOut Step %d of %d, TimePassed %d ms, ExtraSleep %d ms, Delay left %d\n"), i, iSteps, nTimePassed, nExtraSleep, nDelayLengthLeft);
  }

  _DBGPRINTF(PSTR("LEDHelper: FadeOut FPS - %d\n"), FastLED.getFPS());
}

#if defined(ENABLE_HUE)
/* Unified sector calc: divide LED range [iStartIdx,iEndIdx) into numSectors, average RGB per sector, write to Hue buffer.
   pHueIndices: array of Hue LED indices (length numSectors), cornerA/B + bFirstA/B for merge-at-corner logic. */
static void hueCalcSectorsSide(CRGB* pLeds, CRGB* pLedsHue, int iStartIdx, int iEndIdx, int numSectors,
    const byte* pHueIndices, byte cornerA, byte cornerB, bool bFirstA, bool bFirstB, const char* pSideName) {
  int iRed = 0, iGreen = 0, iBlue = 0;
  int iStep = (iEndIdx - iStartIdx) / numSectors;
  if (iStep < 1) iStep = 1;
  int iNumSec = 1;
  int iSecIdx = 0;
  _DBGPRINTF_ULTRA(PSTR("LEDHelper:hueCalcSectorsSide %s ENTER %p / %p / %d / %d\n"), pSideName, pLeds, global::m_pHueHelper->getLedsHuePtr(), iStep, numSectors);

  for (int j = iStartIdx; j < iEndIdx; j++) {
    if (iSecIdx >= numSectors) break;
    int iHueIdx = pHueIndices[iSecIdx];
    if (iHueIdx >= MAX_HUE_LEDS) {
      global::errprintf_P(PSTR("LEDHelper:hueCalcSectorsSide %s iHueIdx %d >= MAX_HUE_LEDS %d\n"), pSideName, iHueIdx, MAX_HUE_LEDS);
      break;
    }
    if (iNumSec % iStep > 0 && j != (iEndIdx - 1)) {
      iRed += pLeds[j].r;
      iGreen += pLeds[j].g;
      iBlue += pLeds[j].b;
      iNumSec++;
    } else {
      iRed = iRed / iNumSec;
      iGreen = iGreen / iNumSec;
      iBlue = iBlue / iNumSec;
      byte r = (byte)(iRed < 255 ? (iRed > 0 ? iRed : 0) : 255);
      byte g = (byte)(iGreen < 255 ? (iGreen > 0 ? iGreen : 0) : 255);
      byte b = (byte)(iBlue < 255 ? (iBlue > 0 ? iBlue : 0) : 255);
      bool bMerge = (!bFirstA && iHueIdx == cornerA) || (!bFirstB && iHueIdx == cornerB);
      if (bMerge) {
        pLedsHue[iHueIdx].r = (byte)((r + pLedsHue[iHueIdx].r) / 2);
        pLedsHue[iHueIdx].g = (byte)((g + pLedsHue[iHueIdx].g) / 2);
        pLedsHue[iHueIdx].b = (byte)((b + pLedsHue[iHueIdx].b) / 2);
      } else {
        pLedsHue[iHueIdx].r = r;
        pLedsHue[iHueIdx].g = g;
        pLedsHue[iHueIdx].b = b;
      }
      _DBGPRINTF_ULTRA(PSTR("LEDHelper:hueCalcSectorsSide %s(Idx:%d) NumSec:%d RGB:%d/%d/%d\n"), pSideName, iHueIdx, iNumSec, iRed, iGreen, iBlue);
      iRed = iGreen = iBlue = 0;
      iNumSec = 1;
      iSecIdx++;
    }
  }
}

void LEDHelper::hueCalcSectorsLeft(void* pVoidLeds, void* pVoidLedsHue, int iStartIdx, int iEndIdx, bool bReverse, bool bFirstTop, bool bFirstBottom) {
  static const byte hueLeft[] = { HUE_LED_BOTTOM_LEFT, HUE_LED_MIDDLE_LEFT, HUE_LED_TOP_LEFT };
  static const byte hueLeftRev[] = { HUE_LED_TOP_LEFT, HUE_LED_MIDDLE_LEFT, HUE_LED_BOTTOM_LEFT };
  CRGB* pLeds = (CRGB*)pVoidLeds;
  CRGB* pLedsHue = (CRGB*)pVoidLedsHue;
  hueCalcSectorsSide(pLeds, pLedsHue, iStartIdx, iEndIdx, HUE_LED_LEFT_SECTORS,
      bReverse ? hueLeftRev : hueLeft, HUE_LED_TOP_LEFT, HUE_LED_BOTTOM_LEFT, bFirstTop, bFirstBottom, "Left");
}
void LEDHelper::hueCalcSectorsRight(void* pVoidLeds, void* pVoidLedsHue, int iStartIdx, int iEndIdx, bool bReverse, bool bFirstTop, bool bFirstBottom) {
  static const byte hueRight[] = { HUE_LED_TOP_RIGHT, HUE_LED_MIDDLE_RIGHT, HUE_LED_BOTTOM_RIGHT };
  static const byte hueRightRev[] = { HUE_LED_BOTTOM_RIGHT, HUE_LED_MIDDLE_RIGHT, HUE_LED_TOP_RIGHT };
  CRGB* pLeds = (CRGB*)pVoidLeds;
  CRGB* pLedsHue = (CRGB*)pVoidLedsHue;
  hueCalcSectorsSide(pLeds, pLedsHue, iStartIdx, iEndIdx, HUE_LED_RIGHT_SECTORS,
      bReverse ? hueRightRev : hueRight, HUE_LED_TOP_RIGHT, HUE_LED_BOTTOM_RIGHT, bFirstTop, bFirstBottom, "Right");
}
void LEDHelper::hueCalcSectorsBottom(void* pVoidLeds, void* pVoidLedsHue, int iStartIdx, int iEndIdx, bool bReverse, bool bFirstLeft, bool bFirstRight) {
  static const byte hueBottom[] = { HUE_LED_BOTTOM_RIGHT, HUE_LED_BOTTOM_CENTER3, HUE_LED_BOTTOM_CENTER2, HUE_LED_BOTTOM_CENTER1, HUE_LED_BOTTOM_LEFT };
  static const byte hueBottomRev[] = { HUE_LED_BOTTOM_LEFT, HUE_LED_BOTTOM_CENTER1, HUE_LED_BOTTOM_CENTER2, HUE_LED_BOTTOM_CENTER3, HUE_LED_BOTTOM_RIGHT };
  CRGB* pLeds = (CRGB*)pVoidLeds;
  CRGB* pLedsHue = (CRGB*)pVoidLedsHue;
  hueCalcSectorsSide(pLeds, pLedsHue, iStartIdx, iEndIdx, HUE_LED_BOTTOM_SECTORS,
      bReverse ? hueBottomRev : hueBottom, HUE_LED_BOTTOM_LEFT, HUE_LED_BOTTOM_RIGHT, bFirstLeft, bFirstRight, "Bottom");
}
void LEDHelper::hueCalcSectorsTop(void* pVoidLeds, void* pVoidLedsHue, int iStartIdx, int iEndIdx, bool bReverse, bool bFirstLeft, bool bFirstRight) {
  static const byte hueTop[] = { HUE_LED_TOP_LEFT, HUE_LED_TOP_CENTER1, HUE_LED_TOP_CENTER2, HUE_LED_TOP_CENTER3, HUE_LED_TOP_RIGHT };
  static const byte hueTopRev[] = { HUE_LED_TOP_RIGHT, HUE_LED_TOP_CENTER3, HUE_LED_TOP_CENTER2, HUE_LED_TOP_CENTER1, HUE_LED_TOP_LEFT };
  CRGB* pLeds = (CRGB*)pVoidLeds;
  CRGB* pLedsHue = (CRGB*)pVoidLedsHue;
  hueCalcSectorsSide(pLeds, pLedsHue, iStartIdx, iEndIdx, HUE_LED_TOP_SECTORS,
      bReverse ? hueTopRev : hueTop, HUE_LED_TOP_LEFT, HUE_LED_TOP_RIGHT, bFirstLeft, bFirstRight, "Top");
}
#endif

#if defined(ENABLE_RELAY)
void LEDHelper::RelayPrepareDirectLedData(void) {
  /* Preprocess Led Colors also in RELAY Mode now! */
  this->PreProcessLedColors(&m_leds_next[0]);

  /* Raw copy of m_leds_next into m_leds1 */
  memcpy(m_leds1, m_leds_next, MAX_LEDS * sizeof(struct CRGB));

  /* ZERO unused LEDs */
  for (int k = (int)this->getNumLeds(); k < MAX_LEDS; k++) {
    m_leds1[k].r = 0;
    m_leds1[k].g = 0;
    m_leds1[k].b = 0;
  }
}

bool LEDHelper::RelayConnect(void) {
  if (pTcpClient == NULL) {
    pTcpClient = new WiFiClient();
    pTcpClient->setTimeout(1);
    // pTcpClient->setNoDelay(true);
    m_bClientConnected = false;
  }

  // Try to connect
  if (!pTcpClient->connected()) {
    // Try to connect
    if (pTcpClient->connect(global::GetAmbibackRelayIPAddr(), TCP_PORT) == 1) {
      _DBGPRINTF(PSTR("LEDHelper:RelayConnect: TCP Connect to %s:%d SUCCESS!\n"), global::GetAmbibackRelayIPAddrChar(), TCP_PORT);
      m_bClientConnected = true;
    } else {
      _ERRPRINTF(PSTR("LEDHelper:RelayConnect: TCP Connect to %s:%d FAIL!\n"), global::GetAmbibackRelayIPAddrChar(), TCP_PORT);
      m_bClientConnected = false;
    }
  } else {
    m_bClientConnected = true;
  }
  // Return Connected State
  return m_bClientConnected;
}
void LEDHelper::RelayDisconnect(void) {
  if (pTcpClient != NULL) {
    _DBGPRINTF(PSTR("LEDHelper:RelayDisconnect: Closing TCP Connection to %s:%d\n"), global::GetAmbibackRelayIPAddrChar(), TCP_PORT);
    pTcpClient->stop();
    delete pTcpClient;
    pTcpClient = NULL;
    m_bClientConnected = false;
  }
}

void LEDHelper::RelayLedData() {
  // Check for relay to be initialized?!
  bool bRelayInitialized = global::GetAmbibackRelaymodeInitialized();
  if ( !bRelayInitialized )
    bRelayInitialized = global::InitializeAmbibackRelaymode();

  if ( bRelayInitialized ) {
    // Create Buffer on stack for LED Data
    byte bBuf[MAX_PAKET_LEDDATA_LENGTH];
    byte* pbBuf = &bBuf[0]; // new byte[MAX_PAKET_LEDDATA_LENGTH];
    size_t stLenPaket = this->CreateLedNetPacket(pbBuf, MAX_PAKET_LEDDATA_LENGTH);
    bool bSendSuccess = false;
    this->IncrementSequence(); // Increment sequence number

    // Send LED Data
    if (global::GetAmbibackRelayMode() == RELAY_UDP) {
      // Disconnect if not in TCP Mode
      if (this->GetRelayClientConnected()) {
        this->RelayDisconnect();
      }

      if (global::g_timeNowLastNetworkErr > millis()) {
        return;
      }

      // Send UDP
      bSendSuccess = UdpServer::m_pUdpServer->SendUdpMessage(pbBuf, stLenPaket, global::GetAmbibackRelayIPAddr(), UDP_PORT);
      if (bSendSuccess) {
        _DBGPRINTF_ULTRA(PSTR("LEDHelper:RelayLedData: UDP Message to %s send SUCCESS!\n"), global::GetAmbibackRelayIPAddrChar() );
      } else {
        if (WiFi.isConnected() && millis() > global::g_timeNowLastNetworkErr ) {
          // Show Error only if WIFI is connected
          _ERRPRINTF(PSTR("LEDHelper:RelayLedData: UDP Message to %s datalen %d send FAIL with Status=%#08x(%s) !\n"),
                     global::GetAmbibackRelayIPAddrChar(),
                     stLenPaket,
                     UdpServer::m_pUdpServer->GetLastErr(),
                     esp_err_to_name(UdpServer::m_pUdpServer->GetLastErr()) );
          global::g_timeNowLastNetworkErr = millis() + 250; // BLOCK sending for 250ms | COOLDOWN!
        }
      }
    } else if (global::GetAmbibackRelayMode() == RELAY_TCP) {
      if (this->RelayConnect()) {
        size_t stLenSend = pTcpClient->write((const uint8_t*) pbBuf, stLenPaket);
        pTcpClient->flush();
        if (stLenSend > 0) {
          _DBGPRINTF_ULTRA(PSTR("LEDHelper:RelayLedData: TCP Message to %s with %d bytes send SUCCESS!\n"), global::GetAmbibackRelayIPAddrChar(), stLenSend);
          bSendSuccess = true;
          /*         while(client.available()) {
                       // DROP DATA
                     } */
        } else {
          _ERRPRINTF(PSTR("LEDHelper:RelayLedData: TCP Message to %s with %d bytes send FAIL! - FORCE RECONNECT\n"), global::GetAmbibackRelayIPAddrChar(), stLenPaket);
          this->RelayDisconnect();
        }
      }
    } else if (global::GetAmbibackRelayMode() == RELAY_BT) {
      _ERRPRINTF(PSTR("LEDHelper:RelayLedData: RELAY_BT NOT IMPLEMENTED YET!\n"));
    } else {
      _ERRPRINTF(PSTR("LEDHelper:RelayLedData: UNKNOWN RELAY MODE %d!\n"), global::GetAmbibackRelayMode());
    }

    // Handle Relay Failure count
    if (bSendSuccess) {
      global::ResetAmbibackRelayFailed();
    } else {
      global::IncrementAmbibackRelayFailed();
    }

    // Free memory
    // delete[] pbBuf;
  } else {
    global::errprintf_P(PSTR("LEDHelper:RelayLedData: InitializeAmbibackRelaymode failed!\n"));

    // Sleep till next step
    _taskDelay(AMBIBACK_INIT_RETRY_WAIT);
  }
}

// MAX_PAKET_LEDDATA_LENGTH
size_t LEDHelper::CreateLedNetPacket(byte* pbBuf, size_t stBufLen) {
  size_t lenReturn = 0;
  byte* ptr = pbBuf;
  unsigned short shSequence = this->GetSequenceNumber();
  byte bSequenceLow = (byte)(shSequence & 0x00ff);
  byte bSequenceHigh = (byte)(shSequence >> 8);

  // Fixed Startbytes
  *ptr++ = 0x00;
  *ptr++ = 0xFF;
  *ptr++ = 0x13;
  *ptr++ = 0x37;
  *ptr++ = this->m_numleds; // Stored count-1 (0=1 LED .. 255=256 LEDs); receiver decodes as (byte+1)
  *ptr++ = bSequenceLow;    // Sequence LOW value, before 0x00 StartIndex
  *ptr++ = 0x01;            // enable CRC Check
  *ptr++ = 0x00;            // CRC check, being calced at the end
  lenReturn += NET_HEADLEN;

  // add pixels to array! (getNumLeds() = m_numleds+1, so 1..256)
  for (int bLed = 0; bLed < (int)this->getNumLeds(); bLed++)
  {
    *ptr++ = (bLed == 0 ? bSequenceHigh : 0x00);  // First byte used as sequence High, others are reserved bytes
    *ptr++ = m_leds1[bLed].r;  // RED byte
    *ptr++ = m_leds1[bLed].g;  // GREEN byte
    *ptr++ = m_leds1[bLed].b;  // BLUE byte
    lenReturn += NET_CHUNKSIZE;
    if ((lenReturn + NET_CHUNKSIZE) >= stBufLen) {
      global::errprintf_P(PSTR("LEDHelper:CreateLedNetPacket: insufficient BUFFER %d/%d \n"), lenReturn, stBufLen);
    }
  }

  // Finally calc checksum
  *(pbBuf + 7) = (byte) global::crc_8( pbBuf + NET_HEADLEN, lenReturn - NET_HEADLEN );

  return lenReturn;
}
#endif

/* LED PostProcess code like disabling LED sides. */
void LEDHelper::PostProcessLedColors(void* pLeds) {
  CRGB* pLedsNext = (CRGB*) pLeds;

  _DBGPRINTF_ULTRA(PSTR("LEDHelper: PostProcessLedColors ENTER\n"));

#if defined(ENABLE_HUE)
  // Handle HUE color stuff first
  CRGB* pLedsHue = (CRGB*) global::m_pHueHelper->getLedsHuePtr();
  byte ledsHueMax = global::m_pHueHelper->getLedsMax();
  int iStartIdx = 0;
  int iEndIdx = 0;
  int iStepIdx = 0;
  //_DBGPRINTF(PSTR("LEDHelper:PostProcessLedColors ENTER %p / %p / %d\n"), pLeds, global::m_pHueHelper->getLedsHuePtr(), ledsHueMax);

  // --- Calc Hue Sectors
  switch (this->GetSetupDirection()) {
    case Left_Clock_bottom_left_up:
      iEndIdx = this->GetSetupVertical();
      this->hueCalcSectorsLeft(pLeds, pLedsHue, iStartIdx, iEndIdx, false, true, true);   iStartIdx = iEndIdx; iEndIdx += this->GetSetupHorizontal();
      this->hueCalcSectorsTop(pLeds, pLedsHue, iStartIdx, iEndIdx, false, false, true);   iStartIdx = iEndIdx; iEndIdx += this->GetSetupVertical();
      this->hueCalcSectorsRight(pLeds, pLedsHue, iStartIdx, iEndIdx, false, false, true); iStartIdx = iEndIdx; iEndIdx += this->GetSetupHorizontal();
      this->hueCalcSectorsBottom(pLeds, pLedsHue, iStartIdx, iEndIdx, false, false, false);
      break;
    case Top_Clock_top_left_right:
      iEndIdx = this->GetSetupHorizontal();
      this->hueCalcSectorsTop(pLeds, pLedsHue, iStartIdx, iEndIdx, false, true, true);    iStartIdx = iEndIdx; iEndIdx += this->GetSetupVertical();
      this->hueCalcSectorsRight(pLeds, pLedsHue, iStartIdx, iEndIdx, false, false, true); iStartIdx = iEndIdx; iEndIdx += this->GetSetupHorizontal();
      this->hueCalcSectorsBottom(pLeds, pLedsHue, iStartIdx, iEndIdx, false, false, true); iStartIdx = iEndIdx; iEndIdx += this->GetSetupVertical();
      this->hueCalcSectorsLeft(pLeds, pLedsHue, iStartIdx, iEndIdx, false, false, false);
      break;
    case Right_Clock_top_right_down:
      iEndIdx = this->GetSetupVertical();
      this->hueCalcSectorsRight(pLeds, pLedsHue, iStartIdx, iEndIdx, false, true, true);  iStartIdx = iEndIdx; iEndIdx += this->GetSetupHorizontal();
      this->hueCalcSectorsBottom(pLeds, pLedsHue, iStartIdx, iEndIdx, false, true, false); iStartIdx = iEndIdx; iEndIdx += this->GetSetupVertical();
      this->hueCalcSectorsLeft(pLeds, pLedsHue, iStartIdx, iEndIdx, false, true, false);  iStartIdx = iEndIdx; iEndIdx += this->GetSetupHorizontal();
      this->hueCalcSectorsTop(pLeds, pLedsHue, iStartIdx, iEndIdx, false, false, false);
      break;
    case Bottom_Clock_bottom_right_left:
      iEndIdx = this->GetSetupHorizontal();
      this->hueCalcSectorsBottom(pLeds, pLedsHue, iStartIdx, iEndIdx, false, true, true); iStartIdx = iEndIdx; iEndIdx += this->GetSetupVertical();
      this->hueCalcSectorsLeft(pLeds, pLedsHue, iStartIdx, iEndIdx, false, true, false);  iStartIdx = iEndIdx; iEndIdx += this->GetSetupHorizontal();
      this->hueCalcSectorsTop(pLeds, pLedsHue, iStartIdx, iEndIdx, false, false, true);   iStartIdx = iEndIdx; iEndIdx += this->GetSetupVertical();
      this->hueCalcSectorsRight(pLeds, pLedsHue, iStartIdx, iEndIdx, false, false, false);
      break;
    case Left_Cclock_top_left_down:
      iEndIdx = this->GetSetupHorizontal();
      this->hueCalcSectorsLeft(pLeds, pLedsHue, iStartIdx, iEndIdx, true, true, true);    iStartIdx = iEndIdx; iEndIdx += this->GetSetupVertical();
      this->hueCalcSectorsBottom(pLeds, pLedsHue, iStartIdx, iEndIdx, true, false, true); iStartIdx = iEndIdx; iEndIdx += this->GetSetupHorizontal();
      this->hueCalcSectorsRight(pLeds, pLedsHue, iStartIdx, iEndIdx, true, true, false);  iStartIdx = iEndIdx; iEndIdx += this->GetSetupVertical();
      this->hueCalcSectorsTop(pLeds, pLedsHue, iStartIdx, iEndIdx, true, false, false);
      break;
    case Top_Cclock_top_right_left:
      iEndIdx = this->GetSetupVertical();
      this->hueCalcSectorsTop(pLeds, pLedsHue, iStartIdx, iEndIdx, true, true, true);     iStartIdx = iEndIdx; iEndIdx += this->GetSetupHorizontal();
      this->hueCalcSectorsLeft(pLeds, pLedsHue, iStartIdx, iEndIdx, true, false, true);   iStartIdx = iEndIdx; iEndIdx += this->GetSetupVertical();
      this->hueCalcSectorsBottom(pLeds, pLedsHue, iStartIdx, iEndIdx, true, false, true); iStartIdx = iEndIdx; iEndIdx += this->GetSetupHorizontal();
      this->hueCalcSectorsRight(pLeds, pLedsHue, iStartIdx, iEndIdx, true, false, false);
      break;
    case Right_Cclock_bottom_right_up:
      iEndIdx = this->GetSetupHorizontal();
      this->hueCalcSectorsRight(pLeds, pLedsHue, iStartIdx, iEndIdx, true, true, true);   iStartIdx = iEndIdx; iEndIdx += this->GetSetupVertical();
      this->hueCalcSectorsTop(pLeds, pLedsHue, iStartIdx, iEndIdx, true, true, false);    iStartIdx = iEndIdx; iEndIdx += this->GetSetupHorizontal();
      this->hueCalcSectorsLeft(pLeds, pLedsHue, iStartIdx, iEndIdx, true, false, true);   iStartIdx = iEndIdx; iEndIdx += this->GetSetupVertical();
      this->hueCalcSectorsBottom(pLeds, pLedsHue, iStartIdx, iEndIdx, true, false, false);
      break;
    case Bottom_Cclock_bottom_left_right:
      iEndIdx = this->GetSetupVertical();
      this->hueCalcSectorsBottom(pLeds, pLedsHue, iStartIdx, iEndIdx, true, true, true);  iStartIdx = iEndIdx; iEndIdx += this->GetSetupHorizontal();
      this->hueCalcSectorsRight(pLeds, pLedsHue, iStartIdx, iEndIdx, true, true, false);  iStartIdx = iEndIdx; iEndIdx += this->GetSetupVertical();
      this->hueCalcSectorsTop(pLeds, pLedsHue, iStartIdx, iEndIdx, true, true, false);    iStartIdx = iEndIdx; iEndIdx += this->GetSetupHorizontal();
      this->hueCalcSectorsLeft(pLeds, pLedsHue, iStartIdx, iEndIdx, true, false, false);
      break;
    default:
      _DBGPRINTF(PSTR("LEDHelper:PostProcessLedColors: Invalid SETUP DIRECTION:%d\n"), this->GetSetupDirection());
      break;
  }

  // Set new LED Data!
  global::m_pHueHelper->setNewLedData(true);
#endif

  // --- Now handle disabled SIDES
  int iLast = 0;

  // Check for disabled sides based on Direction
  switch (this->GetSetupDirection()) {
    case Left_Clock_bottom_left_up:
      if (this->GetSetupSideLeft() == 0) {
        this->handleSideDisableVertical(pLeds, &iLast);
      } else {
        this->handleSideBrightnessVertical(pLeds, &iLast, this->GetSideLeftBrightness() );
      }
      if (this->GetSetupSideTop() == 0) {
        this->handleSideDisableHorizontal(pLeds, &iLast);
      } else {
        this->handleSideBrightnessHorizontal(pLeds, &iLast, this->GetSideTopBrightness() );
      }
      if (this->GetSetupSideRight() == 0) {
        this->handleSideDisableVertical(pLeds, &iLast);
      } else {
        this->handleSideBrightnessVertical(pLeds, &iLast, this->GetSideRightBrightness() );
      }
      if (this->GetSetupSideBottom() == 0) {
        this->handleSideDisableHorizontal(pLeds, &iLast);
      } else {
        this->handleSideBrightnessHorizontal(pLeds, &iLast, this->GetSideBottomBrightness() );
      }
      break;
    case Top_Clock_top_left_right:
      if (this->GetSetupSideTop() == 0) {
        this->handleSideDisableHorizontal(pLeds, &iLast);
      } else {
        this->handleSideBrightnessHorizontal(pLeds, &iLast, this->GetSideTopBrightness() );
      }
      if (this->GetSetupSideRight() == 0) {
        this->handleSideDisableVertical(pLeds, &iLast);
      } else {
        this->handleSideBrightnessVertical(pLeds, &iLast, this->GetSideRightBrightness() );
      }
      if (this->GetSetupSideBottom() == 0) {
        this->handleSideDisableHorizontal(pLeds, &iLast);
      } else {
        this->handleSideBrightnessHorizontal(pLeds, &iLast, this->GetSideBottomBrightness() );
      }
      if (this->GetSetupSideLeft() == 0) {
        this->handleSideDisableVertical(pLeds, &iLast);
      } else {
        this->handleSideBrightnessVertical(pLeds, &iLast, this->GetSideLeftBrightness() );
      }
      break;
    case Right_Clock_top_right_down:
      if (this->GetSetupSideRight() == 0) {
        this->handleSideDisableVertical(pLeds, &iLast);
      } else {
        this->handleSideBrightnessVertical(pLeds, &iLast, this->GetSideRightBrightness() );
      }
      if (this->GetSetupSideBottom() == 0) {
        this->handleSideDisableHorizontal(pLeds, &iLast);
      } else {
        this->handleSideBrightnessHorizontal(pLeds, &iLast, this->GetSideBottomBrightness() );
      }
      if (this->GetSetupSideLeft() == 0) {
        this->handleSideDisableVertical(pLeds, &iLast);
      } else {
        this->handleSideBrightnessVertical(pLeds, &iLast, this->GetSideLeftBrightness() );
      }
      if (this->GetSetupSideTop() == 0) {
        this->handleSideDisableHorizontal(pLeds, &iLast);
      } else {
        this->handleSideBrightnessHorizontal(pLeds, &iLast, this->GetSideTopBrightness() );
      }
      break;
    case Bottom_Clock_bottom_right_left:
      if (this->GetSetupSideBottom() == 0) {
        this->handleSideDisableHorizontal(pLeds, &iLast);
      } else {
        this->handleSideBrightnessHorizontal(pLeds, &iLast, this->GetSideBottomBrightness() );
      }
      if (this->GetSetupSideLeft() == 0) {
        this->handleSideDisableVertical(pLeds, &iLast);
      } else {
        this->handleSideBrightnessVertical(pLeds, &iLast, this->GetSideLeftBrightness() );
      }
      if (this->GetSetupSideTop() == 0) {
        this->handleSideDisableHorizontal(pLeds, &iLast);
      } else {
        this->handleSideBrightnessHorizontal(pLeds, &iLast, this->GetSideTopBrightness() );
      }
      if (this->GetSetupSideRight() == 0) {
        this->handleSideDisableVertical(pLeds, &iLast);
      } else {
        this->handleSideBrightnessVertical(pLeds, &iLast, this->GetSideRightBrightness() );
      }
      break;
    case Left_Cclock_top_left_down:
      if (this->GetSetupSideLeft() == 0) {
        this->handleSideDisableVertical(pLeds, &iLast);
      } else {
        this->handleSideBrightnessVertical(pLeds, &iLast, this->GetSideLeftBrightness() );
      }
      if (this->GetSetupSideBottom() == 0) {
        this->handleSideDisableHorizontal(pLeds, &iLast);
      } else {
        this->handleSideBrightnessHorizontal(pLeds, &iLast, this->GetSideBottomBrightness() );
      }
      if (this->GetSetupSideRight() == 0) {
        this->handleSideDisableVertical(pLeds, &iLast);
      } else {
        this->handleSideBrightnessVertical(pLeds, &iLast, this->GetSideRightBrightness() );
      }
      if (this->GetSetupSideTop() == 0) {
        this->handleSideDisableHorizontal(pLeds, &iLast);
      } else {
        this->handleSideBrightnessHorizontal(pLeds, &iLast, this->GetSideTopBrightness() );
      }
      break;
    case Top_Cclock_top_right_left:
      if (this->GetSetupSideTop() == 0) {
        this->handleSideDisableHorizontal(pLeds, &iLast);
      } else {
        this->handleSideBrightnessHorizontal(pLeds, &iLast, this->GetSideTopBrightness() );
      }
      if (this->GetSetupSideLeft() == 0) {
        this->handleSideDisableVertical(pLeds, &iLast);
      } else {
        this->handleSideBrightnessVertical(pLeds, &iLast, this->GetSideLeftBrightness() );
      }
      if (this->GetSetupSideBottom() == 0) {
        this->handleSideDisableHorizontal(pLeds, &iLast);
      } else {
        this->handleSideBrightnessHorizontal(pLeds, &iLast, this->GetSideBottomBrightness() );
      }
      if (this->GetSetupSideRight() == 0) {
        this->handleSideDisableVertical(pLeds, &iLast);
      } else {
        this->handleSideBrightnessVertical(pLeds, &iLast, this->GetSideRightBrightness() );
      }
      break;
    case Right_Cclock_bottom_right_up:
      if (this->GetSetupSideRight() == 0) {
        this->handleSideDisableVertical(pLeds, &iLast);
      } else {
        this->handleSideBrightnessVertical(pLeds, &iLast, this->GetSideRightBrightness() );
      }
      if (this->GetSetupSideTop() == 0) {
        this->handleSideDisableHorizontal(pLeds, &iLast);
      } else {
        this->handleSideBrightnessHorizontal(pLeds, &iLast, this->GetSideTopBrightness() );
      }
      if (this->GetSetupSideLeft() == 0) {
        this->handleSideDisableVertical(pLeds, &iLast);
      } else {
        this->handleSideBrightnessVertical(pLeds, &iLast, this->GetSideLeftBrightness() );
      }
      if (this->GetSetupSideBottom() == 0) {
        this->handleSideDisableHorizontal(pLeds, &iLast);
      } else {
        this->handleSideBrightnessHorizontal(pLeds, &iLast, this->GetSideBottomBrightness() );
      }
      break;
    case Bottom_Cclock_bottom_left_right:
      if (this->GetSetupSideBottom() == 0) {
        this->handleSideDisableHorizontal(pLeds, &iLast);
      } else {
        this->handleSideBrightnessHorizontal(pLeds, &iLast, this->GetSideBottomBrightness() );
      }
      if (this->GetSetupSideRight() == 0) {
        this->handleSideDisableVertical(pLeds, &iLast);
      } else {
        this->handleSideBrightnessVertical(pLeds, &iLast, this->GetSideRightBrightness() );
      }
      if (this->GetSetupSideTop() == 0) {
        this->handleSideDisableHorizontal(pLeds, &iLast);
      } else {
        this->handleSideBrightnessHorizontal(pLeds, &iLast, this->GetSideTopBrightness() );
      }
      if (this->GetSetupSideLeft() == 0) {
        this->handleSideDisableVertical(pLeds, &iLast);
      } else {
        this->handleSideBrightnessVertical(pLeds, &iLast, this->GetSideLeftBrightness() );
      }
      break;
    default:
      _DBGPRINTF(PSTR("LEDHelper:PostProcessLedColors: Invalid SETUP DIRECTION:%d\n"), this->GetSetupDirection());
      break;
  }
  // ---

}
void LEDHelper::PreProcessColorBoost(int iIndex, int* piRed, int* piGreen, int* piBlue) {
  int iBoostRed = 0;
  int iBoostGreen = 0;
  int iBoostBlue = 0;
  int iDiffRed = 0;
  int iDiffGreen = 0;
  int iDiffBlue = 0;
  int iColorBoostDiff = 0;

  /* Peform Colorboost on RAW pixels if
      - Colorboost is activated
      - At least one color is above ColorBoostThreshold (Default 0)
  */
  if (  m_bColorBoost > 0 && m_bColorBoost <= 100 &&
        (*piRed > this->GetColorBoostThreshold() || *piGreen > this->GetColorBoostThreshold() || *piBlue > this->GetColorBoostThreshold())
     ) {
    // Copy ColorBoost over (Now calced as percent Colorboost from color difference! Can be 5% to 500%)
    iBoostRed = (m_bColorBoostRed * m_bColorBoost * 5) / 100;
    iBoostGreen = (m_bColorBoostGreen * m_bColorBoost * 5) / 100;
    iBoostBlue = (m_bColorBoostBlue * m_bColorBoost * 5) / 100;

    // Check for highest color
    if      ( (*piRed >= *piGreen && *piRed > *piBlue) || (*piRed > *piGreen && *piRed >= *piBlue) ) {
      iDiffGreen = *piRed - *piGreen;
      iDiffBlue = *piRed - *piBlue;
      iDiffRed = (iDiffGreen + iDiffBlue) / 2;
      if (iDiffRed == 0) iDiffRed++;

      // Calc Boost Values first!
      iBoostRed   = (iBoostRed * iDiffRed) / 100;
      iBoostGreen = (*piGreen >= *piBlue ? (((m_bColorBoostGreen * ((*piGreen * m_bColorBoostBalance) / *piRed)) / 100) * iBoostRed) / 100 : 0);
      // (iBoostGreen * (((iDiffRed-iDiffGreen) * 100) / iDiffRed)) / 100;
      // (iDiffGreen > 0 ? (iBoostGreen * (((iDiffRed-iDiffGreen) * 100) / iDiffRed)) / 100 : +(iBoostGreen * iDiffRed) / 100 );
      iBoostBlue  = (*piBlue >= *piGreen ? (((m_bColorBoostBlue * ((*piBlue * m_bColorBoostBalance) / *piRed) ) / 100) * iBoostRed) / 100 : 0);
      // (iBoostBlue * (((iDiffRed-iDiffBlue) * 100) / iDiffRed)) / 100;
      // (iDiffBlue > 0 ? (iBoostBlue * (((iDiffRed-iDiffBlue) * 100) / iDiffRed)) / 100 : +(iBoostBlue * iDiffRed) / 100);
      // Smooth Colorboost
      if (m_bColorBoostMin > 0 && iDiffRed < m_bColorBoostMin ) {
        iBoostRed = (iBoostRed * iDiffRed) / m_bColorBoostMin;
        iBoostGreen = (iBoostGreen * iDiffRed) / m_bColorBoostMin;
        iBoostBlue = (iBoostBlue * iDiffRed) / m_bColorBoostMin;
      }
      // Limit Color Boost
      if ((*piRed + iBoostRed) > 255) {
        iColorBoostDiff = ((*piRed + iBoostRed - 255) * 100) / iBoostRed;
        iBoostGreen -= iBoostRed * iColorBoostDiff / 100; //iBoostGreen * iColorBoostDiff / 100;
        iBoostBlue  -= iBoostRed * iColorBoostDiff / 100; //iBoostBlue * iColorBoostDiff / 100;
      }
    } else if ( (*piGreen >= *piRed && *piGreen > *piBlue) || (*piGreen > *piRed && *piGreen >= *piBlue) ) {
      iDiffRed = *piGreen - *piRed;
      iDiffBlue = *piGreen - *piBlue;
      iDiffGreen  = (iDiffRed + iDiffBlue) / 2;
      if (iDiffGreen == 0) iDiffGreen++;
      // Calc Boost Values first!
      iBoostGreen = (iBoostGreen * iDiffGreen) / 100;
      iBoostRed   = (*piRed >= *piBlue ? (((m_bColorBoostRed * ((*piRed * m_bColorBoostBalance) / *piGreen) ) / 100) * iBoostGreen) / 100 : 0);
      iBoostBlue  = (*piBlue >= *piRed ? (((m_bColorBoostBlue * ((*piBlue * m_bColorBoostBalance) / *piGreen) ) / 100) * iBoostGreen) / 100 : 0);
      // Smooth Colorboost
      if (m_bColorBoostMin > 0 && iDiffGreen < m_bColorBoostMin) {
        iBoostGreen = (iBoostGreen * iDiffGreen) / m_bColorBoostMin;
        iBoostRed   = (iBoostRed * iDiffGreen) / m_bColorBoostMin;
        iBoostBlue  = (iBoostBlue * iDiffGreen) / m_bColorBoostMin;
      }
      // Limit Color Boost
      if ((*piGreen + iBoostGreen) > 255) {
        //iColorBoostDiff = 255 - *piGreen;
        //iBoostRed     = (*piRed > *piBlue ? iColorBoostDiff : -iColorBoostDiff);
        //iBoostBlue    = (*piBlue > *piRed ? iColorBoostDiff : -iColorBoostDiff);
        iColorBoostDiff = ((*piGreen + iBoostGreen - 255) * 100) / iBoostGreen;
        iBoostRed   -= iBoostGreen * iColorBoostDiff / 100; // iBoostRed * iColorBoostDiff / 100;
        iBoostBlue  -= iBoostGreen * iColorBoostDiff / 100; // iBoostBlue * iColorBoostDiff / 100;
      }
    } else if ( (*piBlue >= *piRed && *piBlue > *piGreen) || (*piBlue > *piRed && *piBlue >= *piGreen) ) {
      iDiffRed = *piBlue - *piRed;
      iDiffGreen = *piBlue - *piGreen;
      iDiffBlue = (iDiffGreen + iDiffRed) / 2;
      if (iDiffBlue == 0) iDiffBlue++;
      // Calc Boost Values first!
      iBoostBlue  = (iBoostBlue * iDiffBlue) / 100;
      iBoostRed   = (*piRed >= *piGreen ? (((m_bColorBoostRed * ((*piRed * m_bColorBoostBalance) / *piBlue) ) / 100) * iBoostBlue) / 100 : 0);
      iBoostGreen = (*piGreen >= *piRed ? (((m_bColorBoostGreen * ((*piGreen * m_bColorBoostBalance) / *piBlue) ) / 100) * iBoostBlue) / 100 : 0);
      // Smooth Colorboost
      if (m_bColorBoostMin > 0 && iDiffBlue < m_bColorBoostMin) {
        iBoostBlue  = (iBoostBlue * iDiffBlue) / m_bColorBoostMin;
        iBoostGreen = (iBoostGreen * iDiffBlue) / m_bColorBoostMin;
        iBoostRed   = (iBoostRed * iDiffBlue) / m_bColorBoostMin;
      }
      // Limit Color Boost
      if ( (*piBlue + iBoostBlue) > 255) {
        iColorBoostDiff = ((*piBlue + iBoostBlue - 255) * 100) / iBoostBlue;
        iBoostRed    -= iBoostBlue * iColorBoostDiff / 100; // iBoostRed * iColorBoostDiff / 100;
        iBoostGreen  -= iBoostBlue * iColorBoostDiff / 100; // iBoostGreen * iColorBoostDiff / 100;
      }
    }
    else
    {
      // DO NOT BOOST AT ALL?!
      if (global::m_bUltraDebugEnabled /* && iIndex == 0*/) _DBGPRINTF(PSTR("LEDHelper: PreProcessColorBoost(%d) NO BOOST R:%d,G:%d,B:%d\\n"), iIndex, *piRed, *piGreen, *piBlue);
      iBoostRed = 0;
      iBoostGreen = 0;
      iBoostBlue = 0;
    }

    // Add calced color boost
    if (global::m_bUltraDebugEnabled /* && iIndex == 0 */) _DBGPRINTF(PSTR("LEDHelper: PreProcessColorBoost(%d) Boost:%d,R:%d/%d,G:%d/%d,B:%d/%d\n"), iIndex, m_bColorBoost, iBoostRed, *piRed, iBoostGreen, *piGreen, iBoostBlue, *piBlue);
    *piRed = (*piRed + iBoostRed > 0 ? (*piRed + iBoostRed < 255 ? *piRed + iBoostRed : 255) : 0);
    *piGreen = (*piGreen + iBoostGreen > 0 ? (*piGreen + iBoostGreen < 255 ? *piGreen + iBoostGreen : 255) : 0);
    *piBlue = (*piBlue + iBoostBlue > 0 ? (*piBlue + iBoostBlue < 255 ? *piBlue + iBoostBlue : 255) : 0);
  } else {
    if (global::m_bUltraDebugEnabled /* && iIndex == 0 */) _DBGPRINTF(PSTR("LEDHelper: PreProcessColorBoost(%d) NO Boost #2:R:%d,G:%d,B:%d\n"), iIndex, *piRed, *piGreen, *piBlue);
  }

  if (global::m_bUltraDebugEnabled /* && iIndex == 0*/) _DBGPRINTF(PSTR("LEDHelper: PreProcessColorBoost(%d) Final:%d,R:%d,G:%d,B:%d\n"), iIndex, m_bColorBoost, *piRed, *piGreen, *piBlue);
}
void LEDHelper::PreProcessValueGain(int iIndex, int* piRed, int* piGreen, int* piBlue) {
  //  All values now if (m_iColorValueGain > 100)
  {
    // calc value gain
    *piRed = (*piRed * m_iColorValueGain) / 100;
    *piGreen = (*piGreen * m_iColorValueGain) / 100;
    *piBlue = (*piBlue * m_iColorValueGain) / 100;
  }
  if (global::m_bUltraDebugEnabled && iIndex == 0) _DBGPRINTF(PSTR("LEDHelper: PreProcessValueGain(%d) %d - %d - %d\n"), m_iColorValueGain, *piRed, *piGreen, *piBlue);
}
void LEDHelper::PreProcessMinLuminosity(int iIndex, int* piRed, int* piGreen, int* piBlue) {
  if (m_bMinimumLuminosity > 0) {
    *piRed = (m_bMinimumLuminosity > *piRed ? m_bMinimumLuminosity : *piRed);
    *piGreen = (m_bMinimumLuminosity > *piGreen ? m_bMinimumLuminosity : *piGreen);
    *piBlue = (m_bMinimumLuminosity > *piBlue ? m_bMinimumLuminosity : *piBlue);
  }
  if (global::m_bUltraDebugEnabled && iIndex == 0) _DBGPRINTF(PSTR("LEDHelper: PreProcessMinLuminosity(%d) %d - %d - %d\n"), m_bMinimumLuminosity, *piRed, *piGreen, *piBlue);
}
void LEDHelper::PreProcessLowColorSmoothing(int iIndex, byte bLowColorSmoothing, int* piRed, int* piGreen, int* piBlue) {
  int iDiffRed = 0;
  int iDiffGreen = 0;
  int iDiffBlue = 0;

  if (global::m_bUltraDebugEnabled && iIndex == 0) _DBGPRINTF(PSTR("LEDHelper: PreProcessLowColorSmoothing START (%d) %d - %d - %d\n"), bLowColorSmoothing, *piRed, *piGreen, *piBlue);

  if      ( (*piRed) > *piGreen && (*piRed) > *piBlue) {
    iDiffGreen = *piRed - *piGreen;
    iDiffBlue = *piRed - *piBlue;
    iDiffRed = (iDiffGreen >= iDiffBlue ? iDiffBlue : iDiffGreen);

    // calc red value smoothing
    *piRed = *piRed - (iDiffRed * bLowColorSmoothing / 100 / 2);
    *piGreen = *piGreen + (iDiffRed * bLowColorSmoothing / 100 / 2);
    *piBlue = *piBlue + (iDiffRed * bLowColorSmoothing / 100 / 2);
  } else if ( (*piGreen) > *piRed && (*piGreen) > *piBlue) {
    iDiffRed = *piGreen - *piRed;
    iDiffBlue = *piGreen - *piBlue;
    iDiffGreen = (iDiffRed >= iDiffBlue ? iDiffBlue : iDiffRed);

    // calc green value smoothing
    *piGreen = *piGreen - (iDiffGreen * bLowColorSmoothing / 100 / 2);
    *piBlue = *piBlue + (iDiffGreen * bLowColorSmoothing / 100 / 2);
    *piRed = *piRed + (iDiffGreen * bLowColorSmoothing / 100 / 2);

  } else if ( (*piBlue) > *piRed && (*piBlue) > *piGreen) {
    iDiffRed = *piBlue - *piRed;
    iDiffGreen = *piBlue - *piGreen;
    iDiffBlue = (iDiffRed >= iDiffGreen ? iDiffGreen : iDiffRed);

    // calc blue value smoothing
    *piBlue = *piBlue - (iDiffBlue * bLowColorSmoothing / 100 / 2);
    *piRed = *piRed + (iDiffBlue * bLowColorSmoothing / 100 / 2);
    *piGreen = *piGreen + (iDiffBlue * bLowColorSmoothing / 100 / 2);
  } else {
    // DO NOT LOW COLOR AT ALL?!
    if (global::m_bUltraDebugEnabled && iIndex == 0) _DBGPRINTF(PSTR("LEDHelper: PreProcessLowColorSmoothing NO LOWER %d,%d,%d\n"), *piRed, *piGreen, *piBlue);
  }

  if (global::m_bUltraDebugEnabled && iIndex == 0) _DBGPRINTF(PSTR("LEDHelper: PreProcessLowColorSmoothing END (%d) %d - %d - %d\n"), bLowColorSmoothing, *piRed, *piGreen, *piBlue);
}
void LEDHelper::PreProcessFrameSmoothing(int iIndex, int* piRed, int* piGreen, int* piBlue, void* pVoidLedFrames) {
  CRGB* pLedFrames = (CRGB*) pVoidLedFrames; // Cast Pointer of Frame data to CRGB
  if (pLedFrames != NULL &&
      m_bFrameSmoothing > DEFAULT_FRAME_SMOOTHING) {
    for (int z = 0; z < m_bFrameSmoothing; z++) {
      int iAddIdx = z * MAX_LEDS;
      *piRed +=   (pLedFrames + iAddIdx + iIndex)->r;
      *piGreen += (pLedFrames + iAddIdx + iIndex)->g;
      *piBlue +=  (pLedFrames + iAddIdx + iIndex)->b;
    }
    *piRed = *piRed / (m_bFrameSmoothing + 1);
    *piGreen = *piGreen / (m_bFrameSmoothing + 1);
    *piBlue = *piBlue / (m_bFrameSmoothing + 1);

    if (global::m_bUltraDebugEnabled && iIndex == 0) _DBGPRINTF(PSTR("LEDHelper: PreProcessFrameSmoothing(%d) %d/%d - %d/%d - %d/%d\n"), m_bFrameSmoothing,
          (pLedFrames + iIndex)->r, *piRed,
          (pLedFrames + iIndex)->g, *piGreen,
          (pLedFrames + iIndex)->b, *piBlue);
  }
}
void LEDHelper::PostProcessFrameSmoothing(void* pVoidLedFrames, void* pVoidLedsNext, int numLeds) {
  CRGB* pLedFrames = (CRGB*) pVoidLedFrames;  // Cast Pointer of Frame data to CRGB
  CRGB* pLedsNext = (CRGB*) pVoidLedsNext;    //
  int iMaxLedsCrgbSize = MAX_LEDS * sizeof(struct CRGB);

  /* Copy Frame data over, only needed if more than 1 frame is used */
  if (pLedFrames != NULL) {
    if (m_bFrameSmoothing > 1) {
      // Move old frames memory if more than 1
      memmove(((byte*)pLedFrames) + iMaxLedsCrgbSize/*MAX_LEDS*/, pLedFrames, ((m_bFrameSmoothing - 1) * iMaxLedsCrgbSize));
    }

    // Copy latest frame data over
    memset(pLedFrames, 0, iMaxLedsCrgbSize);
    for (int l = 0; l < numLeds; l++) {
      (pLedFrames + l)->r =  (pLedsNext + l)->r;
      (pLedFrames + l)->g =  (pLedsNext + l)->g;
      (pLedFrames + l)->b =  (pLedsNext + l)->b;
    }
  }
}

/* LED PreProcess code which is done FIRST on LED Data */
void LEDHelper::PreProcessLedColors(void* pLeds) {
  int iRed = 0;
  int iGreen = 0;
  int iBlue = 0;
  CRGB* pLedsNext = (CRGB*) pLeds;

  /* Presets 12-15 (health presets: 40 Hz Gamma, 10 Hz Alpha, 34 Hz Gamma Red, Rotlicht): skip smoothing/min luminosity so Intensity works correctly */
  int preset = (this->GetLedMode() == LED_MODE_AMBIENT) ? global::GetAmbientPreset() : -1;
  bool skipProcess = (preset >= 12 && preset <= 15);

  /* Calc color corrections before steps! */
  for (int l = 0; l < (int)this->getNumLeds(); l++)
  {
    iRed = (pLedsNext + l)->r;
    iGreen = (pLedsNext + l)->g;
    iBlue = (pLedsNext + l)->b;

    if (global::m_bUltraDebugEnabled && l == 0) _DBGPRINTF(PSTR("LEDHelper: PreProcessLedColors Begin(%u) R:%d,G:%d,B:%d\n"), (unsigned)this->getNumLeds(), iRed, iGreen, iBlue);

    if (!skipProcess) {
      /* 1. Check for ColorBoost 0-100% | DO NOT CALC IN CAM Mode, we do the Color Boost on the RAW Pixels! */
      if (this->GetLedMode() != LED_MODE_CAM) {
        this->PreProcessColorBoost(l, &iRed, &iGreen, &iBlue);
      }

      /* 2. Check for Value Gain */
      this->PreProcessValueGain(l, &iRed, &iGreen, &iBlue);

      /* 3. Calc Min Luminosity */
      this->PreProcessMinLuminosity(l, &iRed, &iGreen, &iBlue);
    }

# if defined(ENABLE_CAM)
    if (global::m_pCamProcessor && global::m_pCamProcessor->GetCamSmoothEnable() == 1) {
      /* 4. Camera Motion Smoothing */
      global::m_pCamProcessor->PreProcessCameraSmoothing(l, &iRed, &iGreen, &iBlue, global::m_pCamProcessor->GetVoidLedSmoothFramesPtr());
    }
# endif

# if defined(ENABLE_RELAY)
    // Only do framesmoothing if not relaying
    if (global::GetAmbibackRelaymodeEnabled() == 0 || global::GetAmbibackRelayModeDirect() == 0) {
# endif
      /* 5. Frame Smoothing - skip for 40 Hz / 34 Hz Gamma to preserve Intensity */
      if (!skipProcess) {
        this->PreProcessFrameSmoothing(l, &iRed, &iGreen, &iBlue, m_pLedFrames);
      }
# if defined(ENABLE_RELAY)
    }
# endif

    // Finally copy over  and check if values are out of bounds
    (pLedsNext + l)->r = (byte)(iRed < 255 ? (iRed > 0 ? iRed : 0) : 255);
    (pLedsNext + l)->g = (byte)(iGreen < 255 ? (iGreen > 0 ? iGreen : 0) : 255);
    (pLedsNext + l)->b = (byte)(iBlue < 255 ? (iBlue > 0 ? iBlue : 0) : 255);
  }

# if defined(ENABLE_CAM)
  if (global::m_pCamProcessor && global::m_pCamProcessor->GetCamSmoothEnable() == 1) {
    /* 6. Post Process for Camera Motion Smoothing */
    global::m_pCamProcessor->PostProcessCameraSmoothing(global::m_pCamProcessor->GetVoidLedSmoothFramesPtr(), pLedsNext, (int)this->getNumLeds());
  }
# endif

# if defined(ENABLE_RELAY)
  // Only do framesmoothing if not relaying
  if (global::GetAmbibackRelaymodeEnabled() == 0 || global::GetAmbibackRelayModeDirect() == 0) {
# endif
    /* 7. Post Process for Frame Smoothing (skip for health presets 12-15 to preserve Intensity) */
    if (!skipProcess) {
      this->PostProcessFrameSmoothing(m_pLedFrames, pLedsNext, (int)this->getNumLeds());
    }
# if defined(ENABLE_RELAY)
  }
# endif
}
void LEDHelper::ProcessLedNextStep() {
  int iFrameLength = (!global::GetFirmwareRunning() ? this->GetFrameLength() : 200 /* 5 FPS while OTA update */);
  int iSteps = (m_iDelayLength / iFrameLength > 0 ? m_iDelayLength / iFrameLength : 1);
  /* Presets 12–14 (40 Hz Gamma, 10 Hz Alpha, 34 Hz Gamma) need minimal pipeline delay for timing */
  if (this->GetLedMode() == LED_MODE_AMBIENT && global::GetAmbientPreset() >= 12 && global::GetAmbientPreset() <= 14) {
    iFrameLength = 1;
    iSteps = 1;
  }
  long nDelayLengthLeft = m_iDelayLength;
  long nTimeNow = 0;
  long nTimePassed = 0;
  long nExtraSleep = 0;
  int iRed = 0;
  int iGreen = 0;
  int iBlue = 0;

  _DBGPRINTF_ULTRA(PSTR("LEDHelper: ProcessLedNextStep ENTER\n"));

  /* Get copy of m_leds_next */
  CRGB leds_next[MAX_LEDS];
  memcpy(leds_next, m_leds_next, MAX_LEDS * sizeof(struct CRGB));

  /* ZERO unused LEDs */
  for (int k = (int)this->getNumLeds(); k < MAX_LEDS; k++)
  {
    m_leds1[k].r = 0;
    m_leds1[k].g = 0;
    m_leds1[k].b = 0;
  }

  // Set NewLedData to FALSE !
  this->SetIsNewLedData(false);

  /* Prepare Led Colors! */
  this->PreProcessLedColors(&leds_next[0]);

  /* Postprocess Led Colors! */
  this->PostProcessLedColors(&leds_next[0]);

  /* Now calc color steps! */
  for (int i = iSteps; i > 0; i--) {
    nTimeNow = millis();

    // New data received? Adapt our final colors
    if (this->GetIsNewLedData()) {
      _DBGPRINTF_ULTRA(PSTR("LEDHelper: ProcessLedNextStep NEW DATA at Step %d of %d\n"), i, iSteps);

      // Copy last leds_next to m_leds1
      memcpy(m_leds1, leds_next, MAX_LEDS * sizeof(struct CRGB));

      // Copy new pixels over
      memcpy(leds_next, m_leds_next, MAX_LEDS * sizeof(struct CRGB));

      // Set NewLedData to FALSE !
      this->SetIsNewLedData(false);

      /* Prepare Led Colors! */
      this->PreProcessLedColors(&leds_next[0]);

      /* Postprocess Led Colors! */
      this->PostProcessLedColors(&leds_next[0]);

      // Reset iSteps, then LastStep back to iSteps
      iSteps = (m_iDelayLength / iFrameLength > 0 ? m_iDelayLength / iFrameLength : 1);
      i = iSteps;
      nDelayLengthLeft = m_iDelayLength;
    }

    /* calc new LED interrim colors */
    for (int j = 0; j < (int)this->getNumLeds(); j++)
    {
      /* Calc next Colors**/
      iRed = (i > 1) ?
             (leds_next[j].r >= m_leds1[j].r ?
              m_leds1[j].r + ((((leds_next[j].r - m_leds1[j].r) * 100) / i ) / 100) :
              m_leds1[j].r - ((((m_leds1[j].r - leds_next[j].r) * 100) / i ) / 100) ) :
             leds_next[j].r;
      iGreen = (i > 1) ?
               (leds_next[j].g >= m_leds1[j].g ?
                m_leds1[j].g + ((((leds_next[j].g - m_leds1[j].g) * 100) / i ) / 100) :
                m_leds1[j].g - ((((m_leds1[j].g - leds_next[j].g) * 100) / i ) / 100) ) :
               leds_next[j].g;
      iBlue = (i > 1) ?
              (leds_next[j].b >= m_leds1[j].b ?
               m_leds1[j].b + ((((leds_next[j].b - m_leds1[j].b) * 100) / i ) / 100) :
               m_leds1[j].b - ((((m_leds1[j].b - leds_next[j].b) * 100) / i ) / 100) ) :
              leds_next[j].b;

      // Finally copy over  and check if values are out of bounds
      m_leds1[j].r = (byte)(iRed < 255 ? (iRed > 0 ? iRed : 0) : 255);
      m_leds1[j].g = (byte)(iGreen < 255 ? (iGreen > 0 ? iGreen : 0) : 255);
      m_leds1[j].b = (byte)(iBlue < 255 ? (iBlue > 0 ? iBlue : 0) : 255);

      if (global::m_bUltraDebugEnabled && j == 0) _DBGPRINTF(PSTR("LEDHelper: ProcessLedNextStep Final %d/%d - %d/%d - %d/%d\n"), iRed, leds_next[j].r, iGreen, leds_next[j].g, iBlue, leds_next[j].b);
    }

#if defined(ENABLE_RELAY)
    if ( global::GetAmbibackRelaymodeEnabled() ) {
      /* Activate remote LED Colors */
      this->RelayLedData();
    } else {
#else
    {
#endif
      /* Activate new LED Colors localy */
      this->ShowLedColors();
    }

    // Increment Framecounter
    this->IncrementCurrentFramesCtr();

    /* Check how long we need to sleep for next frame */
    nTimePassed = millis() - nTimeNow;
    nDelayLengthLeft -= nTimePassed;
    nExtraSleep = iFrameLength - nTimePassed;

    /* Sleep only if necessary */
    if (  nTimePassed < iFrameLength &&
          nExtraSleep > 0 &&
          i > 1 ) {
      // Sleep till next step
      _taskDelay(nExtraSleep);
      // Calc delay length left
      nDelayLengthLeft -= nExtraSleep;
    } else if (nTimePassed > iFrameLength && i > 0) {
      // Skip a step and sleep
      nExtraSleep = (iFrameLength * 2) - nTimePassed;
      if (nExtraSleep > 0) {
        _DBGPRINTF_ULTRA(PSTR("LEDHelper: ProcessLedNextStep SKIPPED Step %d of %d TimePassed %d ms, ExtraSleep %d ms, Delay left %d\n"), i, iSteps, nTimePassed, nExtraSleep, nDelayLengthLeft);
        // Sleep till next step
        _taskDelay(nExtraSleep);
      } else {
        _DBGPRINTF_ULTRA(PSTR("LEDHelper: ProcessLedNextStep WTF CANNOT SLEEP Step %d of %d TimePassed %d ms, ExtraSleep %d ms, Delay left %d\n"), i, iSteps, nTimePassed, nExtraSleep, nDelayLengthLeft);
        // --- Perform other tasks
        _taskYIELD();
        // ---
      }

      // Skip Step
      i--;
      // Calc delay length left
      nDelayLengthLeft -= nExtraSleep;
    } else {
      // --- Perform other tasks
      _taskYIELD();
      // ---
      nExtraSleep = 0;
    }

    /* Debug Output */
    _DBGPRINTF_ULTRA(PSTR("LEDHelper: ProcessLedNextStep Step %d of %d, TimePassed %d ms, ExtraSleep %d ms, Delay left %d\n"), i, iSteps, nTimePassed, nExtraSleep, nDelayLengthLeft);
  }

  _DBGPRINTF_ULTRA(PSTR("LEDHelper: ProcessLedNextStep EXIT\n"));
}

void LEDHelper::effectFadeToBlack(int iControllerIndex, int iBrightNess) {
  _DBGPRINTF(PSTR("LEDHelper::effectFadeToBlack: CtrlIndex %d\n"), iControllerIndex);

  if (m_pLeds[iControllerIndex] != NULL &&
      m_controllers[iControllerIndex] != NULL) {
    m_pLeds[iControllerIndex]->fadeToBlackBy(240);
    //    m_controllers[iControllerIndex]->showLeds(iBrightNess);
    FastLED.show(m_iBrightness);
    _taskDelay(100);
  } else {
    global::errprintf_P(PSTR("LEDHelper::effectFadeToBlack: ERROR PLEDS/CONTROLLER NULL\n"));
  }
}

void LEDHelper::setFullColor(int iControllerIndex, int iBrightNess, byte bWhite, byte bRed, byte bGreen, byte bBlue) {
  _DBGPRINTF_ULTRA(PSTR("LEDHelper::setFullColor: CtrlIndex %d, LEDs %d, Brightness %d, White %d, Red %d, Green %d, Blue %d\n"), iControllerIndex, m_pLedCount[iControllerIndex], iBrightNess, bWhite, bRed, bGreen, bBlue);

  if (m_pLeds[iControllerIndex] != NULL &&
      m_controllers[iControllerIndex] != NULL) {
    fill_solid(m_pLeds[iControllerIndex], m_pLedCount[iControllerIndex], CRGB( bRed, bGreen, bBlue));
    //    m_controllers[iControllerIndex]->showLeds(iBrightNess);
    FastLED.show(m_iBrightness);
  } else {
    global::errprintf_P(PSTR("LEDHelper::setFullColor: ERROR PLEDS/CONTROLLER NULL\n"));
  }
}

void LEDHelper::setFullColor(byte bWhite, byte bRed, byte bGreen, byte bBlue) {
#if defined(ENABLE_VIDEO)
  this->setFullColor(LED_PRT1IDX, m_iBrightness, bWhite, bRed, bGreen, bBlue);
# if defined(USE_HAS_SECOND_PORT) || defined(CLONE_TO_SECOND_PORT)
  this->setFullColor(LED_PRT2IDX, m_iBrightness, bWhite, bRed, bGreen, bBlue);
# endif
#endif
}

void LEDHelper::setFullColor(int iColorCode) {
  byte bWhite = (iColorCode & 0xFF000000) >> 24;
  byte bRed = (iColorCode & 0x00FF0000) >> 16;
  byte bGreen = (iColorCode & 0x0000FF00) >> 8;
  byte bBlue = (iColorCode & 0x000000FF);

  _DBGPRINTF(PSTR("LEDHelper::setFullColor: %d\n"), iColorCode);
  this->setFullColor(bWhite, bRed, bGreen, bBlue);
}

void LEDHelper::colorBlink(bool bRed, bool bGreen, bool bBlue, byte bSteps, bool bOff) {
  _DBGPRINTF_ULTRA(PSTR("colorBlink: ENTER\n"));
  unsigned long timeNow = 0;
  unsigned long timePassed = 0;

  // Flash to color
  for (int i = 0; i < 255; i += bSteps) {
    timeNow = millis();
    this->setFullColor(0, (bRed ? i : 0), (bGreen ? i : 0), (bBlue ? i : 0));
    timePassed = millis() - timeNow;
    if (millis() - timeNow < m_bFrameLength) {
      _delay( m_bFrameLength - timePassed);
    }
  }

  // Flash to black
  for (int i = 255; i > 0; i -= bSteps) {
    timeNow = millis();
    this->setFullColor(0, (bRed ? i : 0), (bGreen ? i : 0), (bBlue ? i : 0));
    timePassed = millis() - timeNow;
    if (millis() - timeNow < m_bFrameLength) {
      _delay( m_bFrameLength - timePassed);
    }
  }

  /* Turn LED off */
  if (bOff) {
    this->setFullColor(0, 0, 0, 0);
  }
}

void LEDHelper::colorBlink(byte bRed, byte bGreen, byte bBlue, byte bSteps, bool bOff) {
#if defined(ENABLE_VIDEO)
  this->colorBlink(LED_PRT1IDX, 255, 0, bRed, bGreen, bBlue, bSteps);
# if defined(USE_HAS_SECOND_PORT) || defined(CLONE_TO_SECOND_PORT)
  this->colorBlink(LED_PRT2IDX, 255, 0, bRed, bGreen, bBlue, bSteps);
# endif
#endif
}

void LEDHelper::bootColorChase(byte bRed, byte bGreen, byte bBlue, byte bLedsPerStep, unsigned long lStepDelayMs, bool bOffAfter) {
#if defined(ENABLE_VIDEO)
  uint16_t numLeds = this->getNumLeds();
  byte ledsPerStep = (bLedsPerStep == 0 ? 1 : bLedsPerStep);

  memset(m_leds1, 0, MAX_LEDS * sizeof(struct CRGB));
# if defined(USE_HAS_SECOND_PORT)
  memset(m_leds2, 0, MAX_LEDS * sizeof(struct CRGB));
# endif
  this->ShowLedColors();

  for (uint16_t i = 0; i < numLeds; i += ledsPerStep) {
    for (uint16_t j = i; j < numLeds && j < (uint16_t)(i + ledsPerStep); j++) {
      uint16_t percent = (numLeds <= 1 ? 100 : (uint16_t)(10 + ((uint32_t)j * 90UL) / (uint32_t)(numLeds - 1)));
      m_leds1[j].r = (byte)(((uint16_t)bRed * percent) / 100);
      m_leds1[j].g = (byte)(((uint16_t)bGreen * percent) / 100);
      m_leds1[j].b = (byte)(((uint16_t)bBlue * percent) / 100);
# if defined(USE_HAS_SECOND_PORT)
      m_leds2[j].r = m_leds1[j].r;
      m_leds2[j].g = m_leds1[j].g;
      m_leds2[j].b = m_leds1[j].b;
# endif
    }
    this->ShowLedColors();
    if (lStepDelayMs > 0) {
      _delay(lStepDelayMs);
    }
  }

  if (bOffAfter) {
    memset(m_leds1, 0, MAX_LEDS * sizeof(struct CRGB));
# if defined(USE_HAS_SECOND_PORT)
    memset(m_leds2, 0, MAX_LEDS * sizeof(struct CRGB));
# endif
    this->ShowLedColors();
  }
#endif
}

void LEDHelper::colorBlinkUp(int iControllerIndex, int iBrightNess, byte bWhite, byte bRed, byte bGreen, byte bBlue, byte bSteps) {
  _DBGPRINTF_ULTRA(PSTR("colorBlinkUp: ENTER byte %d/%d/%d/%d/%d\n"), iControllerIndex, iBrightNess, bWhite, bRed, bGreen, bBlue, bSteps);
  unsigned long timeNow = 0;
  unsigned long timePassed = 0;
  int iRedVal = bRed > 0 ? 255 / (int)bRed : 0;
  int iGreenVal = bGreen > 0 ? 255 / (int)bGreen : 0;
  int iBlueVal = bBlue > 0 ? 255 / (int)bBlue : 0;

  if (bSteps > 1) {
    // Flash to color
    for (int i = 0; i < 255; i += bSteps) {
      timeNow = millis();
      this->setFullColor(iControllerIndex, iBrightNess, bWhite, iRedVal > 0 ? (i / iRedVal) : 0, iGreenVal > 0 ? (i / iGreenVal) : 0, iBlueVal > 0 ? (i / iBlueVal) : 0);
      timePassed = millis() - timeNow;
      if (millis() - timeNow < m_bFrameLength)
        _delay(m_bFrameLength - timePassed);
    }
  } else {
    this->setFullColor(iControllerIndex, iBrightNess, bWhite, bRed, bGreen, bBlue);
  }
}

void LEDHelper::colorBlinkDown(int iControllerIndex, int iBrightNess, byte bWhite, byte bRed, byte bGreen, byte bBlue, byte bSteps) {
  _DBGPRINTF_ULTRA(PSTR("colorBlinkDown: ENTER byte %d/%d/%d/%d/%d\n"), iControllerIndex, iBrightNess, bWhite, bRed, bGreen, bBlue, bSteps);
  unsigned long timeNow = 0;
  unsigned long timePassed = 0;
  int iRedVal = bRed > 0 ? 255 / (int)bRed : 0;
  int iGreenVal = bGreen > 0 ? 255 / (int)bGreen : 0;
  int iBlueVal = bBlue > 0 ? 255 / (int)bBlue : 0;


  if (bSteps > 1) {
    // Flash to black
    for (int i = 255; i > 0; i -= bSteps) {
      timeNow = millis();
      this->setFullColor(iControllerIndex, iBrightNess, bWhite, iRedVal > 0 ? (i / iRedVal) : 0, iGreenVal > 0 ? (i / iGreenVal) : 0, iBlueVal > 0 ? (i / iBlueVal) : 0);
      timePassed = millis() - timeNow;
      if (millis() - timeNow < m_bFrameLength)
        _delay( m_bFrameLength - timePassed);
    }
  }

  /* Turn LED off */
  _DBGPRINTF(PSTR("colorBlinkDown: Turn OFF for LEDIdx %d\n"), iControllerIndex);
  this->setFullColor(iControllerIndex, iBrightNess, 0, 0, 0, 0);
}

void LEDHelper::colorBlink(int iControllerIndex, int iBrightNess, byte bWhite, byte bRed, byte bGreen, byte bBlue, byte bSteps) {
  if (bSteps > 1) {
    this->colorBlinkUp(iControllerIndex, iBrightNess, bWhite, bRed, bGreen, bBlue, bSteps);
    this->colorBlinkDown(iControllerIndex, iBrightNess, bWhite, bRed, bGreen, bBlue, bSteps);
  } else {
    this->setFullColor(iControllerIndex, iBrightNess, bWhite, bRed, bGreen, bBlue);
  }
}

void LEDHelper::processIRCommand(unsigned long uilastir) {
  _DBGPRINTF(PSTR("processIRCommand: 0x%08x\n"), uilastir);

  /* Process IR Command */
  switch (uilastir)
  {
    case 0xFFA05F:  /* +Light */
      if (m_bRed < 255 && m_bGreen < 255 && m_bBlue < 255) {
        m_bRed = (m_bRed > 0 ? m_bRed + m_bRedStep : m_bRed);
        m_bGreen = (m_bGreen > 0 ? m_bGreen + m_bGreenStep : m_bGreen);
        m_bBlue = (m_bBlue > 0 ? m_bBlue + m_bBlueStep : m_bBlue);
      }
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;
    case 0x00ff906f:  /* Red */
    case 0xf647450bUL:
      _DBGPRINTF(PSTR("processIRCommand: RED\n"));
      m_bRed = 255; m_bGreen = 0; m_bBlue = 0; m_bRedStep = 8; m_bGreenStep = 8; m_bBlueStep = 8;
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;
    case 0xFFB04F:  /* Red-Orange */
      m_bRed = 255; m_bGreen = 31; m_bBlue = 0; m_bRedStep = 8; m_bGreenStep = 1; m_bBlueStep = 0;
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;
    case 0xFFA857:  /* Orange */
      m_bRed = 255; m_bGreen = 63; m_bBlue = 0; m_bRedStep = 8; m_bGreenStep = 2; m_bBlueStep = 0;
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;
    case 0xFF9867:  /* Orange-yellow */
      m_bRed = 255; m_bGreen = 127; m_bBlue = 0; m_bRedStep = 8; m_bGreenStep = 4; m_bBlueStep = 0;
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;
    case 0xFF8877:  /* Yellow */
      m_bRed = 255; m_bGreen = 255; m_bBlue = 0; m_bRedStep = 8; m_bGreenStep = 8; m_bBlueStep = 8;
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;

    case 0xFF20DF:  /* -Light */
      m_bRed = (m_bRed > m_bRedStep ? m_bRed - m_bRedStep : m_bRed);
      m_bGreen = (m_bGreen > m_bGreenStep ? m_bGreen - m_bGreenStep : m_bGreen);
      m_bBlue = (m_bBlue > m_bBlueStep ? m_bBlue - m_bBlueStep : m_bBlue);
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;
    case 0x00ff10ef:/* Green */
    case 0x797727f7:
      _DBGPRINTF(PSTR("processIRCommand: GREEN\n"));
      m_bRed = 0; m_bGreen = 255; m_bBlue = 0; m_bRedStep = 8; m_bGreenStep = 8; m_bBlueStep = 8;
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;
    case 0xFF30CF:  /* Green-light */
      m_bRed = 63; m_bGreen = 255; m_bBlue = 63; m_bRedStep = 2; m_bGreenStep = 8; m_bBlueStep = 2;
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;
    case 0xFF28D7:  /* Cyan-light*/
      m_bRed = 127; m_bGreen = 255; m_bBlue = 255; m_bRedStep = 4; m_bGreenStep = 8; m_bBlueStep = 8;
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;
    case 0xFF18E7:  /* Cyan */
      m_bRed = 0; m_bGreen = 255; m_bBlue = 255; m_bRedStep = 8; m_bGreenStep = 8; m_bBlueStep = 8;
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;
    case 0xFF08F7:  /* Cyan-Dark */
      m_bRed = 0; m_bGreen = 191; m_bBlue = 191; m_bRedStep = 0; m_bGreenStep = 8; m_bBlueStep = 8;
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;

    case 0xFF609F:  /* OFF */
    case 0xdbfe4c8bUL:
      this->setFullColor(0, 0, 0, 0);
      m_bLedMode = LED_MODE_OFF;
      break;
    case 0xFF50AF:  /* Blue-dark */
      m_bRed = 0; m_bGreen = 0; m_bBlue = 127; m_bRedStep = 0; m_bGreenStep = 0; m_bBlueStep = 4;
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;
    case 0xFF708F:  /* Blue */
    case 0x2a89195f:
    case 0x0b52a82f:
      _DBGPRINTF(PSTR("processIRCommand: BLUE\n"));
      m_bRed = 0; m_bGreen = 0; m_bBlue = 255; m_bRedStep = 8; m_bGreenStep = 8; m_bBlueStep = 8;
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;
    case 0xFF6897:  /* Purple */
      m_bRed = 127; m_bGreen = 0; m_bBlue = 127; m_bRedStep = 4; m_bGreenStep = 0; m_bBlueStep = 4;
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;
    case 0xFF58A7:  /* Pink */
      m_bRed = 255; m_bGreen = 0; m_bBlue = 255; m_bRedStep = 8; m_bGreenStep = 8; m_bBlueStep = 8;
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;
    case 0xFF48B7:  /* Pink-Light */
      m_bRed = 255; m_bGreen = 63; m_bBlue = 195; m_bRedStep = 8; m_bGreenStep = 2; m_bBlueStep = 4;
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;

    case 0xFFE01F:  /* ON */
    case 0x026115c3:
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      m_bLedMode = LED_MODE_AMBIENT;
      break;
    case 0xFFD02F:  /* White */
      m_bRed = 255; m_bGreen = 255; m_bBlue = 255; m_bRedStep = 8; m_bGreenStep = 8; m_bBlueStep = 8;
      this->setFullColor(0, m_bRed, m_bGreen, m_bBlue);
      break;
    case 0xFFF00F:  /* Flash */
      this->colorBlink(m_bRed, m_bGreen, m_bBlue, 4, true);
      break;
    case 0xFFE817:  /* Strobe */
      break;
    case 0xFFD827:  /* Fade */
      break;
    case 0xFFC837:  /* Smooth */
      break;
  }
}

void LEDHelper::toggleLed(int iControllerIndex, int iBrightNess, byte bWhite, byte bRed, byte bGreen, byte bBlue, byte bSteps, bool bLedState) {
  _DBGPRINTF_ULTRA(PSTR("LEDHelper:toggleLed ENTER %d/%d%d/%d/%d/%d/%d/%d\n"), iControllerIndex, iBrightNess, bWhite, bRed, bGreen, bBlue, bSteps, bLedState);

  if (bLedState) {
    _DBGPRINTF_ULTRA(PSTR("toggleLed: TurnON LEDIdx %d\n"), iControllerIndex);
    pStaticLedhelper->colorBlinkUp(iControllerIndex,
                                   iBrightNess,
                                   bWhite,
                                   bRed,
                                   bGreen,
                                   bBlue,
                                   bSteps);
  } else {
    _DBGPRINTF_ULTRA(PSTR("toggleLed: TurnOFF LEDIdx %d\n"), iControllerIndex);
    pStaticLedhelper->colorBlinkDown(iControllerIndex,
                                     iBrightNess,
                                     bWhite,
                                     bRed,
                                     bGreen,
                                     bBlue,
                                     bSteps);
  }
}

void LEDHelper::handleSideBrightnessVertical(void* pVoidleds, int* piLast, byte bBrightnessLevel) {
  if (bBrightnessLevel < 100) {
    // Cast Pointer
    CRGB* pLeds = (CRGB*) pVoidleds;
    // Get Last Position!
    int x = 0;
    int xEnd = this->GetSetupVertical() + *piLast;
    int iRed = 0;
    int iGreen = 0;
    int iBlue = 0;
    for (x = *piLast; x < xEnd; x++) {
      // calc brightness level
      iRed = (pLeds[x].r * bBrightnessLevel) / 100;
      iGreen = (pLeds[x].g * bBrightnessLevel) / 100;
      iBlue = (pLeds[x].b * bBrightnessLevel) / 100;

      // Copy over and check if values are out of bounds
      pLeds[x].r = (byte)(iRed < 255 ? (iRed > 0 ? iRed : 0) : 255);
      pLeds[x].g = (byte)(iGreen < 255 ? (iGreen > 0 ? iGreen : 0) : 255);
      pLeds[x].b = (byte)(iBlue < 255 ? (iBlue > 0 ? iBlue : 0) : 255);
    }
    // Copy position back to iLast
    *piLast = x;
  } else {
    // Only copy position back to iLast
    *piLast = this->GetSetupVertical() + *piLast;
  }
}
void LEDHelper::handleSideBrightnessHorizontal(void* pVoidleds, int* piLast, byte bBrightnessLevel) {
  if (bBrightnessLevel < 100) {
    // Cast Pointer
    CRGB* pLeds = (CRGB*) pVoidleds;
    // Get Last Position!
    int x = 0;
    int xEnd = this->GetSetupHorizontal() + *piLast;
    int iRed = 0;
    int iGreen = 0;
    int iBlue = 0;
    for (x = *piLast; x < xEnd; x++) {
      // calc brightness level
      iRed = (pLeds[x].r * bBrightnessLevel) / 100;
      iGreen = (pLeds[x].g * bBrightnessLevel) / 100;
      iBlue = (pLeds[x].b * bBrightnessLevel) / 100;

      // Copy over and check if values are out of bounds
      pLeds[x].r = (byte)(iRed < 255 ? (iRed > 0 ? iRed : 0) : 255);
      pLeds[x].g = (byte)(iGreen < 255 ? (iGreen > 0 ? iGreen : 0) : 255);
      pLeds[x].b = (byte)(iBlue < 255 ? (iBlue > 0 ? iBlue : 0) : 255);
    }
    // Copy position back to iLast
    *piLast = x;
  } else {
    // Only copy position back to iLast
    *piLast = (this->GetSetupHorizontal() + *piLast);
  }
}
void LEDHelper::handleSideDisableVertical(void* pVoidleds, int* piLast) {
  // Cast Pointer
  CRGB* pLeds = (CRGB*) pVoidleds;
  // Get Last Position!
  int x = 0;
  int xEnd = this->GetSetupVertical() + *piLast;
  for (x = *piLast; x < xEnd; x++) {
    pLeds[x].r = 0;
    pLeds[x].g = 0;
    pLeds[x].b = 0;
  }
  // Copy position back to iLast
  *piLast = x;
}
void LEDHelper::handleSideDisableHorizontal(void* pVoidleds, int* piLast) {
  // Cast Pointer
  CRGB* pLeds = (CRGB*) pVoidleds;
  // Get Last Position!
  int x = 0;
  int xEnd = this->GetSetupHorizontal() + *piLast;
  for (x = *piLast; x < xEnd; x++) {
    pLeds[x].r = 0;
    pLeds[x].g = 0;
    pLeds[x].b = 0;
  }
  // Copy position back to iLast
  *piLast = x;
}

// Task Function Helpers
#if defined(ESP32)
void TaskFadeOutCode(void * pvParameters) {
  do {
    // Timeout, turn OFF the lights!
    _DBGPRINTF(PSTR("TaskFadeOut: timeout %d ms - %d hit - turn off.\n"), pStaticLedhelper->GetVideoTimeout(), pStaticLedhelper->GetLastUpdate());
    pStaticLedhelper->FadeOut(1000);
    pStaticLedhelper->setFullColor(0);

    // Make sure NewLedData is set to FALSE!
    pStaticLedhelper->SetIsNewLedData(false);
    
    // Fadeout done
    pStaticLedhelper->setFadeOutRunning(false);

    // Suspend ourselves.
    _DBGPRINTF(PSTR("TaskFadeOut: Suspend TASK after FadeOut - handle %d \n"), handleTaskFadeOut);
    vTaskSuspend(handleTaskFadeOut);
  } while (true);
}

void TaskProcessNextStep(void * pvParameters) {
  _DBGPRINTF(PSTR("TaskProcessNextStep: ENTER\n"));
  do {
    _DBGPRINTF_ULTRA(PSTR("TaskProcessNextStep: ProcessLedNextStep Loop Begin\n"));

    // In Processing now
    pStaticLedhelper->setProcessingLedNextStep(true);

    /* Process LED Data Frame */
    pStaticLedhelper->ProcessLedNextStep();

    // Processing done
    pStaticLedhelper->setProcessingLedNextStep(false);
    /*
        // Wait until we get new work todo!
        while( pStaticLedhelper->getProcessingLedNextStep() == false ) {
          _taskDelay(1);
          // _yield();
        }
    */
    // Suspend ourselves.
    _DBGPRINTF_ULTRA(PSTR("TaskProcessNextStep: Suspend TASK after ProcessLedNextStep - handle %d \n"), handleTaskProcessNextStep);
    vTaskSuspend(handleTaskProcessNextStep);
  } while (true);

  _DBGPRINTF(PSTR("TaskProcessNextStep: EXIT\n"));
}

void TaskDemoCode(void * pvParameters) {
  _DBGPRINTF(PSTR("TaskDemoCode: ENTER\n"));

  // Main Task LOOP
  do {
    byte bRed   = 255;
    byte bGreen = 127;
    byte bBlue  = 0;
    byte bDemoFrames = 0;
    byte bDemoFramesCtr = 0;

    // Main Loop
    while (pStaticLedhelper->GetLedMode() == LED_MODE_DEMO) {
# if defined(ENABLE_RELAY)
      if (global::GetAmbibackRelaymodeEnabled() && global::GetAmbibackRelayModeDirect() == 1) {
        // Check for WIFI State! - Sleep if not connected!
        while (WiFi.status() != WL_CONNECTED) {
          _DBGPRINTF(PSTR("TaskDemoCode: Error, WiFi DISCONNECTED - wait 1000ms!\n"));
          _taskDelay(1000);
          continue;
        }
      }
# endif

      // Starttimer now!
      m_nTimeStart = millis();

      /* Set Framelength to limit processing
          NET_SLOW_FPS_FRAME FPS while OTA update
          NET_SLOW_FPS_FRAME FPS while SEND Failures
          NET_FAST_FPS_FRAME FPS for testing
      */
      int iCurrenFrameLength = (global::GetFirmwareRunning()
# if defined(ENABLE_RELAY)
                                || global::GetAmbibackRelayFailed() > 0
# endif
                                ? NET_SLOW_FPS_FRAME : NET_FAST_FPS_FRAME );

      // Change LED Color - Move Colorwheel
      for (int idx = 0; idx < (int)pStaticLedhelper->getNumLeds(); idx++) {
        m_leds_next[idx].r = bRed;
        m_leds_next[idx].g = (idx % 100 ? bGreen : bGreen++);
        m_leds_next[idx].b = (idx % 10 ? bBlue : bBlue++);
      }

      bRed++;
      bGreen++;

      /* ZERO unconfigured LEDs */
      pStaticLedhelper->ZeroConfiguredLeds();

      // Wait again for OLD TASK to finish its work!
      while ( pStaticLedhelper->getProcessingLedNextStep() ) {
        _taskYIELD();
      }

      bDemoFramesCtr++;

      // ENDTimer now!
      m_nTimeFinish =  (millis() - m_nTimeStart);

      // Calculate dynamic Delay Length now!
      global::CalcDynamicDelayLength( m_nTimeStart );

#if defined(ENABLE_RELAY)
      if (global::GetAmbibackRelaymodeEnabled() && global::GetAmbibackRelayModeDirect() == 1) {
        // Prepare direct data
        pStaticLedhelper->RelayPrepareDirectLedData();
        // In direct mode, we skip processing by LED Task and send direct!
        pStaticLedhelper->RelayLedData();
      } else
#endif
      {
        // Set TaskProcessNextStep work | wakes up NextStep Task
        pStaticLedhelper->setProcessingLedNextStep(true);

        // Make sure our Process Task is running
        pStaticLedhelper->EnableTaskProcessNextStep();
      }

      /* Output in ULTRA Debug, or every second only! */
      if (m_nTimeFPSCheck == 0 ||
          millis() > m_nTimeFPSCheck) {
        // Set Demo Counter
        bDemoFrames = bDemoFramesCtr;
        bDemoFramesCtr = 0;

        /* Debug Output */
        _DBGPRINTF(PSTR("TaskDemoCode: PROCESSED in %d ms, DelayLen=%d, DemoFPS=%d \n"),
                   m_nTimeFinish,
                   pStaticLedhelper->GetDelayLength(),
                   bDemoFrames
                  );

        // Next in 1000 ms
        m_nTimeFPSCheck = millis() + 1000;
      }



      // Sleep if necessary
      //      _taskYIELD();
      if (iCurrenFrameLength > m_nTimeFinish) {
        _taskDelay(iCurrenFrameLength - m_nTimeFinish);
      }
    }

    _DBGPRINTF(PSTR("TaskDemoCode: DemoLOOP END!\n"));

    /* Start LED fadeOut */
    pStaticLedhelper->EnableTaskFadeOut();

    // handle other tasks
    _taskYIELD();

    // Suspend ourselves.
    _DBGPRINTF(PSTR("TaskCamCode: Suspend TASK for ourself %d \n"), handleTaskDemo);
    vTaskSuspend(handleTaskDemo);
  } while (true);

  _DBGPRINTF(PSTR("TaskDemoCode: EXIT\n"));
}

// Simple deterministic "random" for ambient presets: returns 0-255 from frame and index.
static byte ambientNoise(unsigned long frame, int i) {
  unsigned long v = (frame * 31UL + (unsigned long)(i & 0xFF)) * 2654435761UL;
  return (byte)(v >> 16);
}
static byte ambientAlphaStyleValue(int intensity, float wave) {
  if (intensity < 1) intensity = 1;
  if (intensity > 100) intensity = 100;
  if (wave < 0.0f) wave = 0.0f;
  if (wave > 1.0f) wave = 1.0f;

  float floorFrac = (100.0f - (float)intensity) / 100.0f;
  if (floorFrac < 0.0f) floorFrac = 0.0f;
  if (floorFrac > 1.0f) floorFrac = 1.0f;

  float bright = floorFrac + (1.0f - floorFrac) * wave;
  int val = (int)(bright * 255.0f);
  if (val < 0) val = 0;
  if (val > 255) val = 255;
  return (byte)val;
}

// Ambient preset frame update: writes one frame into pLeds. Preset 1-11, speed 1-10, intensity 1-100.
static void UpdateAmbientFrame(CRGB* pLeds, int numLeds, int preset, int speed, int intensity, unsigned long frameCount) {
  if (numLeds <= 0 || !pLeds) return;
  int scale = (intensity * 255) / 100;
  if (scale > 255) scale = 255;
  if (scale < 1) scale = 1;
  int phase = (int)(frameCount & 0xFF);
  int phase2 = (int)((frameCount * 3) & 0xFF);
  for (int i = 0; i < numLeds; i++) {
    byte n = ambientNoise(frameCount, i);
    byte n2 = ambientNoise(frameCount + 1, i + 17);
    int pos = (i * 256) / (numLeds > 1 ? numLeds : 1);
    int wave = (phase + pos / 2) & 0xFF;
    int wave2 = (phase2 + i * 4) & 0xFF;
    int tri = wave < 128 ? wave * 2 : (255 - wave) * 2;
    int tri2 = wave2 < 128 ? wave2 * 2 : (255 - wave2) * 2;
    int r = 0, g = 0, b = 0;
    switch (preset) {
      case 1: { /* Fire - hotter, irregular flame tongues with ember bursts */
        int speedStep = speed;
        if (speedStep < 1) speedStep = 1;
        if (speedStep > 10) speedStep = 10;

        int motion = 1 + (speedStep / 3);
        byte n3 = ambientNoise(frameCount + (unsigned long)(i * 11), i + 29);
        byte n4 = ambientNoise((frameCount >> 1) + (unsigned long)(i * 5), i + 47);
        int pos = (i * 256) / (numLeds > 1 ? numLeds : 1);
        int rise = 255 - pos;

        int flame = 54 + (rise * 112 / 255);
        flame += ((int)(n & 0x1F) - 8) * 2;
        flame += ((int)(n2 & 0x0F) - 4) * 3;
        flame += ((int)(n3 & 0x0F) - 4) * 4;
        flame += ((int)(n4 & 0x07) - 2) * 6;
        flame += ((int)((frameCount * (unsigned long)motion + (unsigned long)(i * (3 + motion))) & 0x1F) - 16);

        if (n > 236) {
          flame += 36 + ((n - 236) * 3);
        } else if (n > 210) {
          flame += 6;
        }

        if (flame < 0) flame = 0;
        if (flame > 255) flame = 255;

        CRGB c = HeatColor((byte)flame);
        r = c.r;
        g = (c.g * 210) / 255;
        b = c.b / 5;

        if (pos < 96) {
          r += 28;
          g = (g * 85) / 100;
        } else if (pos > 160) {
          r += 10;
          g += 24;
        } else {
          r += 16;
          g += 10;
        }

        if (n4 > 240) {
          r += 18;
          g += 8;
        }

        r = (r * scale) / 255;
        g = (g * scale) / 255;
        b = (b * scale) / 255;

        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        break;
      }
      case 2: { /* Christmas - traditional red and green with stable hue under dimming */
        byte n3 = ambientNoise(frameCount + 7, i + 31);
        int twinkle = (n > 240) ? ((n2 + n3) * scale / 420) : 0;
        byte festiveHue = ((i + phase) & 1) == 0 ? 0 : 96;
        byte festiveValue = (byte)((scale * 82) / 100);
        festiveValue = qadd8(festiveValue, (byte)(twinkle / 3));

        if (n3 > 245) {
          festiveValue = qadd8(festiveValue, (byte)(((n3 - 245) * scale) / 40));
        }

        CRGB c = CHSV(festiveHue, 255, festiveValue);
        r = c.r;
        g = c.g;
        b = c.b;
        break;
      }
      case 3: /* Halloween - darker, richer orange/purple */
        r = (scale * (140 + (n % 70))) / 100;
        g = (scale * (45 + (n2 % 35))) / 100;
        b = (scale * (80 + (n % 70))) / 100;
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        break;
      case 4: /* Ocean - more contrast: deep blue to bright cyan */
        b = (scale * (120 + (tri * 135 / 256))) / 100;
        g = (scale * (60 + (tri2 * 195 / 256))) / 100;
        r = (scale * (5 + (n % 25))) / 100;
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        break;
      case 5: { /* Sunset - warm orange/pink/red with stable hue under dimming */
        byte sunsetHue;
        if (tri < 85) {
          sunsetHue = 8;   /* deep orange */
        } else if (tri < 170) {
          sunsetHue = 18;  /* sunset amber */
        } else {
          sunsetHue = 236; /* pink/red twilight */
        }

        byte sunsetSat = 220;
        byte sunsetVal = (byte)((scale * (72 + (tri2 * 18 / 256))) / 100);
        if (n > 240) {
          sunsetVal = qadd8(sunsetVal, (byte)((n % 24) / 3));
        }

        CRGB c = CHSV(sunsetHue, sunsetSat, sunsetVal);
        r = c.r;
        g = c.g;
        b = c.b;
        break;
      }
      case 6: { /* Aurora - vivid green/blue/purple with stable hue under dimming */
        byte auroraHue;
        if (tri < 64) {
          auroraHue = 96;   /* green */
        } else if (tri < 128) {
          auroraHue = 120;  /* cyan */
        } else if (tri < 192) {
          auroraHue = 176;  /* blue */
        } else {
          auroraHue = 208;  /* purple */
        }

        byte auroraSat = 255;
        byte auroraVal = (byte)((scale * (68 + (tri2 * 22 / 256))) / 100);
        if (n > 240) {
          auroraVal = qadd8(auroraVal, (byte)((n % 20) / 4));
        }

        CRGB c = CHSV(auroraHue, auroraSat, auroraVal);
        r = c.r;
        g = c.g;
        b = c.b;
        break;
      }
      case 7: { /* Candle - steadier orange/yellow flame with a small red core */
        int speedStep = speed;
        if (speedStep < 1) speedStep = 1;
        if (speedStep > 10) speedStep = 10;

        byte n3 = ambientNoise((frameCount >> 2) + (unsigned long)(i * 7), i + 23);
        byte n4 = ambientNoise((frameCount >> 3) + (unsigned long)(i * 3), i + 41);
        int pos = (i * 256) / (numLeds > 1 ? numLeds : 1);
        int taper = 255 - pos;

        int flame = 36 + (taper * 58 / 255);
        flame += ((int)(n & 0x0F) - 7) * 2;
        flame += ((int)(n2 & 0x07) - 3) * 3;
        flame += ((int)(n3 & 0x07) - 3) * 2;
        flame += ((int)(n4 & 0x03) - 1) * 4;
        flame += (((int)frameCount + i * (1 + speedStep / 4)) & 0x0F) - 8;

        if (flame < 0) flame = 0;
        if (flame > 255) flame = 255;

        CRGB c = HeatColor((byte)flame);
        r = (c.r * 160) / 255;
        g = (c.g * 235) / 255;
        b = c.b / 18;

        if (pos < 88) {
          r = (r * 70) / 100;
          g += 18;
        } else if (pos > 168) {
          r = (r * 45) / 100;
          g += 28;
          if (g > 255) g = 255;
        } else {
          r = (r * 58) / 100;
          g += 22;
        }

        if (pos > 96 && pos < 156) {
          r += 20;
        } else {
          r = (r * 88) / 100;
        }

        r = (r * scale) / 255;
        g = (g * scale) / 255;
        b = (b * scale) / 255;

        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        break;
      }
      case 8: /* Rainbow */
        fill_rainbow(pLeds, numLeds, (byte)((frameCount * (2 + speed)) & 0xFF), 256 / (numLeds > 0 ? numLeds : 1));
        for (int j = 0; j < numLeds; j++) {
          pLeds[j].r = (pLeds[j].r * scale) / 255;
          pLeds[j].g = (pLeds[j].g * scale) / 255;
          pLeds[j].b = (pLeds[j].b * scale) / 255;
        }
        return;
      case 9: /* Thunderstorm - more black between lightning */
        if ((n + frameCount) % 80 < 2) {
          int flash = (n2 * scale / 255) + 120;
          if (flash > 255) flash = 255;
          r = flash;
          g = flash;
          b = flash;
        } else {
          r = (scale * 4) / 100;
          g = (scale * 5) / 100;
          b = (scale * 12) / 100;
        }
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        break;
      case 10: { /* Prism Drift - broader layered gradients with stronger color spread */
        int shimmer = ambientNoise(frameCount + (unsigned long)(i * 3), i + 11);
        int shimmer2 = ambientNoise((frameCount >> 1) + (unsigned long)(i * 13), i + 37);
        int colorSpan = numLeds / 4;
        if (colorSpan < 12) colorSpan = 12;
        int colorBand = i / colorSpan;
        byte hue1 = (byte)((frameCount * (1 + speed / 3) + colorBand * 34 + pos / 4 + (shimmer >> 3)) & 0xFF);
        byte hue2 = (byte)((hue1 + 88 + (shimmer2 >> 2)) & 0xFF);
        byte hue3 = (byte)((hue1 + 176 + (shimmer2 >> 3)) & 0xFF);
        byte value1 = (byte)((scale * (210 + (tri / 3))) / 255);
        byte value2 = (byte)((scale * (180 + (tri2 / 2))) / 255);
        byte value3 = (byte)((scale * (150 + (shimmer >> 3))) / 255);
        CRGB c = CHSV(hue1, 255, value1);
        c += CHSV(hue2, 255, value2);
        c += CHSV(hue3, 255, value3);
        if ((shimmer & 0xF0) == 0xF0) {
          c += CRGB::White;
        }
        r = c.r;
        g = c.g;
        b = c.b;
        break;
      }
      case 11: { /* Neon Surge - longer colorful bands with sharper moving accents */
        int blockLen = numLeds / 6;
        if (blockLen < 8) blockLen = 8;
        if (speed > 0) blockLen += speed / 2;
        int lane = i / blockLen;
        int chase = (int)((frameCount * (4 + speed) + i * (2 + speed)) & 0xFF);
        byte mainHue = (byte)((lane * 41 + frameCount * (7 + speed * 2)) & 0xFF);
        byte accentHue = (byte)((mainHue + 80 + (chase >> 2)) & 0xFF);
        byte flashHue = (byte)((mainHue + 165 + (chase >> 1)) & 0xFF);
        CRGB c;
        int phase = (i + (int)frameCount * (2 + speed)) % (blockLen * 3);
        if (phase < blockLen) {
          c = CHSV(mainHue, 255, scale);
        } else if (phase < blockLen * 2) {
          c = CHSV(accentHue, 240, (byte)((scale * 230) / 255));
        } else {
          c = CHSV(flashHue, 220, (byte)((scale * 180) / 255));
        }
        if ((ambientNoise(frameCount + 3, i + 19) & 0xE0) == 0xE0) {
          c += CRGB::White;
        }
        if (((i + (int)frameCount) & 5) == 0) {
          c += CHSV((byte)(mainHue + 32), 180, (byte)(scale / 2));
        }
        r = c.r;
        g = c.g;
        b = c.b;
        break;
      }
      case 12: { /* 40 Hz Gamma: intensity sets the floor, peak follows global brightness */
        r = 0;
        g = 0;
        b = ambientAlphaStyleValue(intensity, (frameCount & 1) ? 0.0f : 1.0f);
        break;
      }
      case 13: { /* 10 Hz Alpha: 12 frames/cycle (~8ms pipeline), intensity sets the floor */
        const int kAlphaFramesPerCycle = 12;
        float phase = (float)(frameCount % kAlphaFramesPerCycle) / (float)kAlphaFramesPerCycle;
        float wave = sinf(phase * 3.14159265f);
        if (wave < 0) wave = 0;
        r = ambientAlphaStyleValue(intensity, wave);
        g = 0;
        b = 0;
        break;
      }
      case 14: { /* 34 Hz Gamma (Red): intensity sets the floor, peak follows global brightness */
        r = ambientAlphaStyleValue(intensity, (frameCount & 1) ? 0.0f : 1.0f);
        g = 0;
        b = 0;
        break;
      }
      case 15: { /* Rotlicht Abend: intensity sets the floor; wave 0–1 so intensity=100 reaches black */
        const int kBreathFramesPerCycle = 50;
        float phase = (float)(frameCount % kBreathFramesPerCycle) / (float)kBreathFramesPerCycle;
        float wave = (1.0f + sinf(phase * 6.28318531f)) * 0.5f;
        r = ambientAlphaStyleValue(intensity, wave);
        g = 0;
        b = 0;
        break;
      }
      default:
        r = scale / 3;
        g = scale / 4;
        b = 0;
        break;
    }
    pLeds[i].r = (byte)(r > 255 ? 255 : (r < 0 ? 0 : r));
    pLeds[i].g = (byte)(g > 255 ? 255 : (g < 0 ? 0 : g));
    pLeds[i].b = (byte)(b > 255 ? 255 : (b < 0 ? 0 : b));
  }
}

void TaskAmbientCode(void * pvParameters) {
  _DBGPRINTF(PSTR("TaskAmbientCode: ENTER\n"));
  unsigned long frameCount = 0;
  do {
    while (pStaticLedhelper->GetLedMode() == LED_MODE_AMBIENT && global::GetAmbientPreset() != 0) {
      int preset = global::GetAmbientPreset();
      int speed = global::GetAmbientSpeed();
      int intensity = (preset >= 12) ? global::GetAmbientHealthIntensity() : global::GetAmbientIntensity();
      int numLeds = pStaticLedhelper->getNumLeds();
      unsigned long t0Us = (preset == 10 || preset == 11 || preset == 12 || preset == 13 || preset == 14) ? micros() : 0;
      UpdateAmbientFrame(m_leds_next, numLeds, preset, speed, intensity, frameCount);
      frameCount++;
      pStaticLedhelper->ZeroConfiguredLeds();
      while (pStaticLedhelper->getProcessingLedNextStep()) {
        _taskYIELD();
      }
      pStaticLedhelper->setProcessingLedNextStep(true);
      pStaticLedhelper->EnableTaskProcessNextStep();
      if (preset == 10) {
        /* Prism Drift: slower cinematic motion with speed-adjusted cadence. */
        const unsigned long kPrismFrameUs = 50000UL;
        const unsigned long kPrismSpeedStepUs = 2500UL;
        unsigned long targetUs = kPrismFrameUs;
        unsigned long speedStep = (unsigned long)((speed > 0) ? speed : 1) * kPrismSpeedStepUs;
        if (targetUs > speedStep)
          targetUs -= speedStep;
        if (targetUs < 24000UL)
          targetUs = 24000UL;
        unsigned long elapsed = micros() - t0Us;
        if (elapsed < targetUs)
          _delayMicroseconds(targetUs - elapsed);
      } else if (preset == 11) {
        /* Neon Surge: faster pulse-chase motion with tighter gaps. */
        const unsigned long kNeonFrameUs = 26000UL;
        const unsigned long kNeonSpeedStepUs = 1400UL;
        unsigned long targetUs = kNeonFrameUs;
        unsigned long speedStep = (unsigned long)((speed > 0) ? speed : 1) * kNeonSpeedStepUs;
        if (targetUs > speedStep)
          targetUs -= speedStep;
        if (targetUs < 12000UL)
          targetUs = 12000UL;
        unsigned long elapsed = micros() - t0Us;
        if (elapsed < targetUs)
          _delayMicroseconds(targetUs - elapsed);
      } else if (preset == 12) {
        /* 40 Hz Gamma: target 12.5 ms half-period; sleep only the remainder after pipeline work.
         * Add small correction (~150 µs): ESP32 delayMicroseconds/micros() often runs ~1% short. */
        const unsigned long k40HzHalfPeriodUs = 12500;
        const unsigned long k40HzCorrectionUs = 150;
        unsigned long elapsed = micros() - t0Us;
        unsigned long targetUs = k40HzHalfPeriodUs + k40HzCorrectionUs;
        if (elapsed < targetUs)
          _delayMicroseconds(targetUs - elapsed);
#if defined(ENABLE_NORMAL_DEBUG)
        /* Optional rate verification: every 80 half-cycles (≈1 s) print measured frequency */
        {
          static unsigned long s_40hzStartUs = 0;
          static int s_40hzCycles = 0;
          if (s_40hzCycles == 0)
            s_40hzStartUs = micros();
          s_40hzCycles++;
          if (s_40hzCycles >= 80) {
            unsigned long elapsed80 = micros() - s_40hzStartUs;
            if (elapsed80 > 0) {
              /* 80 half-cycles = 40 full cycles; elapsed in µs → freq = 40e6 / elapsed */
              float freq = 40.0f * 1000000.0f / (float)elapsed80;
              float periodMs = (float)elapsed80 / 80.0f / 1000.0f;
              _DBGPRINTF(PSTR("40Hz Gamma: %.1f Hz (target 40), half-period %.2f ms\n"), (double)freq, (double)periodMs);
            }
            s_40hzStartUs = micros();
            s_40hzCycles = 0;
          }
        }
#endif
      } else if (preset == 13) {
        /* 10 Hz Alpha: 12 frames per 100 ms cycle → ~8.3 ms per frame (pipeline-limited, no extra delay) */
        const unsigned long k10HzPeriodUs = 100000;
        const int kAlphaFramesPerCycle = 12;
        unsigned long elapsed = micros() - t0Us;
        unsigned long targetUs = k10HzPeriodUs / kAlphaFramesPerCycle;
        if (elapsed < targetUs)
          _delayMicroseconds(targetUs - elapsed);
#if defined(ENABLE_NORMAL_DEBUG)
        {
          static unsigned long s_13StartUs = 0;
          static int s_13Cycles = 0;
          if (s_13Cycles == 0) s_13StartUs = micros();
          s_13Cycles++;
          if (s_13Cycles >= kAlphaFramesPerCycle) {
            unsigned long elapsedUs = micros() - s_13StartUs;
            if (elapsedUs > 0) {
              float freq = 1000000.0f / (float)elapsedUs;
              float periodMs = (float)elapsedUs / 1000.0f;
              _DBGPRINTF(PSTR("10Hz Alpha: %.1f Hz (target 10), period %.2f ms\n"), (double)freq, (double)periodMs);
            }
            s_13StartUs = micros();
            s_13Cycles = 0;
          }
        }
#endif
      } else if (preset == 14) {
        /* 34 Hz Gamma Red: half-period ≈ 14.7 ms */
        const unsigned long k34HzHalfPeriodUs = 14706;
        const unsigned long k34HzCorrectionUs = 150;
        unsigned long elapsed = micros() - t0Us;
        unsigned long targetUs = k34HzHalfPeriodUs + k34HzCorrectionUs;
        if (elapsed < targetUs)
          _delayMicroseconds(targetUs - elapsed);
#if defined(ENABLE_NORMAL_DEBUG)
        {
          static unsigned long s_14StartUs = 0;
          static int s_14Cycles = 0;
          if (s_14Cycles == 0) s_14StartUs = micros();
          s_14Cycles++;
          if (s_14Cycles >= 68) {
            unsigned long elapsedUs = micros() - s_14StartUs;
            if (elapsedUs > 0) {
              float freq = 34.0f * 1000000.0f / (float)elapsedUs;
              float periodMs = (float)elapsedUs / 68.0f / 1000.0f;
              _DBGPRINTF(PSTR("34Hz Gamma Red: %.1f Hz (target 34), half-period %.2f ms\n"), (double)freq, (double)periodMs);
            }
            s_14StartUs = micros();
            s_14Cycles = 0;
          }
        }
#endif
      } else if (preset == 15) {
        /* Rotlicht Abend: Speed 5 (default) = 1 Hz, period = 5000/speed ms. 50 frames per cycle. */
        unsigned long periodMs = (speed > 0) ? (5000UL / (unsigned long)speed) : 1000;
        if (periodMs < 200) periodMs = 200;
        unsigned long frameMs = periodMs / 50;
        if (frameMs < 10) frameMs = 10;
        _taskDelay((int)frameMs);
#if defined(ENABLE_NORMAL_DEBUG)
        {
          static unsigned long s_15StartMs = 0;
          static int s_15Cycles = 0;
          if (s_15Cycles == 0) s_15StartMs = millis();
          s_15Cycles++;
          if (s_15Cycles >= 50) {
            unsigned long elapsedMs = millis() - s_15StartMs;
            if (elapsedMs > 0) {
              float targetFreq = (float)speed / 5.0f;
              float freq = 1000.0f / (float)elapsedMs;
              float periodMsAct = (float)elapsedMs;
              _DBGPRINTF(PSTR("Rotlicht Abend: %.2f Hz (target %.2f), period %.0f ms\n"), (double)freq, (double)targetFreq, (double)periodMsAct);
            }
            s_15StartMs = millis();
            s_15Cycles = 0;
          }
        }
#endif
      } else {
        int frameMs = 120 - (speed * 9);
        if (frameMs < 25) frameMs = 25;
        _taskDelay(frameMs);
      }
    }
    _DBGPRINTF(PSTR("TaskAmbientCode: leaving AMBIENT or preset=0, suspend\n"));
    vTaskSuspend(handleTaskAmbient);
    _taskYIELD();
  } while (true);
}

#if defined(ENABLE_CAM)
bool LEDHelper::camCalcNextLedStep(void* pVoidCamleds) {
  if (!pVoidCamleds || !global::m_pCamProcessor) return false;
  int iLast = 0;
  camCalcNextLedStepLeft(pVoidCamleds, &iLast, false, false);
  camCalcNextLedStepTop(pVoidCamleds, &iLast, false, false);
  camCalcNextLedStepRight(pVoidCamleds, &iLast, false, false);
  camCalcNextLedStepBottom(pVoidCamleds, &iLast, false, false);
  return true;
}

void LEDHelper::camCalcNextLedStepLeft(void* pVoidCamleds, int* piLast, bool bReverse, bool bLast) {
  CRGB* pCam = (CRGB*)pVoidCamleds;
  byte d = global::m_pCamProcessor->GetCamDetailLevel();
  int iStart = *piLast;
  int iEnd = iStart + GetSetupVertical();
  int iSectorBase = (global::m_pCamProcessor->GetCamMode() == LED_MODE_CAM_SINGLE) ? 0 : 1;
  for (int i = iStart; i < iEnd && i < (int)getNumLeds(); i++) {
    int idx = (iEnd - iStart) > 0 ? (bReverse ? (iEnd - 1 - (i - iStart)) : (i - iStart)) : 0;
    int sec = iSectorBase + (d > 0 ? (idx * d / (iEnd - iStart)) : 0);
    if (sec < 0) sec = 0;
    m_leds_next[i].r = pCam[sec].r; m_leds_next[i].g = pCam[sec].g; m_leds_next[i].b = pCam[sec].b;
  }
  (void)bLast;
  *piLast = iEnd;
}

void LEDHelper::camCalcNextLedStepTop(void* pVoidCamleds, int* piLast, bool bReverse, bool bLast) {
  CRGB* pCam = (CRGB*)pVoidCamleds;
  byte d = global::m_pCamProcessor->GetCamDetailLevel();
  int iStart = *piLast;
  int iEnd = iStart + GetSetupHorizontal();
  int iSectorBase = (global::m_pCamProcessor->GetCamMode() == LED_MODE_CAM_SINGLE) ? 0 : (1 + (int)d);
  for (int i = iStart; i < iEnd && i < (int)getNumLeds(); i++) {
    int idx = (iEnd - iStart) > 0 ? (bReverse ? (iEnd - 1 - (i - iStart)) : (i - iStart)) : 0;
    int sec = iSectorBase + (d > 0 ? (idx * d / (iEnd - iStart)) : 0);
    if (sec < 0) sec = 0;
    m_leds_next[i].r = pCam[sec].r; m_leds_next[i].g = pCam[sec].g; m_leds_next[i].b = pCam[sec].b;
  }
  (void)bLast;
  *piLast = iEnd;
}

void LEDHelper::camCalcNextLedStepRight(void* pVoidCamleds, int* piLast, bool bReverse, bool bLast) {
  CRGB* pCam = (CRGB*)pVoidCamleds;
  byte d = global::m_pCamProcessor->GetCamDetailLevel();
  int iStart = *piLast;
  int iEnd = iStart + GetSetupVertical();
  int iSectorBase = (global::m_pCamProcessor->GetCamMode() == LED_MODE_CAM_SINGLE) ? 0 : (1 + 2 * (int)d);
  for (int i = iStart; i < iEnd && i < (int)getNumLeds(); i++) {
    int idx = (iEnd - iStart) > 0 ? (bReverse ? (iEnd - 1 - (i - iStart)) : (i - iStart)) : 0;
    int sec = iSectorBase + (d > 0 ? (idx * d / (iEnd - iStart)) : 0);
    if (sec < 0) sec = 0;
    m_leds_next[i].r = pCam[sec].r; m_leds_next[i].g = pCam[sec].g; m_leds_next[i].b = pCam[sec].b;
  }
  (void)bLast;
  *piLast = iEnd;
}

void LEDHelper::camCalcNextLedStepBottom(void* pVoidCamleds, int* piLast, bool bReverse, bool bLast) {
  CRGB* pCam = (CRGB*)pVoidCamleds;
  byte d = global::m_pCamProcessor->GetCamDetailLevel();
  int iStart = *piLast;
  int iEnd = iStart + GetSetupHorizontal();
  int iSectorBase = (global::m_pCamProcessor->GetCamMode() == LED_MODE_CAM_SINGLE) ? 0 : (1 + 3 * (int)d);
  for (int i = iStart; i < iEnd && i < (int)getNumLeds(); i++) {
    int idx = (iEnd - iStart) > 0 ? (bReverse ? (iEnd - 1 - (i - iStart)) : (i - iStart)) : 0;
    int sec = iSectorBase + (d > 0 ? (idx * d / (iEnd - iStart)) : 0);
    if (sec < 0) sec = 0;
    m_leds_next[i].r = pCam[sec].r; m_leds_next[i].g = pCam[sec].g; m_leds_next[i].b = pCam[sec].b;
  }
  (void)bLast;
  *piLast = iEnd;
}

bool LEDHelper::SetLedBufferFromCamSectors(void* pVoidCamleds) {
  return camCalcNextLedStep(pVoidCamleds);
}

#if defined(ENABLE_CAM) && defined(ESP32)
void LEDHelper::deleteTaskCam() {
  CamTask_Delete();
}
#endif

#endif /* ENABLE_CAM */
#endif /* ESP32 */
