#include <stdio.h>
#include <stdarg.h>
#include <Arduino.h>

#include "syslog.h"

// Public Methods //////////////////////////////////////////////////////////////

Syslog::Syslog(uint8_t protocol) {
  this->InitClass(); 
  this->_protocol = protocol;
  this->_server = NULL;
  this->_port = 0;
  this->_deviceHostname = SYSLOG_NILVALUE;
  this->_appName = SYSLOG_NILVALUE;
  this->_priDefault = LOG_KERN;
}

Syslog::Syslog(const char* server, uint16_t port, const char* deviceHostname, const char* appName, uint16_t priDefault, uint8_t protocol) {
  this->InitClass(); 
  this->_protocol = protocol;
  this->_server = server;
  this->_ip.fromString(server); 
  this->_port = port;
  this->_deviceHostname = (deviceHostname == NULL) ? SYSLOG_NILVALUE : deviceHostname;
  this->_appName = (appName == NULL) ? SYSLOG_NILVALUE : appName;
  this->_priDefault = priDefault;
}

Syslog::Syslog(IPAddress ip, uint16_t port, const char* deviceHostname, const char* appName, uint16_t priDefault, uint8_t protocol) {
  this->InitClass(); 
  this->_protocol = protocol;
  this->_ip = ip;
  this->_server = NULL;
  this->_port = port;
  this->_deviceHostname = (deviceHostname == NULL) ? SYSLOG_NILVALUE : deviceHostname;
  this->_appName = (appName == NULL) ? SYSLOG_NILVALUE : appName;
  this->_priDefault = priDefault;
}

void Syslog::InitClass(void) {
  this->_ip = INADDR_NONE; 
  m_pAsyncUdp = new AsyncUDP();
}

Syslog &Syslog::server(const char* server, uint16_t port) {
  this->_server = server;
  this->_port = port;
  return *this;
}

Syslog &Syslog::server(IPAddress ip, uint16_t port) {
  this->_ip = ip;
  this->_server = NULL;
  this->_port = port;
  return *this;
}

Syslog &Syslog::deviceHostname(const char* deviceHostname) {  
  this->_deviceHostname = (deviceHostname == NULL) ? SYSLOG_NILVALUE : deviceHostname;
  return *this;
}

Syslog &Syslog::appName(const char* appName) {
  this->_appName = (appName == NULL) ? SYSLOG_NILVALUE : appName;
  return *this;
}

Syslog &Syslog::defaultPriority(uint16_t pri) {
  this->_priDefault = pri;
  return *this;
}

Syslog &Syslog::logMask(uint8_t priMask) {
  this->_priMask = priMask;
  return *this;
}


bool Syslog::log(uint16_t pri, const __FlashStringHelper *message) {
  return this->_sendLog(pri, message);
}

bool Syslog::log(uint16_t pri, const String &message) {
  return this->_sendLog(pri, message.c_str());
}

bool Syslog::log(uint16_t pri, const char *message) {
  return this->_sendLog(pri, message);
}

