#include "globalincludes.h"
#include "global.h"

#ifdef ENABLE_SSD1306
#include "SSD1306.h"
#include "OLEDDisplayUi.h"

SSD1306* m_pOled = NULL;
OLEDDisplayUi* m_pUI = NULL;
int m_iScreenWidth = 0;
int m_iScreenHeight = 0;
char m_szStr1[SSD1306_MAXSTR];
char m_szStr2[SSD1306_MAXSTR];
char m_szStr3[SSD1306_MAXSTR];
int m_iTextAlign = TEXT_ALIGN_CENTER;
bool m_bTextAvailable = false;

static void drawFrame1(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  int x1 = m_iScreenWidth / 2;
  display->setFont(ArialMT_Plain_10);
  display->drawString(x1 + x, y, (WiFi.status() == WL_CONNECTED ? PSTR("Wifi Connected") : PSTR("Wifi Disconnected")));
  if (y + 10 < m_iScreenHeight) {
    char buf[64];
    IPAddress ip = WiFi.localIP();
    snprintf_P(buf, sizeof(buf), PSTR("IP: %d.%d.%d.%d"), ip[0], ip[1], ip[2], ip[3]);
    display->drawString(x1 + x, y + 10, buf);
  }
#ifdef ENABLE_AS312
  if (digitalRead(AS312_PIN) && (y + 20 < m_iScreenHeight)) {
    display->drawString(x1 + x, y + 20, PSTR("Movement detected"));
  }
#endif
  if (global::GetButtonPressed()) {
    unsigned long timePassed = (millis() - global::GetButtonPressedStart()) / 1000;
    char buf[32];
    snprintf_P(buf, sizeof(buf), PSTR("Button pressed for %lu"), timePassed);
    display->drawString(64 + x,
#ifdef ENABLE_AS312
                        30
#else
                        20
#endif
                        + y, buf);
  }
}

static void drawFrame2(OLEDDisplay* display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64 + x, 0 + y, PSTR("AmbiBack Ready!"));
  if (y + 10 < m_iScreenHeight) {
    char buf[64];
    IPAddress ip = WiFi.localIP();
    snprintf_P(buf, sizeof(buf), PSTR("http://%d.%d.%d.%d"), ip[0], ip[1], ip[2], ip[3]);
    display->drawString(64 + x, 10 + y, buf);
  }
  if (y + 20 < m_iScreenHeight)
    display->drawString(64 + x, 20 + y, PSTR("to connect"));
  if (global::GetButtonPressed()) {
    unsigned long timePassed = (millis() - global::GetButtonPressedStart()) / 1000;
    char buf[32];
    snprintf_P(buf, sizeof(buf), PSTR("Button pressed for %lu"), timePassed);
    display->drawString(64 + x, 30 + y, buf);
  }
}

static FrameCallback frames[] = {drawFrame1, drawFrame2};
#define FRAMES_SIZE (sizeof(frames) / sizeof(frames[0]))

void global::oledSetObj(void* pOled) {
  m_pOled = (SSD1306*)pOled;
  m_iScreenWidth = m_pOled->getWidth();
  m_iScreenHeight = m_pOled->getHeight();
  _DBGPRINTF(PSTR("global: oledSetObj: Width=%d Height=%d\n"), m_iScreenWidth, m_iScreenHeight);
}

void global::uiSetObj(void* pUI) {
  m_pUI = (OLEDDisplayUi*)pUI;
}

void global::uiReset() {
  m_pUI->setFrames(frames, FRAMES_SIZE);
}

void global::uiInit() {
  m_pUI->setTargetFPS(20);
  m_pUI->setIndicatorPosition(BOTTOM);
  m_pUI->setIndicatorDirection(LEFT_RIGHT);
  m_pUI->setFrameAnimation(SLIDE_LEFT);
  m_pUI->setFrames(frames, FRAMES_SIZE);
  m_pUI->setTimePerFrame(6000);
}

void global::oledDrawStringQueue(String str1, String str2, String str3) {
  size_t stLength = (str1.length() >= SSD1306_MAXSTR ? SSD1306_MAXSTR : str1.length());
  memcpy(&m_szStr1, str1.c_str(), stLength);
  m_szStr1[stLength] = '\0';
  if (!str2.isEmpty()) {
    stLength = (str2.length() >= SSD1306_MAXSTR ? SSD1306_MAXSTR : str2.length());
    memcpy(&m_szStr2, str2.c_str(), stLength);
    m_szStr2[stLength] = '\0';
  } else {
    m_szStr2[0] = '\0';
  }
  if (!str3.isEmpty()) {
    stLength = (str3.length() >= SSD1306_MAXSTR ? SSD1306_MAXSTR : str3.length());
    memcpy(&m_szStr3, str3.c_str(), stLength);
    m_szStr3[stLength] = '\0';
  } else {
    m_szStr3[0] = '\0';
  }
  m_iTextAlign = TEXT_ALIGN_CENTER;
  m_bTextAvailable = true;
}

