#include "globalincludes.h"
#include "global.h"
#include "debughelper.h"
#include <stdarg.h>
#include <HTTPClient.h>
#include "syslog.h"

#include "syslog.h"
#if defined(_DEBUG) && defined(_DEBUG_UDP)
#include "syslog.h"
#endif

static HTTPClient s_http;

static void errprintf_P_v(const char* fmt, ...) {
  va_list va;
  va_start(va, fmt);
  DebugHelper::errprintf_P(fmt, va);
  va_end(va);
}

void DebugHelper::dbgprintf_P(PGM_P pMsg, va_list va) {
#ifdef _DEBUG
  if (global::m_bSerialEnabled) {
    va_list vaSerial;
    va_copy(vaSerial, va);
    DebugHelper::printfserial((char*)pMsg, vaSerial);
    va_end(vaSerial);
  }
#if defined(_DEBUG_UDP)
  if (global::GetNetworkStarted() &&
      global::m_bSyslogEnabled &&
      global::m_pSyslog != NULL)
  {
    va_list vaSyslog;
    va_copy(vaSyslog, va);
    global::m_pSyslog->vlogf((uint16_t)LOG_DEBUG, (char*)pMsg, vaSyslog);
    va_end(vaSyslog);
  }
#endif
#endif
}

void DebugHelper::dbgprintf(const char* pMsg, va_list va) {
#ifdef _DEBUG
  if (global::m_bSerialEnabled) {
    va_list vaSerial;
    va_copy(vaSerial, va);
    DebugHelper::printfserial((char*)pMsg, vaSerial);
    va_end(vaSerial);
  }
#if defined(_DEBUG_UDP)
  if (global::GetNetworkStarted() &&
      global::m_bSyslogEnabled &&
      global::m_pSyslog != NULL)
  {
    va_list vaSyslog;
    va_copy(vaSyslog, va);
    global::m_pSyslog->vlogf((uint16_t)LOG_DEBUG, (char*)pMsg, vaSyslog);
    va_end(vaSyslog);
  }
#endif
#endif
}

void DebugHelper::errprintf(PGM_P pMsg, va_list va) {
#ifdef _DEBUG
  if (global::m_bSerialEnabled)
  {
    va_list vaSerial;
    va_copy(vaSerial, va);
    DebugHelper::printfserial((char*)pMsg, vaSerial);
    va_end(vaSerial);
  }
#if defined(_DEBUG_UDP)
  if (global::GetNetworkStarted() &&
      global::m_bSyslogEnabled &&
      global::m_pSyslog != NULL)
  {
    va_list vaSyslog;
    va_copy(vaSyslog, va);
    global::m_pSyslog->vlogf((uint16_t)LOG_ERR, (char*)pMsg, vaSyslog);
    va_end(vaSyslog);
  }
#endif
#endif
}

void DebugHelper::errprintf_P(const char* pMsg, va_list va) {
#ifdef _DEBUG
  if (global::m_bSerialEnabled)
  {
    va_list vaSerial;
    va_copy(vaSerial, va);
    DebugHelper::printfserial((char*)pMsg, vaSerial);
    va_end(vaSerial);
  }
#if defined(_DEBUG_UDP)
  if (global::GetNetworkStarted() &&
      global::m_bSyslogEnabled &&
      global::m_pSyslog != NULL)
  {
    va_list vaSyslog;
    va_copy(vaSyslog, va);
    global::m_pSyslog->vlogf((uint16_t)LOG_ERR, (char*)pMsg, vaSyslog);
    va_end(vaSyslog);
  }
#endif
#endif
}

