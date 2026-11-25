#include "globalincludes.h"
#if defined(ENABLE_IMPROV_SERIAL)
# include <WiFi.h>
# include "improvserial.h"
# include "eepromhelper.h"

// Packet layout: "IMPROV" | version | type | length | data[length] | checksum
// checksum = sum of all previous bytes (mod 256). A '\n' follows sent packets.
static const char g_szImprovHeader[] = "IMPROV";
# define IMPROV_HEADER_LEN 6

uint8_t ImprovSerial::m_bBuf[IMPROV_MAX_PACKET];
size_t ImprovSerial::m_stPos = 0;

void ImprovSerial::Poll() {
  while (Serial.available() > 0) {
    int iByte = Serial.read();
    if (iByte < 0) break;
    FeedByte((uint8_t) iByte);
  }
}

void ImprovSerial::FeedByte(uint8_t bByte) {
  // Match the header byte by byte, restart on mismatch
  if (m_stPos < IMPROV_HEADER_LEN) {
    if (bByte == (uint8_t) g_szImprovHeader[m_stPos]) {
      m_bBuf[m_stPos++] = bByte;
    } else {
      m_stPos = (bByte == (uint8_t) g_szImprovHeader[0]) ? 1 : 0;
      if (m_stPos == 1) m_bBuf[0] = bByte;
    }
    return;
  }

  if (m_stPos >= IMPROV_MAX_PACKET) {
    m_stPos = 0;
    return;
  }
  m_bBuf[m_stPos++] = bByte;

  // version, type, length present?
  if (m_stPos < IMPROV_HEADER_LEN + 3) return;
  if (m_bBuf[IMPROV_HEADER_LEN] != IMPROV_SERIAL_VERSION) {
    m_stPos = 0;
    return;
  }
  size_t stDataLen = m_bBuf[IMPROV_HEADER_LEN + 2];
  size_t stTotal = IMPROV_HEADER_LEN + 3 + stDataLen + 1;
  if (stTotal > IMPROV_MAX_PACKET) {
    m_stPos = 0;
    return;
  }
  if (m_stPos < stTotal) return;

  // Complete packet: verify checksum
  uint8_t bSum = 0;
  for (size_t st = 0; st < stTotal - 1; st++) bSum += m_bBuf[st];
  uint8_t bType = m_bBuf[IMPROV_HEADER_LEN + 1];
  m_stPos = 0;
  if (bSum != m_bBuf[stTotal - 1]) {
    _DBGPRINTF(PSTR("ImprovSerial: checksum mismatch, type=0x%02X\n"), bType);
    SendError(IMPROV_ERROR_INVALID_RPC);
    return;
  }
  HandlePacket(bType, &m_bBuf[IMPROV_HEADER_LEN + 3], stDataLen);
}

void ImprovSerial::HandlePacket(uint8_t bType, const uint8_t* pbData, size_t stLen) {
  if (bType == IMPROV_TYPE_RPC) {
    HandleRpc(pbData, stLen);
  }
  // Other packet types are device -> client only
}

void ImprovSerial::HandleRpc(const uint8_t* pbData, size_t stLen) {
  if (stLen < 2 || (size_t) pbData[1] + 2 > stLen) {
    SendError(IMPROV_ERROR_INVALID_RPC);
    return;
  }
  uint8_t bCommand = pbData[0];
  const uint8_t* pbCmdData = &pbData[2];
  size_t stCmdLen = pbData[1];
  _DBGPRINTF(PSTR("ImprovSerial: RPC command 0x%02X len=%d\n"), bCommand, (int) stCmdLen);

  switch (bCommand) {
    case IMPROV_CMD_WIFI_SETTINGS:
      HandleWifiSettings(pbCmdData, stCmdLen);
      break;
    case IMPROV_CMD_GET_STATE: {
      uint8_t bState = GetCurrentState();
      SendState(bState);
      if (bState == IMPROV_STATE_PROVISIONED) {
        char szUrl[48];
        GetDeviceUrl(szUrl, sizeof(szUrl));
        const char* aStrings[] = { szUrl };
        SendRpcResult(IMPROV_CMD_GET_STATE, aStrings, 1);
      }
      break;
    }
    case IMPROV_CMD_GET_INFO: {
      const char* aStrings[] = { "AmbiBack", AMBIBACK_VERSION, "ESP32", global::GetDeviceName() };
      SendRpcResult(IMPROV_CMD_GET_INFO, aStrings, 4);
      break;
    }
    case IMPROV_CMD_GET_NETWORKS: {
      int iCount = WiFi.scanNetworks();
      for (int i = 0; i < iCount; i++) {
        char szRssi[8];
        snprintf(szRssi, sizeof(szRssi), "%d", (int) WiFi.RSSI(i));
        String strSsid = WiFi.SSID(i);
        if (strSsid.length() == 0) continue;
        const char* aStrings[] = { strSsid.c_str(), szRssi, (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "NO" : "YES") };
        SendRpcResult(IMPROV_CMD_GET_NETWORKS, aStrings, 3);
        _yield();
      }
      WiFi.scanDelete();
      // Empty result marks the end of the list
      SendRpcResult(IMPROV_CMD_GET_NETWORKS, NULL, 0);
      break;
    }
    default:
      SendError(IMPROV_ERROR_UNKNOWN_RPC);
      break;
  }
}

