#ifndef debughelper_h
#define debughelper_h

#include "globalincludes.h"
#include <stdarg.h>

/* DebugHelper: debug/serial/syslog output, reboot, remote LED mode.
 * Call sites keep using global::* which delegate here. */

class DebugHelper {
public:
  static void dbgprintf_P(PGM_P pMsg, va_list va);
  static void dbgprintf(const char* pMsg, va_list va);
  static void errprintf(PGM_P pMsg, va_list va);
  static void errprintf_P(const char* pMsg, va_list va);
  static void printfserial(const char* fmt, va_list args);
  static void flushserial();
  static void heap_caps_alloc_failed_hook(size_t requested_size, uint32_t caps, const char* function_name);
  static int dbgprintf_esp(const char* fmt, va_list args);
  static void SetDebugSyslogServer(char* pszNew);
  static void RebootController();
  static void SetRemoteLEDMode(char* pszRemote, byte bLedMode);
};

#endif