bool global::oledProcessQueue() {
  bool bReturn = false;
  if (m_bTextAvailable) {
    _DBGPRINTF(PSTR("global: oledProcessQueue: Text available...\n"));
    if (strlen(m_szStr2) > 0 && strlen(m_szStr3) > 0) {
      global::oledDrawStringCenter(m_szStr1, m_szStr2, m_szStr3);
    } else if (strlen(m_szStr2) > 0) {
      global::oledDrawStringCenter(m_szStr1, m_szStr2);
    } else if (strlen(m_szStr1) > 0) {
      global::oledDrawStringCenter(m_szStr1);
    } else {
      _DBGPRINTF(PSTR("global: oledProcessQueue: INVALID STRINGS\n"));
    }
    m_bTextAvailable = false;
    bReturn = true;
  }
  return bReturn;
}

void global::oledDrawStringCenter(String str) {
  if (m_pOled != NULL && xPortGetCoreID() == ARDUINO_RUNNING_CORE) {
    m_pOled->setFont(ArialMT_Plain_10);
    m_pOled->setTextAlignment(TEXT_ALIGN_CENTER);
    m_pOled->clear();
    m_pOled->drawString(m_iScreenWidth / 2, 0, (str.length() > 20 ? str.substring(1, 20) : str));
    m_pOled->display();
  } else {
    _DBGPRINTF(PSTR("global: oledDrawStringCenter: WRONG CORE=%d\n"), xPortGetCoreID());
  }
}

void global::oledDrawStringCenter(String str, String str2) {
  if (m_pOled != NULL && xPortGetCoreID() == ARDUINO_RUNNING_CORE) {
    m_pOled->setFont(ArialMT_Plain_10);
    m_pOled->setTextAlignment(TEXT_ALIGN_CENTER);
    m_pOled->clear();
    m_pOled->drawString(m_iScreenWidth / 2, 0, (str.length() > 20 ? str.substring(1, 20) : str));
    m_pOled->drawString(m_iScreenWidth / 2, 10, (str2.length() > 20 ? str2.substring(1, 20) : str2));
    m_pOled->display();
  } else {
    _DBGPRINTF(PSTR("global: oledDrawStringCenter: WRONG CORE=%d\n"), xPortGetCoreID());
  }
}

void global::oledDrawStringCenter(String str, String str2, String str3) {
  if (m_pOled != NULL && xPortGetCoreID() == ARDUINO_RUNNING_CORE) {
    m_pOled->setFont(ArialMT_Plain_10);
    m_pOled->setTextAlignment(TEXT_ALIGN_CENTER);
    m_pOled->clear();
    m_pOled->drawString(m_iScreenWidth / 2, 0, (str.length() > 20 ? str.substring(1, 20) : str));
    m_pOled->drawString(m_iScreenWidth / 2, 10, (str2.length() > 20 ? str2.substring(1, 20) : str2));
    m_pOled->drawString(m_iScreenWidth / 2, 20, (str3.length() > 20 ? str3.substring(1, 20) : str3));
    m_pOled->display();
  } else {
    _DBGPRINTF(PSTR("global: oledDrawStringCenter: WRONG CORE=%d\n"), xPortGetCoreID());
  }
}

void global::oledDrawStringLeft(String str) {
  if (m_pOled != NULL && xPortGetCoreID() == ARDUINO_RUNNING_CORE) {
    m_pOled->setFont(ArialMT_Plain_10);
    m_pOled->setTextAlignment(TEXT_ALIGN_LEFT);
    m_pOled->clear();
    m_pOled->drawString(0, 0, (str.length() > 22 ? str.substring(1, 20) : str));
    m_pOled->display();
  } else {
    _DBGPRINTF(PSTR("global: oledDrawStringCenter: WRONG CORE=%d\n"), xPortGetCoreID());
  }
}

void global::oledDrawStringRight(String str) {
  if (m_pOled != NULL && xPortGetCoreID() == ARDUINO_RUNNING_CORE) {
    m_pOled->setFont(ArialMT_Plain_10);
    m_pOled->setTextAlignment(TEXT_ALIGN_RIGHT);
    m_pOled->clear();
    m_pOled->drawString(128, 0, (str.length() > 22 ? str.substring(1, 20) : str));
    m_pOled->display();
  } else {
    _DBGPRINTF(PSTR("global: oledDrawStringCenter: WRONG CORE=%d\n"), xPortGetCoreID());
  }
}

#endif /* ENABLE_SSD1306 */