void DebugHelper::RebootController() {
  _DBGPRINTF(PSTR("global::RebootController: Performing REBOOT now\n"));
  global::SetSerialEnabled(false);
  global::TogglePwrLedMsg();
  if (global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingInt("syslastmillis", millis());
  esp_restart();
  while (true) {}
}

void DebugHelper::SetRemoteLEDMode(char* pszRemote, byte bLedMode) {
  char szQuryUrl[AMBIBACK_MAX_URLSIZE];
  snprintf_P(szQuryUrl, AMBIBACK_MAX_URLSIZE, PSTR("http://%s/setcolormode?mode=%d"), pszRemote, bLedMode);
  s_http.setConnectTimeout(1000);
  s_http.setReuse(false);
  s_http.begin(szQuryUrl);
  int iHttpCode = s_http.GET();
  if (iHttpCode == HTTP_CODE_OK) {
    _DBGPRINTF(PSTR("global:TurnOffRemoteLEDMode: Change Mode to %s: Http request to %s, HTTP code: %d\n"), global::m_pLedHelper->GetLedModeStr(bLedMode), szQuryUrl, iHttpCode);
    String payload = s_http.getString();
    _DBGPRINTF_ULTRA(PSTR("global:TurnOffRemoteLEDMode: Http response payload: %.*s\n"), (payload.length() < 924 ? (int)payload.length() : 924), payload.c_str());
    if (payload.length() > 0) {
      _DBGPRINTF(PSTR("global:TurnOffRemoteLEDMode: SUCCESSFULLY changed mode to %s on connected device %s\n"), global::m_pLedHelper->GetLedModeStr(bLedMode), global::GetClientConnectedIpStr());
    } else {
      errprintf_P_v(PSTR("global:TurnOffRemoteLEDMode: LED TURN OFF: Failed to change mode to %s on connected device %s - empty response!\n"), global::m_pLedHelper->GetLedModeStr(bLedMode), global::GetClientConnectedIpStr());
    }
  } else {
    errprintf_P_v(PSTR("global:TurnOffRemoteLEDMode: LED TURN OFF: Failed to change mode to %s on connected device %s - with httpcode %d!\n"), global::m_pLedHelper->GetLedModeStr(bLedMode), global::GetClientConnectedIpStr(), iHttpCode);
  }
}

void DebugHelper::heap_caps_alloc_failed_hook(size_t requested_size, uint32_t caps, const char* function_name) {
  _DBGPRINTF(PSTR("heap_caps_alloc_failed_hook: %s was called but failed to allocate %d bytes with 0x%X capabilities. \n"),
             function_name, (int)requested_size, (unsigned)caps);
}

int DebugHelper::dbgprintf_esp(const char* fmt, va_list args) {
  char message_stack[512];
  char* message = message_stack;
  size_t initialLen = strlen(fmt);
  size_t len;
  bool bHeapAllocated = false;
  if (initialLen >= 512) {
    message = new (std::nothrow) char[initialLen + 1];
    if (!message) return 1;
    bHeapAllocated = true;
  }
  va_list argsMeasure;
  va_copy(argsMeasure, args);
  len = vsnprintf(message, (bHeapAllocated ? initialLen + 1 : sizeof(message_stack)), fmt, argsMeasure);
  va_end(argsMeasure);
  if (len > (bHeapAllocated ? initialLen : sizeof(message_stack) - 1)) {
    if (bHeapAllocated) delete[] message;
    message = new (std::nothrow) char[len + 1];
    if (!message) return 1;
    bHeapAllocated = true;
    va_list argsRender;
    va_copy(argsRender, args);
    len = vsnprintf(message, len + 1, fmt, argsRender);
    va_end(argsRender);
  }
  if (len > 1 && Serial.baudRate() > 0) {
    const char* taskname = pcTaskGetTaskName(nullptr);
    size_t lenTaskName = strlen(taskname);
    Serial.printf(PSTR("%%debug: %d:%d\t%s%sesp32debug: %s%s"),
      xPortGetCoreID(),
      millis(),
      taskname,
      (lenTaskName > 8 ? "\t" : "\t\t"),
      message,
      (*(message + len - 1) == '\n' ? "" : "\n"));
  }
  if (bHeapAllocated) delete[] message;
  return 1;
}

void DebugHelper::printfserial(const char* fmt, va_list args) {
  char message_stack[512];
  char* message = message_stack;
  size_t initialLen;
  size_t len;
  size_t lenTaskName;
  bool bHeapAllocated = false;
  if (Serial.baudRate() > 0) {
    initialLen = strlen(fmt);
    if (initialLen >= 512) {
      message = new (std::nothrow) char[initialLen + 1];
      if (!message) return;
      bHeapAllocated = true;
    }
    const char* taskname = pcTaskGetTaskName(nullptr);
    lenTaskName = strlen(taskname);
    va_list argsMeasure;
    va_copy(argsMeasure, args);
    len = vsnprintf_P(message, (bHeapAllocated ? initialLen + 1 : sizeof(message_stack)), fmt, argsMeasure);
    va_end(argsMeasure);
    if (len > (bHeapAllocated ? initialLen : sizeof(message_stack) - 1)) {
      if (bHeapAllocated) delete[] message;
      message = new (std::nothrow) char[len + 1];
      if (!message) return;
      bHeapAllocated = true;
      va_list argsRender;
      va_copy(argsRender, args);
      vsnprintf_P(message, len + 1, fmt, argsRender);
      va_end(argsRender);
    }
    Serial.printf(PSTR("%%debug: %d:%d\t%s%s%s"), xPortGetCoreID(), millis(), pcTaskGetTaskName(nullptr), (lenTaskName > 8 ? "\t" : "\t\t"), message);
    if (bHeapAllocated) delete[] message;
  }
}

void DebugHelper::flushserial() {
  Serial.flush();
}

void DebugHelper::SetDebugSyslogServer(char* pszNew) {
  size_t strLen = strlen(pszNew);
  if (strLen > (STR_IPNAME_MAX - 1))
    strLen = STR_IPNAME_MAX - 1;
  memcpy(global::m_DebugSyslogServer, pszNew, strLen);
  global::m_DebugSyslogServer[strLen] = '\0';
  _DBGPRINTF(PSTR("global:SetDebugSyslogServer=%s/%s(%d)\n"), pszNew, global::m_DebugSyslogServer, (int)strLen);
}
