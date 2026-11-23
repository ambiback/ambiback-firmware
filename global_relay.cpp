#include "globalincludes.h"
#include "global.h"
#include <HTTPClient.h>
#include <cJSON.h>
#include "udpserver.h"

#if defined(ENABLE_RELAY)
static HTTPClient s_relayHttp;

byte global::GetAmbibackDeviceCount() {
  return global::m_ambiback_devices_count;
}
const char* global::GetAmbibackDeviceName(byte bIndex) {
  if (bIndex <= global::m_ambiback_devices_count)
    return (const char*) &m_ambiback_devices_name[bIndex][0];
  else
    return PSTR("");
}
const char* global::GetAmbibackDeviceID(byte bIndex) {
  if (bIndex <= global::m_ambiback_devices_count)
    return (const char*) &global::m_ambiback_devices_id[bIndex][0];
  else
    return PSTR("");
}
const char* global::GetAmbibackDeviceMac(byte bIndex) {
  if (bIndex <= global::m_ambiback_devices_count)
    return (const char*) &global::m_ambiback_devices_mac[bIndex][0];
  else
    return PSTR("");
}
const char* global::GetAmbibackDeviceIP(byte bIndex) {
  if (bIndex <= global::m_ambiback_devices_count)
    return (const char*) &global::m_ambiback_devices_ip[bIndex][0];
  else
    return PSTR("");
}
void global::SetAmbibackDeviceCount(byte bNew) {
  global::m_ambiback_devices_count = bNew;
}
void global::SetAmbibackDeviceName(byte bIndex, char* psz) {
  // Copy DeviceName over to Array
  snprintf_P(global::m_ambiback_devices_name[bIndex], DEVICE_NAME_MAX, PSTR("%s"), psz);
}
void global::SetAmbibackDeviceID(byte bIndex, char* psz) {
  // Copy DeviceID over to Array
  snprintf_P(global::m_ambiback_devices_id[bIndex], AMBIBACK_MAX_IDLEN, PSTR("%s"), psz);
}
void global::SetAmbibackDeviceMac(byte bIndex, char* psz) {
  // Copy DeviceMAC over to Array
  snprintf_P(global::m_ambiback_devices_mac[bIndex], AMBIBACK_MAX_IDLEN, PSTR("%s"), psz);
}
void global::SetAmbibackDeviceIP(byte bIndex, char* psz) {
  // Copy DeviceIP over to Array
  snprintf_P(global::m_ambiback_devices_ip[bIndex], AMBIBACK_MAX_IDLEN, PSTR("%s"), psz);
}
void global::SetAmbibackRelayModeDirect(byte bNew, bool bEpprom) {
  global::m_ambiback_relaymode_direct = bNew;
  _DBGPRINTF(PSTR("global:SetAmbibackRelayModeDirect Value=%d\n"), bNew);
  // Write to EEPROM!
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("AmbiBackRelDir", global::m_ambiback_relaymode_direct);
}
void global::SetAmbibackRelayMode(byte bNew, bool bEpprom) {
  global::m_ambiback_relay_mode = bNew;
  _DBGPRINTF(PSTR("global:SetAmbibackRelayMode Value=%d\n"), bNew);
  // Write to EEPROM!
  if (bEpprom) {
    global::m_pEepromHelper->SetSettingByte("AmbiBackRelMde", global::m_ambiback_relay_mode);

#if defined(CONFIG_BT_ENABLED)
# if defined(DISABLE_BLUETOOTH)
    // Reboot if Bluetooth is enabled!
    if (global::m_ambiback_relay_mode == RELAY_BT) {
      global::SetShouldReboot(true); 
    }
# endif
#endif
    
  }
}
void global::SetAmbibackWifiDirectMode(byte bNew, bool bEpprom) {
  global::m_ambiback_relay_wifidirect_enabled = bNew;
  _DBGPRINTF(PSTR("global:SetAmbibackWifiDirectMode Value=%d\n"), bNew);
  // Write to EEPROM!
  if (bEpprom) {
    global::m_pEepromHelper->SetSettingByte("AmbiBackWifiDi", global::m_ambiback_relay_wifidirect_enabled);
  }

  // HANDLE WIFI SWITCH!
  if (global::m_ambiback_relay_wifidirect_enabled == 0 && 
      global::GetWifiDirectConnected() && 
      strcmp(WiFi.SSID().c_str(), global::GetWifiSSID()) != 0) {
    _DBGPRINTF(PSTR("global: SetAmbibackWifiDirectMode: DISABLED: WIFI Direct Connected, restore original WLAN connection to %s\n"), global::GetWifiSSID());
    global::ConnectWifiConfigured(global::GetWifiSSID(), global::GetWifiPassword(), false); 
    global::SetWifiDirectConnected(false);
  } else if (  global::m_ambiback_relay_wifidirect_enabled == 1 ) {
    _DBGPRINTF(PSTR("global: SetAmbibackWifiDirectMode: ENABLED: Recheck AmbibackRelaymodeInitialized\n"));
    global::SetAmbibackRelaymodeInitialized(false);
    
  }
}