bool Syslog::vlogf(uint16_t pri, const char *fmt, va_list args) {
  int result;
  char* header;
  char* message;
  size_t initialLen;
  size_t messageLen;

  if (!WiFi.isConnected() || 
      (this->_server == NULL && uint32_t(this->_ip) == INADDR_NONE) || 
      this->_port == 0)
    return false;

  // Check priority against priMask values.
  if ((LOG_MASK(LOG_PRI(pri)) & this->_priMask) == 0)
    return true;

  // Set default facility if none specified.
  if ((pri & LOG_FACMASK) == 0)
    pri = LOG_MAKEPRI(LOG_FAC(this->_priDefault), pri);
  // IETF Doc: https://tools.ietf.org/html/rfc5424
  // BSD Doc: https://tools.ietf.org/html/rfc3164

  // Create memory for message
//  initialLen = strlen(fmt);
//  initialLen = (initialLen < SYSLOG_MIN_MESSAGE ? SYSLOG_MIN_MESSAGE : initialLen); 
  initialLen = SYSLOG_MIN_MESSAGE + SYSLOG_MIN_HEADER;
  message = new char[initialLen];
  header = new char[SYSLOG_MIN_HEADER]; 

  // Print header %d:%d\t%s\t%s"), xPortGetCoreID(), millis(), pcTaskGetTaskName(nullptr), message);
  size_t stHeadLen = snprintf_P(header, SYSLOG_MIN_HEADER, PSTR("<%d>%s%s %s%s Core:%d Time:%d ms Task:%s "), 
                                  pri,
                                  (this->_protocol == SYSLOG_PROTO_IETF ? "1 - " : ""),
                                  this->_deviceHostname,
                                  this->_appName,
                                  (this->_protocol == SYSLOG_PROTO_IETF ? " - - - \xEF\xBB\xBF": "[0]:"),
                                  xPortGetCoreID(),
                                  millis(),
                                  pcTaskGetTaskName(nullptr)
                                ); 

  memcpy (message, header, stHeadLen);
  messageLen = vsnprintf(message+stHeadLen, initialLen - stHeadLen, fmt, args);
  if ((messageLen+stHeadLen) > initialLen) {
    // Create new buffer
    delete[] message;
    messageLen = (messageLen < SYSLOG_MAX_MESSAGE ? (messageLen + 1) : SYSLOG_MAX_MESSAGE); // Limit to SYSLOG_MAX_MESSAGE
    message = new char[messageLen + stHeadLen];

    memcpy (message, header, stHeadLen);
    vsnprintf(message+stHeadLen, messageLen - stHeadLen, fmt, args);
    // Set NULL Byte to last CHAR in the str
//    if (messageLen + stHeadLen == SYSLOG_MAX_MESSAGE) 
    *(message+messageLen+stHeadLen-1) = '\0'; 
  }
// Serial.printf(PSTR("Syslog: %d/%d/%d: %s "), initialLen, stHeadLen, messageLen, message);

//  result = this->_sendLog(pri, message);

  // Send message to UDP Server 
  result = m_pAsyncUdp->writeTo((const uint8_t*) message, messageLen + stHeadLen, this->_ip, this->_port);

  delete[] message;
  delete[] header;

  if (result == 0)
    return false;
  // success
  return true;
}

bool Syslog::logf(uint16_t pri, const char *fmt, ...) {
  va_list args;
  bool result;

  va_start(args, fmt);
  result = this->vlogf(pri, fmt, args);
  va_end(args);
  return result;
}

bool Syslog::logf(const char *fmt, ...) {
  va_list args;
  bool result;

  va_start(args, fmt);
  result = this->vlogf(this->_priDefault, fmt, args);
  va_end(args);
  return result;
}

bool Syslog::log(const __FlashStringHelper *message) {
  return this->_sendLog(this->_priDefault, message);
}

bool Syslog::log(const String &message) {
  return this->_sendLog(this->_priDefault, message.c_str());
}

bool Syslog::log(const char *message) {
  return this->_sendLog(this->_priDefault, message);
}

// Private Methods /////////////////////////////////////////////////////////////
bool Syslog::_sendLog(uint16_t pri, const char *message, size_t len) {
  int result;

  // Messages are sent to _ip, so without a valid address there is no target.
  if (uint32_t(this->_ip) == 0)
    return false;

  if ((this->_server == NULL && uint32_t(this->_ip) == INADDR_NONE) || this->_port == 0)
    return false;

  // Check priority against priMask values.
  if ((LOG_MASK(LOG_PRI(pri)) & this->_priMask) == 0)
    return true;

  // Set default facility if none specified.
  if ((pri & LOG_FACMASK) == 0)
    pri = LOG_MAKEPRI(LOG_FAC(this->_priDefault), pri);

  // IETF Doc: https://tools.ietf.org/html/rfc5424
  // BSD Doc: https://tools.ietf.org/html/rfc3164

  /* Max 1KB messages */
  char* pszSyslogMessage;
  pszSyslogMessage = new char[len + 128];
  // SYSLOG_MAX_SEND_MESSAGE
//  char szSyslogMessage[SYSLOG_MAX_SEND_MESSAGE]; 
  size_t stLen = snprintf_P(pszSyslogMessage, len + 128 , PSTR("<%d>%s%s %s%s Core:%d Time:%d ms %s"), 
                              pri, 
                              (this->_protocol == SYSLOG_PROTO_IETF ? "1 - " : ""), 
                              this->_deviceHostname, 
                              this->_appName, 
                              (this->_protocol == SYSLOG_PROTO_IETF ? " - - - \xEF\xBB\xBF": "[0]:"), 
                              xPortGetCoreID(), 
                              millis(), 
                              message); 
// Serial.printf(PSTR("Syslog: %d: %s "), stLen, pszSyslogMessage);
  
  // Send message to UDP Server 
  result = m_pAsyncUdp->writeTo((const uint8_t*) pszSyslogMessage, stLen, this->_ip, this->_port);

  delete[] pszSyslogMessage;

  if (result == 0)
    return false;
  // success
  return true;
}

bool Syslog::_sendLog(uint16_t pri, const __FlashStringHelper *message) {
  return _sendLog(pri, (const char*) message); 
}