void ImprovSerial::HandleWifiSettings(const uint8_t* pbData, size_t stLen) {
  // data: ssid_len, ssid, pass_len, pass
  if (stLen < 1) { SendError(IMPROV_ERROR_INVALID_RPC); return; }
  size_t stSsidLen = pbData[0];
  if (stSsidLen == 0 || stSsidLen >= WLAN_SSID_MAX || 1 + stSsidLen + 1 > stLen) { SendError(IMPROV_ERROR_INVALID_RPC); return; }
  size_t stKeyLen = pbData[1 + stSsidLen];
  if (stKeyLen >= WLAN_KEY_MAX || 2 + stSsidLen + stKeyLen > stLen) { SendError(IMPROV_ERROR_INVALID_RPC); return; }

  char szSSID[WLAN_SSID_MAX];
  char szKEY[WLAN_KEY_MAX];
  memcpy(szSSID, &pbData[1], stSsidLen);
  szSSID[stSsidLen] = '\0';
  memcpy(szKEY, &pbData[2 + stSsidLen], stKeyLen);
  szKEY[stKeyLen] = '\0';

  _DBGPRINTF(PSTR("ImprovSerial: Wi-Fi settings for SSID '%s'\n"), szSSID);
  SendState(IMPROV_STATE_PROVISIONING);

  // Same path as the "%WIFI:" serial command (SerialHelper::commandSetWifi)
  if (global::ConnectWifiConfigured(szSSID, szKEY, false)) {
    global::SetWifiSSID(szSSID);
    global::SetWifiPassword(szKEY);
    global::m_pEepromHelper->CommitChanges();
    _yield();

    SendState(IMPROV_STATE_PROVISIONED);
    char szUrl[48];
    GetDeviceUrl(szUrl, sizeof(szUrl));
    const char* aStrings[] = { szUrl };
    SendRpcResult(IMPROV_CMD_WIFI_SETTINGS, aStrings, 1);
    Serial.flush();
    _delay(500);

    // Reboot like the "%WIFI:" command so all services start with the new network
    global::RebootController();
    _delay(1000);
  } else {
    global::errprintf_P(PSTR("ImprovSerial: failed to connect to SSID '%s'\n"), szSSID);
    SendError(IMPROV_ERROR_UNABLE_CONNECT);
    SendState(IMPROV_STATE_AUTHORIZED);
  }
}

uint8_t ImprovSerial::GetCurrentState() {
  if (WiFi.status() == WL_CONNECTED && (WiFi.getMode() & WIFI_STA)) return IMPROV_STATE_PROVISIONED;
  return IMPROV_STATE_AUTHORIZED;
}

void ImprovSerial::GetDeviceUrl(char* pszUrl, size_t stLen) {
  snprintf(pszUrl, stLen, "http://%s/", WiFi.localIP().toString().c_str());
}

void ImprovSerial::SendState(uint8_t bState) {
  SendPacket(IMPROV_TYPE_CURRENT_STATE, &bState, 1);
}

void ImprovSerial::SendError(uint8_t bError) {
  SendPacket(IMPROV_TYPE_ERROR_STATE, &bError, 1);
}

void ImprovSerial::SendRpcResult(uint8_t bCommand, const char** ppszStrings, size_t stCount) {
  uint8_t bData[IMPROV_MAX_PACKET];
  size_t stPos = 2;
  for (size_t i = 0; i < stCount; i++) {
    size_t stStrLen = strlen(ppszStrings[i]);
    if (stStrLen > 255) stStrLen = 255;
    if (stPos + 1 + stStrLen > 240) break; // keep the packet within one length byte
    bData[stPos++] = (uint8_t) stStrLen;
    memcpy(&bData[stPos], ppszStrings[i], stStrLen);
    stPos += stStrLen;
  }
  bData[0] = bCommand;
  bData[1] = (uint8_t) (stPos - 2);
  SendPacket(IMPROV_TYPE_RPC_RESULT, bData, stPos);
}

void ImprovSerial::SendPacket(uint8_t bType, const uint8_t* pbData, size_t stLen) {
  uint8_t bPacket[IMPROV_MAX_PACKET + 12];
  size_t stPos = 0;
  memcpy(bPacket, g_szImprovHeader, IMPROV_HEADER_LEN);
  stPos = IMPROV_HEADER_LEN;
  bPacket[stPos++] = IMPROV_SERIAL_VERSION;
  bPacket[stPos++] = bType;
  bPacket[stPos++] = (uint8_t) stLen;
  memcpy(&bPacket[stPos], pbData, stLen);
  stPos += stLen;
  uint8_t bSum = 0;
  for (size_t st = 0; st < stPos; st++) bSum += bPacket[st];
  bPacket[stPos++] = bSum;
  bPacket[stPos++] = '\n';

  // Keep debug output from splitting the packet
  bool bSerialEnabled = global::GetSerialEnabled();
  global::SetSerialEnabled(false);
  Serial.write(bPacket, stPos);
  global::SetSerialEnabled(bSerialEnabled);
}

#endif // ENABLE_IMPROV_SERIAL