void global::SetAmbibackSelectedDevice(char* pszNew, bool bEpprom) {
  size_t strLen = strlen(pszNew);
  if ( strLen > (AMBIBACK_MAX_IDLEN - 1))
    strLen = (AMBIBACK_MAX_IDLEN - 1); // Limit to max size
  memcpy(global::m_ambiback_selected_device, pszNew, strLen);
  global::m_ambiback_selected_device[strLen] = '\0';
  _DBGPRINTF(PSTR("global:SetAmbibackSelectedDevice=%s/%s(%d)\n"), pszNew, global::m_ambiback_selected_device, strLen);

  // Store into EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingString("AmbiBackSelDev", global::m_ambiback_selected_device, AMBIBACK_MAX_IDLEN);

  // Enable, disable RELAY Mode
  global::SetAmbibackRelaymodeEnabled( (strLen > 0 ? true : false) );
}
void global::SetAmbibackRelaymodeEnabled(bool bNew) {
  _DBGPRINTF(PSTR("global:SetAmbibackRelaymodeEnabled=%d/%d\n"), global::m_ambiback_relaymode_enabled, bNew);

  global::m_ambiback_relaymode_enabled = bNew;
  global::SetAmbibackRelaymodeInitialized(false);

  // Reset LED Settings
  global::m_pLedHelper->SetSetupVertical(global::m_pEepromHelper->GetSettingByte("SetupVertical", 18), false);
  global::m_pLedHelper->SetSetupHorizontal(global::m_pEepromHelper->GetSettingByte("SetupHorizontal", 32), false);
  global::m_pLedHelper->SetSetupDirection(global::m_pEepromHelper->GetSettingByte("SetupDirection", 0x01), false);

  // Restore FRAME Settings
  global::m_pLedHelper->SetFrameLength((byte) global::m_pEepromHelper->GetSettingByte("FrameLength", DEFAULT_FRAME_LENGTH), false);
  global::m_pLedHelper->SetDelayLength((byte) global::m_pEepromHelper->GetSettingByte("DelayLength", DEFAULT_FRAME_SMOOTHING), false);
  global::SetMotionPreset( global::m_pEepromHelper->GetSettingInt("MotionPreset", 2), false);
}
void global::SetAmbibackRelayIPAddr(char* pszNew, bool bEpprom) {
  size_t strLen = strlen(pszNew);
  if (strLen > 0) {
    global::m_ambiback_relaymode_ipaddr.fromString(pszNew);
    _DBGPRINTF(PSTR("global:SetAmbibackRelayIPAddr=%s(%d)\n"), pszNew, strLen);
  }

  // Store into EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingString("AmbiBackSelLIP", global::m_ambiback_relaymode_ipaddr.toString().c_str(), STR_IPNAME_MAX);
}
void global::SetAmbibackRelaySsid(char* pszNew, bool bEpprom) {
  size_t strLen = 0;
  if (pszNew != NULL) {
    strLen = strlen(pszNew);
    memcpy(global::m_ambiback_relay_ssid, pszNew, strLen);
    global::m_ambiback_relay_ssid[strLen] = '\0';
  } else {
    // Just Terminate string 
    global::m_ambiback_relay_ssid[0] = '\0';
  }
  
  _DBGPRINTF(PSTR("global:SetAmbibackRelaySsid=%s/%s(%d)\n"), (pszNew != NULL ? pszNew : "NULL"), global::m_ambiback_relay_ssid, strLen);
  // Store into EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingString("AmbiBackSeSSID", global::m_ambiback_relay_ssid, WLAN_SSID_MAX);
}
void global::SetAmbibackRelayPass(char* pszNew, bool bEpprom) {
  size_t strLen = 0;
  if (pszNew != NULL) {
    strLen = strlen(pszNew);
    memcpy(global::m_ambiback_relay_password, pszNew, strLen);
    global::m_ambiback_relay_password[strLen] = '\0';
  } else {
    // Just Terminate string 
    global::m_ambiback_relay_password[0] = '\0';
  }
  _DBGPRINTF(PSTR("global:SetAmbibackRelayPass=%s/%s(%d)\n"), (pszNew != NULL ? pszNew : "NULL"), global::m_ambiback_relay_password, strLen);

  // Store into EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingString("AmbiBackSeKEY", global::m_ambiback_relay_password, WLAN_KEY_MAX);
}

bool global::InitializeAmbibackRelaymode() {
  // First find the AmbiBack values
  int iFoundDeviceID = DEVICE_NOT_FOUND;
  int iDeviceCount = global::GetAmbibackDeviceCount();
  global::m_ambiback_relaymode_initialized = false;

  if (strlen(global::m_ambiback_selected_device) <= 0) {
    global::errprintf_P(PSTR("global:InitializeAmbibackRelaymode: Failed, RELAY DEVICE not configured!\n"));
    return false;
  }
  if (iDeviceCount <= 0) {
    global::errprintf_P(PSTR("global:InitializeAmbibackRelaymode: Failed, no other AmbiBack devices found - sending discovery broadcast!\n"));
    if (!UdpServer::m_pUdpServer->SendUdpDiscoverBroadcast()) {
      global::errprintf_P(PSTR("global:InitializeAmbibackRelaymode: sending discovery broadcast FAILED!\n"));
    }
    return false;
  }

  for (int i = 0; i < iDeviceCount; i++) {
    if ( strcmp(global::m_ambiback_selected_device, global::m_ambiback_devices_mac[i] ) == 0) {
      iFoundDeviceID = i;
      break;
    }
  }

  if (iFoundDeviceID == DEVICE_NOT_FOUND) {
    global::errprintf_P(PSTR("global:InitializeAmbibackRelaymode: Failed, configured AmbiBack devices with MAC '%s' NOT FOUND!\n"), global::m_ambiback_selected_device);
    if (!global::GetAmbibackDiscoverySend() || strlen(global::GetAmbibackRelayIPAddrChar()) == 0 ) {
      // Trigger discovery!
      UdpServer::m_pUdpServer->SendUdpDiscoverBroadcast();
      global::SetAmbibackDiscoverySend(true);
      return false;
    } else {
      // TRY LAST KNOWN IP 
      _DBGPRINTF(PSTR("global:InitializeAmbibackRelaymode: Discovery already send, try last known IP Address '%s'!\n"), global::GetAmbibackRelayIPAddrChar() );
      if (global::CheckInitializeAmbibackRelaymode(global::GetAmbibackRelayIPAddrChar())) {
        // Return success result!
        global::SetAmbibackDiscoverySend(false);
        return true;
      }
    }
  } else {
    if (global::CheckInitializeAmbibackRelaymode(global::m_ambiback_devices_ip[iFoundDeviceID])) {
      global::SetAmbibackDiscoverySend(false);
    } else {
      // Trigger discovery - AGAIN!
      UdpServer::m_pUdpServer->SendUdpDiscoverBroadcast();
      return false;
    }
  }
  
  // Return success result!
  return true;
}
bool global::CheckInitializeAmbibackRelaymode(char* pszRemoteIP) {
  bool bReturn = false;

  // Obtain LED Parameters from AmbiBack Debvice
  char szQuryUrl[AMBIBACK_MAX_URLSIZE];
  size_t stLen = snprintf_P(szQuryUrl, AMBIBACK_MAX_URLSIZE, PSTR("http://%s/getparamatersasjson"), pszRemoteIP);

  s_relayHttp.setConnectTimeout(1000); // Set Timeout low to 1000ms
  s_relayHttp.setReuse(false);

  // Perform HTTP Request
  s_relayHttp.begin(szQuryUrl); //HTTP
  int iHttpCode = s_relayHttp.GET();

  // Check http result
  if (iHttpCode == HTTP_CODE_OK) {
    // HTTP header has been send and Server response header has been handled
    _DBGPRINTF(PSTR("global: InitializeAmbibackRelaymode: Http request to %s, HTTP code: %d\n"), szQuryUrl, iHttpCode);

    // Get String from http request for debug
    String payload = s_relayHttp.getString();
    _DBGPRINTF_ULTRA(PSTR("global: InitializeAmbibackRelaymode: Http response payload: %.*s\n"), (payload.length() < 924 ? payload.length() : 924), payload.c_str());

    if (payload.length() > 0) {
      _DBGPRINTF(PSTR("global: InitializeAmbibackRelaymode: SUCCESSFULLY obtained device parameters from %s\n"), pszRemoteIP);

      // Check if our entertain group exists
      // Parse JSON
      cJSON *json = cJSON_Parse(payload.c_str());
      if (json != NULL) {
        cJSON *json_vertical = cJSON_GetObjectItemCaseSensitive(json, "vertical");
        cJSON *json_horizontal = cJSON_GetObjectItemCaseSensitive(json, "horizontal");
        cJSON *json_direction = cJSON_GetObjectItemCaseSensitive(json, "direction");
        cJSON *json_deviceapssid = cJSON_GetObjectItemCaseSensitive(json, "deviceapssid");
        cJSON *json_deviceappass = cJSON_GetObjectItemCaseSensitive(json, "deviceappass");
        
        int iVertical =   (json_vertical != NULL && json_vertical->valuestring != NULL ? atoi(json_vertical->valuestring) : VALUE_NOT_FOUND);
        int iHorizontal = (json_horizontal != NULL && json_horizontal->valuestring != NULL ? atoi(json_horizontal->valuestring) : VALUE_NOT_FOUND);
        int iDirection =  (json_direction != NULL && json_direction->valuestring != NULL ? atoi(json_direction->valuestring) : VALUE_NOT_FOUND);
        char* pszAPSSID = (json_deviceapssid != NULL && json_deviceapssid->valuestring != NULL ? json_deviceapssid->valuestring : NULL);
        char* pszAPKEY =  (json_deviceappass != NULL && json_deviceappass->valuestring != NULL ? json_deviceappass->valuestring : NULL);

        // Temporary overwrite LED Settings
        global::m_pLedHelper->SetSetupVertical((byte) iVertical, false);
        global::m_pLedHelper->SetSetupHorizontal((byte) iHorizontal, false);
        global::m_pLedHelper->SetSetupDirection((byte) iDirection, false);

        // Temporary overwrite FRAME Settings
        global::m_pLedHelper->SetFrameLength((byte) RELAY_FRAME_LENGTH, false);
        global::m_pLedHelper->SetDelayLength((byte) RELAY_DELAY_LENGTH, false);
        global::m_pLedHelper->SetFrameSmoothing(1, false);

        // Set Target IP address
        global::SetAmbibackRelayIPAddr(pszRemoteIP);

        // Set Target IP Settings
        global::SetAmbibackRelaySsid(pszAPSSID);
        global::SetAmbibackRelayPass(pszAPKEY);
        
        if (global::GetAmbibackWifiDirectMode() && pszAPSSID != NULL) {
          _DBGPRINTF(PSTR("global: InitializeAmbibackRelaymode: WIFI Direct enabled, we need to switch the WLAN temporary to %s\n"), global::GetAmbibackRelaySsid());
          if (global::ConnectWifiDirect()) {
            _DBGPRINTF(PSTR("global: InitializeAmbibackRelaymode: WIFI Direct successfully connected, local IP is %s - change RelayIPAddr to %s\n"), 
              WiFi.localIP().toString().c_str(),
              WiFi.gatewayIP().toString().c_str() );
            // Temporary change RELAY IP to Gateway IP
            global::SetAmbibackRelayIPAddr((char*) WiFi.gatewayIP().toString().c_str(), false);
          }
        }

        // Free JSON
        cJSON_Delete(json);

        global::m_ambiback_relaymode_initialized = true;
        bReturn = true;
      } else {
        global::errprintf_P(PSTR("global:InitializeAmbibackRelaymode: Failed, configured AmbiBack devices did not return valid JSON from '%s'!\n"), szQuryUrl);
        bReturn = false;
      }
    } else {
      global::errprintf_P(PSTR("global:InitializeAmbibackRelaymode: Failed, configured AmbiBack devices did not return data at all from '%s'!\n"), szQuryUrl);
      bReturn = false;
    }
  } else {
    global::errprintf_P(PSTR("global:InitializeAmbibackRelaymode: Failed, configured AmbiBack devices did not return data at all from '%s' with httpcode %d!\n"), szQuryUrl, iHttpCode);
    bReturn = false;
  }

  // Return result here
  return bReturn;
}

bool global::ConnectWifiDirect() {
  char statusmsg[MAX_STATUS_MSG_SIZE];
  bool wifiSuccess = false; 

  // Disable WIFI Checking
  global::SetWifiConnectCheck(false);
  
  // Disconnect from current WIFI!
  global::WIFIDisconnect((char*) WiFi.SSID().c_str());
  
  // Start Connect to WIFI DIRECT Network
  if ( global::WIFIConnect(global::GetAmbibackRelaySsid(), global::GetAmbibackRelayPass()) ) {
    wifiSuccess = true;
  } else {
    // Use this function to restore own WIFI
    global::ConnectWifiConfigured(global::GetWifiSSID(), global::GetWifiPassword(), false); 
    
    // Set Last Status Message!
    snprintf_P(statusmsg, MAX_STATUS_MSG_SIZE, PSTR("\"status\": \"0\",\n\"action\": \"ConnectWifiDirect\",\n\"SSID\": \"%s\" "), global::GetAmbibackRelaySsid() );
    global::m_pWebServer->SetLastStatusMessage( statusmsg );
  }

  // Set Direct Connected State 
  global::SetWifiDirectConnected(wifiSuccess);

  // Enable WIFI Checking
  global::SetWifiConnectCheck(true);
  
  // return success state
  return wifiSuccess;
}

#endif
