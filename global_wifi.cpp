#include "globalincludes.h"
#include "global.h"
#include "udpserver.h"
#if defined(ENABLE_TCP)
# include "tcpserver.h"
#endif
#include "esp_wps.h"
#include <GeneralUtils.h>
#include "esp_crc.h"
void global::HandleWifiMode(void) {
  wifi_mode_t wifiMode = (global::GetWifiAPEnable() ? WIFI_MODE_APSTA : WIFI_MODE_STA);
  
  // Check WIFI Mode
  if (WiFi.getMode() != wifiMode) {
    WiFi.mode(wifiMode);
    _DBGPRINTF(PSTR("global:HandleWifiMode: WiFi.mode set to (%s), SoftAP Allowed: %s\n"),
      (global::GetWifiAPEnable() ? "WIFI_MODE_APSTA" : "WIFI_MODE_STA"),
# if defined(ENABLE_WIFI_AP)
      "YES"
# else
      "NO"
# endif  
    );
  } else {
    _DBGPRINTF(PSTR("global:HandleWifiMode: WiFi.mode already set to (%s), SoftAP Allowed: %s\n"),
      (global::GetWifiAPEnable() ? "WIFI_MODE_APSTA" : "WIFI_MODE_STA"),
# if defined(ENABLE_WIFI_AP)
      "YES"
# else
      "NO"
# endif  
    );
  }
}
void global::HandleWifiConnectSetup(void) {
  /* Try WIFI Connect */
  if (WiFi.SSID().length() > 0) {
    _DBGPRINTF(PSTR("global:HandleWifiConnectSetup: try WIFI connecting to %s ...\n"), WiFi.SSID().c_str() );
    WiFi.begin();
    _delay(1000); // give moment for background processing!
  } else {
    if (  global::GetWifiSSID() != NULL && global::GetWifiPassword() != NULL && strlen(global::GetWifiSSID()) > 0 ) {
      /* WIFI SSID/KEY Configured */
      _DBGPRINTF(PSTR("global:HandleWifiConnectSetup: WIFI SSID or KEY configured, try connecting to %s\n"), global::GetWifiSSID());
      global::ConnectWifiConfigured(global::GetWifiSSID(), global::GetWifiPassword(), true);
      _delay(1000); // give moment for background processing!
    } else {
      _DBGPRINTF(PSTR("global:HandleWifiConnectSetup: WIFI SSID EMPTY\n"));
    }
  }
}

void global::HandleWifiAPSetup(void) {
  // Set Mode if needed
  global::HandleWifiMode();

  // Handle AP Startup!
  if (global::GetWifiAPEnable()) {
    global::StartWifiAP();
  } else {
    global::StopWifiAP();
  }
}

void global::StartWifiAP(void) {
# if defined(ENABLE_WIFI_AP)
  int iChannel = random(1, 13);
  char szApPassword[WLAN_KEY_MAX];
  if (global::IsWifiProvisioningActive()) {
    global::BuildGeneratedApPasswordFromMac(global::GetDeviceIDMac(), szApPassword, WLAN_KEY_MAX);
    WiFi.softAP(global::GetDeviceAPssid(), szApPassword, iChannel);
  } else {
    WiFi.softAP(global::GetDeviceAPssid(), global::GetDeviceAPpass(), iChannel);
  }
  IPAddress myIP = WiFi.softAPIP();
  _DBGPRINTF(PSTR("global:StartWifiAP: local WIFI SSID %s with password %s started on WIFI Channel %d, IP is %s\n"),
    global::GetDeviceAPssid(),
    (global::IsWifiProvisioningActive() ? szApPassword : global::GetDeviceAPpass()),
    iChannel,
    myIP.toString().c_str()
  );
#   if defined(ENABLE_WIFI_NAT)
  // Give DNS servers to AP side:
  esp_err_t err;
#     if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  // ESP32 3.x: Use esp_netif API
  esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  esp_netif_dns_info_t dns_info;

  if (!ap_netif) {
    _ERRPRINTF(PSTR("global:StartWifiAP: Failed to get AP netif handle\n"));
  } else {
    err = esp_netif_dhcps_stop(ap_netif);
    _DBGPRINTF(PSTR("global:StartWifiAP: esp_netif_dhcps_stop: err %s\n"), esp_err_to_name(err));
    
    if (sta_netif) {
      err = esp_netif_get_dns_info(sta_netif, ESP_NETIF_DNS_MAIN, &dns_info);
      _DBGPRINTF(PSTR("global:StartWifiAP: esp_netif_get_dns_info: err %s . ip_dns: " IPSTR "\n"), esp_err_to_name(err), IP2STR(&dns_info.ip.u_addr.ip4));
    }

    // Use GOOGLE DNS!
    dns_info.ip.u_addr.ip4.addr = ipaddr_addr("8.8.8.8");
    dns_info.ip.type = IPADDR_TYPE_V4;
      
    err = esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns_info);
    _DBGPRINTF(PSTR("global:StartWifiAP: esp_netif_set_dns_info ESP_NETIF_DNS_MAIN: err %s . ip_dns:" IPSTR "\n"), esp_err_to_name(err), IP2STR(&dns_info.ip.u_addr.ip4));
    
    dhcps_offer_t opt_val = OFFER_DNS; // supply a dns server via dhcps
    err = esp_netif_dhcps_option(ap_netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &opt_val, sizeof(dhcps_offer_t));
    _DBGPRINTF(PSTR("global:StartWifiAP: esp_netif_dhcps_option: err %s\n"), esp_err_to_name(err));

    err = esp_netif_dhcps_start(ap_netif);
    _DBGPRINTF(PSTR("global:StartWifiAP: esp_netif_dhcps_start: err %s\n"), esp_err_to_name(err));
  }
#     else
  // ESP32 2.x: Use legacy tcpip_adapter API
  tcpip_adapter_dns_info_t ip_dns;

  err = tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
  _DBGPRINTF(PSTR("global:StartWifiAP: tcpip_adapter_dhcps_stop: err %s\n"), esp_err_to_name(err));
  err = tcpip_adapter_get_dns_info(TCPIP_ADAPTER_IF_STA, ESP_NETIF_DNS_MAIN, &ip_dns);
  _DBGPRINTF(PSTR("global:StartWifiAP: tcpip_adapter_get_dns_info: err %s . ip_dns: " IPSTR "\n"), esp_err_to_name(err), IP2STR(&ip_dns.ip.u_addr.ip4));

  // Use GOOGLE DNS!
  ip_dns.ip.u_addr.ip4.addr = ipaddr_addr("8.8.8.8");
  ip_dns.ip.type = IPADDR_TYPE_V4;
    
  err = tcpip_adapter_set_dns_info(TCPIP_ADAPTER_IF_AP, ESP_NETIF_DNS_MAIN, &ip_dns);
  _DBGPRINTF(PSTR("global:StartWifiAP: tcpip_adapter_set_dns_info ESP_NETIF_DNS_MAIN: err %s . ip_dns:" IPSTR "\n"), esp_err_to_name(err), IP2STR(&ip_dns.ip.u_addr.ip4));
  
  dhcps_offer_t opt_val = OFFER_DNS; // supply a dns server via dhcps
  tcpip_adapter_dhcps_option(ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &opt_val, 1);

  err = tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP);
  _DBGPRINTF(PSTR("global:StartWifiAP: tcpip_adapter_dhcps_start: err %s\n"), esp_err_to_name(err));
#     endif
#   endif
# endif  
}
void global::StopWifiAP(void) {
  WiFi.softAPdisconnect(true);
  _DBGPRINTF(PSTR("global:StopWifiAP: local WIFI SSID %s stopped\n"),global::GetDeviceAPssid());
}

# if defined(ENABLE_WIFI_AP)

void global::SetWifiProvisioningToken(char* pszNew) {
  size_t strLen = 0;
  if (pszNew != NULL) {
    strLen = strlen(pszNew);
    if (strLen >= WIFI_PROVISIONING_TOKEN_MAX) {
      strLen = WIFI_PROVISIONING_TOKEN_MAX - 1;
    }
    memcpy(global::m_wifi_provisioning_token, pszNew, strLen);
    global::m_wifi_provisioning_token[strLen] = '\0';
  } else {
    global::m_wifi_provisioning_token[0] = '\0';
  }
}

void global::SetWifiProvisioningCredentials(char* pszSSID, char* pszPassword) {
  size_t strLen = 0;
  if (pszSSID != NULL) {
    strLen = strlen(pszSSID);
    if (strLen >= WLAN_SSID_MAX) {
      strLen = WLAN_SSID_MAX - 1;
    }
    memcpy(global::m_wifi_provisioning_ssid, pszSSID, strLen);
    global::m_wifi_provisioning_ssid[strLen] = '\0';
  } else {
    global::m_wifi_provisioning_ssid[0] = '\0';
  }

  strLen = 0;
  if (pszPassword != NULL) {
    strLen = strlen(pszPassword);
    if (strLen >= WLAN_KEY_MAX) {
      strLen = WLAN_KEY_MAX - 1;
    }
    memcpy(global::m_wifi_provisioning_password, pszPassword, strLen);
    global::m_wifi_provisioning_password[strLen] = '\0';
  } else {
    global::m_wifi_provisioning_password[0] = '\0';
  }

  _DBGPRINTF(PSTR("global:SetWifiProvisioningCredentials: staged SSID=%s passwordLen=%d\n"),
    global::m_wifi_provisioning_ssid,
    strlen(global::m_wifi_provisioning_password));
}

bool global::ValidateWifiProvisioningToken(char* pszToken) {
  if (pszToken == NULL || strlen(pszToken) == 0) {
    _DBGPRINTF(PSTR("global:ValidateWifiProvisioningToken: empty token\n"));
    return false;
  }
  bool bValid = strcmp(pszToken, global::m_wifi_provisioning_token) == 0;
  _DBGPRINTF(PSTR("global:ValidateWifiProvisioningToken: providedLen=%d storedLen=%d valid=%s\n"),
    strlen(pszToken),
    strlen(global::m_wifi_provisioning_token),
    (bValid ? "TRUE" : "FALSE"));
  return bValid;
}

void global::BuildGeneratedApPasswordFromMac(char* pszMac, char* pszOut, size_t stOutLen) {
  if (pszOut == NULL || stOutLen == 0) {
    return;
  }

  if (pszMac == NULL || strlen(pszMac) == 0) {
    pszOut[0] = '\0';
    return;
  }

  uint32_t crcMac = esp_crc32_le(UINT16_MAX, (uint8_t const *)pszMac, strlen(pszMac));
  snprintf_P(pszOut, stOutLen, PSTR("%.12u"), crcMac);
}

unsigned long global::GetWifiProvisioningTimeoutMs(void) {
  return (unsigned long)global::m_bWifiProvisioningTimeoutMinutes * 60000UL;
}

unsigned long global::GetWifiProvisioningExpiresIn(void) {
  if (!global::m_bWifiProvisioningActive || global::m_lWifiProvisioningDeadline == 0) {
    return 0;
  }
  unsigned long timeNow = millis();
  if (timeNow >= global::m_lWifiProvisioningDeadline) {
    return 0;
  }
  return global::m_lWifiProvisioningDeadline - timeNow;
}

void global::ClearWifiProvisioning(void) {
  global::m_bWifiProvisioningActive = false;
  global::m_bWifiProvisioningPending = false;
  global::m_lWifiProvisioningStarted = 0;
  global::m_lWifiProvisioningDeadline = 0;
  global::m_wifi_provisioning_token[0] = '\0';
  global::m_wifi_provisioning_ssid[0] = '\0';
  global::m_wifi_provisioning_password[0] = '\0';
}

void global::StartWifiProvisioning(void) {
  if (!global::GetWifiProvisioningEnabled()) {
    global::ClearWifiProvisioning();
    return;
  }

  if (global::m_bWifiProvisioningActive) {
    global::HandleWifiAPSetup();
    return;
  }

  uint32_t seedA = esp_random();
  uint32_t seedB = esp_random();

  global::m_bWifiProvisioningActive = true;
  global::m_bWifiProvisioningPending = false;
  global::m_lWifiProvisioningStarted = millis();
  global::m_lWifiProvisioningDeadline = global::m_lWifiProvisioningStarted + global::GetWifiProvisioningTimeoutMs();
  global::SetWifiTryWPS(false);
  global::SetForceWifiTryWPS(false);
  snprintf_P(global::m_wifi_provisioning_token, WIFI_PROVISIONING_TOKEN_MAX, PSTR("%08lX%08lX"), (unsigned long)seedA, (unsigned long)seedB);

  // Force-enable the SoftAP at runtime for the boot provisioning window without touching EEPROM.
  if (!global::GetWifiAPEnable()) {
    global::SetWifiAPEnable(1, false);
  }
  global::HandleWifiAPSetup();
}

void global::StopWifiProvisioning(void) {
  bool bApWasEnabled = global::GetWifiAPEnable();
  _DBGPRINTF(PSTR("global:StopWifiProvisioning: active=%d pending=%d apEnabled=%d stagedSSID=%s\n"),
    global::m_bWifiProvisioningActive,
    global::m_bWifiProvisioningPending,
    bApWasEnabled,
    global::GetWifiProvisioningSSID());
  global::ClearWifiProvisioning();
  if (bApWasEnabled) {
    global::SetWifiAPEnable(0, false);
    global::HandleWifiMode();
    global::StopWifiAP();
    _DBGPRINTF(PSTR("global:StopWifiProvisioning: SoftAP stopped, WiFi mode now=%d status=%s\n"),
      WiFi.getMode(),
      global::GetWifiStatusTxt(WiFi.status()));
  }
}

void global::HandleWifiProvisioningWindow(void) {
  if (!global::m_bWifiProvisioningActive) {
    return;
  }

  if (global::GetWifiProvisioningExpiresIn() == 0) {
    _DBGPRINTF(PSTR("global:HandleWifiProvisioningWindow: provisioning boot window expired, disabling AP\n"));
    global::StopWifiProvisioning();
  }
}
# endif

# if defined(ENABLE_WIFI_NAT)
// NAT functions are not available in ESP32 2.0.x Arduino framework (missing from precompiled lwip library)
// They work in ESP-IDF but not in Arduino framework versions 2.0.x
# if !(defined(ARDUINO_ESP32_RELEASE_2_0_0) || (defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR == 2))
void global::HandleWifiAPNAT(void) {
  if (global::GetWifiAPEnable()) {
    ip_napt_enable(WiFi.softAPIP(), 1);
    _DBGPRINTF(PSTR("global:HandleWifiAPNAT: Enabled IP NAT on AP Interface with IP %s\n"), WiFi.softAPIP().toString().c_str() );
  }
}
void global::AddPortMappingWifiAPNAT(u16_t sourcePort, u32_t destAddr, u16_t destPort) {
  /*  
   *   HTTP_PORT_FORWARD
  */
  uint32_t sourceAddr = WiFi.localIP();

  if (htonl(destAddr) > 0xC0A80401 && htonl(destAddr) < 0xC0A804FF) {
    // Perform Port Mapping  
    ip_portmap_add(IP_PROTO_TCP, sourceAddr, sourcePort, destAddr, destPort);
    _DBGPRINTF(PSTR("global:AddPortMappingWifiAPNAT: Add portforwarding from %d.%d.%d.%d:%d to %d.%d.%d.%d:%d\n"), 
      sourceAddr & 0xFF, (sourceAddr >> 8) & 0xFF, (sourceAddr >> 16) & 0xFF, (sourceAddr >> 24) & 0xFF,
      sourcePort,
      destAddr & 0xFF, (destAddr >> 8) & 0xFF, (destAddr >> 16) & 0xFF, (destAddr >> 24) & 0xFF,
      destPort
    );
    global::SetClientConnectedIsNat(true);
  } else {
    _DBGPRINTF(PSTR("global:AddPortMappingWifiAPNAT: SKIP portforwarding, not in internal network: %d.%d.%d.%d\n"), 
      destAddr & 0xFF, (destAddr >> 8) & 0xFF, (destAddr >> 16) & 0xFF, (destAddr >> 24) & 0xFF);
    global::SetClientConnectedIsNat(false);
  }
}
void global::RemovePortMappingWifiAPNAT(u16_t sourcePort) {
  ip_portmap_remove(IP_PROTO_TCP, sourcePort);
  _DBGPRINTF(PSTR("global:RemovePortMappingWifiAPNAT: Remove portforwarding for Port %d\n"), sourcePort);
  global::SetClientConnectedIsNat(false);
}
# else
// ESP32 2.0.x: NAT functions not available, provide stub implementations
void global::HandleWifiAPNAT(void) {
  _DBGPRINTF(PSTR("global:HandleWifiAPNAT: NAT not available in ESP32 2.0.x Arduino framework\n"));
}
void global::AddPortMappingWifiAPNAT(u16_t sourcePort, u32_t destAddr, u16_t destPort) {
  _DBGPRINTF(PSTR("global:AddPortMappingWifiAPNAT: NAT not available in ESP32 2.0.x Arduino framework\n"));
  global::SetClientConnectedIsNat(false);
}
void global::RemovePortMappingWifiAPNAT(u16_t sourcePort) {
  _DBGPRINTF(PSTR("global:RemovePortMappingWifiAPNAT: NAT not available in ESP32 2.0.x Arduino framework\n"));
  global::SetClientConnectedIsNat(false);
}
# endif
# endif
void global::HandleWifiWPS(void* pVoidEsp32wps_config) {
  // Cast pointer
  esp_wps_config_t* pEsp32wps_config = (esp_wps_config_t*)pVoidEsp32wps_config;
  
  /* Check for WPS Mode */
  if (global::GetWifiTryWPS()) {
    // Turn off control LED
    global::ToggleCtlLedOff();

    _DBGPRINTF("global:HandleWifiWPS: WIFI begin WPS ...\n");
      
    /* Disconnect again to be on safe side */
    global::WIFIDisconnect((char*) WiFi.SSID().c_str());

    // Set Mode if needed
    global::HandleWifiMode();
      
    esp_wifi_wps_enable(pEsp32wps_config);
#if defined(WIFI_SETSLEEP)
    // Do not use
    WiFi.setSleep(false);
#endif
    bool wpsSuccess = false; 
    _DBGPRINTF(PSTR("global:HandleWifiWPS: call esp_wifi_wps_start\n"));
    Serial.setDebugOutput(true);
    esp_err_t espRet = esp_wifi_wps_start(0);
    Serial.setDebugOutput(false);
    if (espRet == ESP_OK) {
      unsigned long timeWpsWaitStart = millis(); 
      _DBGPRINTF(PSTR("global:HandleWifiWPS: SUCCESS esp_wifi_wps_start Timeout %d\n"), DEFAULT_WPS_TIMOUT);

      do {
        if (WiFi.status() == WL_CONNECTED) {
          wpsSuccess = true; 
        } else {
          _DBGPRINTF(PSTR("global:HandleWifiWPS: WAIT for esp_wifi_wps_start %d / %d WPS Success = %s\n"), (global::g_timeNow-timeWpsWaitStart), DEFAULT_WPS_TIMOUT, (wpsSuccess ? "TRUE" : "FALSE"));
          _delay(1000); 
          global::TogglePwrLed();
          global::g_timeNow = millis(); 
        }
      } while (global::GetWifiTryWPS() && !wpsSuccess && (global::g_timeNow-timeWpsWaitStart) < DEFAULT_WPS_TIMOUT); // Loop until success or TIMEOUT hits 
      if (!wpsSuccess) {
        _DBGPRINTF(PSTR("global:HandleWifiWPS: WPS FAILED after Timeout %d', rebooting\n"), DEFAULT_WPS_TIMOUT );
        global::RebootController(); 
      }
    } else {
      _DBGPRINTF(PSTR("global:HandleWifiWPS: FAIL esp_wifi_wps_start with return '%d'/'%s'\n"), espRet, GeneralUtils::errorToString(espRet) );
    }
    
    /* Check for result **/
    if(wpsSuccess && WiFi.SSID().length() > 0 && WiFi.psk().length() > 0 ) {
      _DBGPRINTF(PSTR("global:HandleWifiWPS: WPS Successfully connected to '%s', Key '%s', IP Address is: %s\n"), 
        WiFi.SSID().c_str(),
        WiFi.psk().c_str(),
        WiFi.localIP().toString().c_str());

      // Save Settings to EEPROM no done in GLOBAL!
      global::SetWifiSSID((char*) WiFi.SSID().c_str()); 
      global::SetWifiPassword((char*) WiFi.psk().c_str()); 

      // Force Write Changes to EEPROM
      global::m_pEepromHelper->CommitChanges();
      _yield(); 

      // Perform Software Reset of device
      global::RebootController();
      _delay(1000); 
    } else {
      _DBGPRINTF(PSTR("global:HandleWifiWPS: WPS Failed with timeout '%d'\n"), (millis() - global::g_timeWIFICheck));
    }
    
    /* disable WPS check */
    global::SetWifiTryWPS(false);
  }
}
void global::HandleWifiCheck() {
#if defined(ENABLE_WIFI_AP)
  global::HandleWifiProvisioningWindow();
#endif

  if (!global::GetWifiConnectCheck()) {
      _DBGPRINTF_ULTRA(PSTR("global:HandleWifiCheck: WifiEnableCheck is DISABLED\n"));
      /* Update global::g_timeWIFICheck */
      global::g_timeWIFICheck = millis();
  } else {
    /* Update global::g_timeWIFICheck */
    global::g_timeWIFICheck = millis();

    /* WIFI Checking Code */
    if (WiFi.status() == WL_CONNECTED) {
      if (!global::GetWifiConnected()) {
        /* Set Connected State */
        global::SetWifiConnected(true);
        global::SetWifiTryConnect(false);
        global::SetWifiTryReConnect(false);
        
        // Make sure we reset WifiTry counter
        global::g_timeWIFITryConnectCount = 0;

        // Turn ON Power LED 
        global::ToggleCtlLedOn();

        _DBGPRINTF(PSTR("global:HandleWifiCheck: WIFI Succcessfully connected to '%s', IP Address is: %s\n"),
          WiFi.SSID().c_str(), 
          WiFi.localIP().toString().c_str());

        // Start Network Services if first time!
        if (!global::GetNetworkStarted()) {
          // Start Servers now!
          global::HandleStartServers();
#if defined(ENABLE_RELAY)
          UdpServer::m_pUdpServer->SendUdpDiscoverBroadcast();
#endif
          global::SetNetworkStarted(true); 
          _DBGPRINTF(PSTR("global:HandleWifiCheck: Network Services started\n"));  
        }

        // Turn off control LED
        global::ToggleCtlLedOn();

        // Scan available Networks
        global::WIFIScanChannels();
      } else {
        if(WiFi.SSID().length() == 0 && !global::GetWifiTryWPS() ) {
          _ERRPRINTF(PSTR("global:HandleWifiCheck: FATAL ERROR, WIFI Status is CONNECTED but SSID is empty! FORCE REBOOT NOW\n"));
          global::RebootController();
          _delay(1000); 
        }
      }
    } else {
      if (WiFi.status() == WL_IDLE_STATUS ||
          WiFi.status() == WL_NO_SSID_AVAIL || 
          WiFi.status() == WL_CONNECT_FAILED || 
          WiFi.status() == WL_CONNECTION_LOST ||
          WiFi.status() == WL_DISCONNECTED ||
          WiFi.status() == WL_NO_SHIELD
          ) {
        if (!global::GetWifiTryConnect() && !global::GetWifiTryReConnect()) {
          // Force disconnect again
          if ( WiFi.status() != WL_DISCONNECTED) {
            // Add extra Sleep on Startup, we are already trying to connect
            if (millis() < DEFAULT_WIFI_STARTUP_CHECK_WAIT) {
              return;
            }
            _DBGPRINTF(PSTR("global:HandleWifiCheck: FORCE DISCONNECT!\n"));
            global::WIFIDisconnect(NULL);
          }

          // Set Mode if needed
          global::HandleWifiMode();
                    
          // Check if WIFI is configured
          if (  global::GetWifiSSID() == NULL || 
                global::GetWifiPassword() == NULL ||
                strlen(global::GetWifiSSID()) == 0 ){
            /* WIFI SSID/KEY Not Configured */
            _DBGPRINTF(PSTR("global:HandleWifiCheck: WIFI SSID or KEY not configured ...\n"));
                  
            if (  global::GetForceWifiTryWPS() == true ) {
              /* Force WPS first time */
              _DBGPRINTF(PSTR("global:HandleWifiCheck: First Loop, forcing WPS Mode once ...\n"));
              global::SetWifiTryWPS(true);
              global::SetWifiTryConnect(false);
              global::SetWifiTryReConnect(false);

              /* We need to disconnect WIFI in anycase */
              global::WIFIDisconnect(NULL);
              // Set Mode if needed
              global::HandleWifiMode();
            } else {
              _DBGPRINTF(PSTR("global:HandleWifiCheck: ForceWifiTryWPS=false...\n"));
            }
          } else {
            // Toggle control LED On
            global::ToggleCtlLedOn(); 

#if defined(WIFI_SETSLEEP)
            // Disable WIFI Sleeps!
            WiFi.setSleep(false);
#endif

            /* Try WIFI Connect */
            _DBGPRINTF(PSTR("global:HandleWifiCheck: try WIFI connecting to %s/%s...\n"), global::GetWifiSSID(), global::GetWifiPassword());
            WiFi.begin(global::GetWifiSSID(), global::GetWifiPassword());

            global::SetWifiTryConnect(true);
            global::g_timeWIFITryConnect = millis();
          }

          // Force WPS Done
          global::SetForceWifiTryWPS(false);
        } else {
          // Toggle control LED
          global::ToggleCtlLed(255,128,0); 

          // Do fancy LED blink stuff
          unsigned long timePassed = (millis() - global::g_timeButtonPressed); 
          if (timePassed > 0) {
            if ((timePassed / 500) % 2 == 0) {
              global::TogglePwrLed(48,192,255); 
            }
          }
           
          // Check for timeout
          if ( (millis() - global::g_timeWIFITryConnect) > DEFAULT_WIFI_CONNECT_TIMEOUT) {
            _DBGPRINTF(PSTR("global:HandleWifiCheck: WIFI timeout connecting to %s/%s, retry ...\n"), global::GetWifiSSID(), global::GetWifiPassword());
            global::SetWifiTryConnect(false);
            global::SetWifiTryReConnect(false);
          }
        }
      }
    }
  }
}
bool global::ConnectWifiConfigured(char* szSSID, char* szKEY, bool bAsync) {
  unsigned long timeNow = millis();
  unsigned long timeWifiWaitStart = millis();
  unsigned long timeWifiRetry = millis();

  // Disable WIFI Checking
  global::SetWifiConnectCheck(false);
  _DBGPRINTF(PSTR("global:ConnectWifiConfigured: begin SSID=%s passwordLen=%d async=%d currentStatus=%s currentMode=%d currentSSID=%s\n"),
    (szSSID != NULL ? szSSID : "[NULL]"),
    (szKEY != NULL ? strlen(szKEY) : 0),
    bAsync,
    global::GetWifiStatusTxt(WiFi.status()),
    WiFi.getMode(),
    WiFi.SSID().c_str());
  
  // Disconnect from current WIFI!
  if (WiFi.status() == WL_CONNECTED) {
    global::WIFIDisconnect((char*) WiFi.SSID().c_str());
    _DBGPRINTF(PSTR("global:ConnectWifiConfigured: disconnected previous STA, status=%s mode=%d\n"),
      global::GetWifiStatusTxt(WiFi.status()),
      WiFi.getMode());
  }

  // Set Mode if needed
  global::HandleWifiMode();
  _DBGPRINTF(PSTR("global:ConnectWifiConfigured: after HandleWifiMode status=%s mode=%d apEnabled=%d apIP=%s\n"),
    global::GetWifiStatusTxt(WiFi.status()),
    WiFi.getMode(),
    global::GetWifiAPEnable(),
    WiFi.softAPIP().toString().c_str());

#if defined(WIFI_SETSLEEP)
  // Do not use
  WiFi.setSleep(false);
#endif

  // Wait for connection
  bool wifiSuccess = false;
  
  if (bAsync) {
    _DBGPRINTF(PSTR("global:ConnectWifiConfigured: ASYNC WiFi.begin to %s/%s ...\n"), szSSID, szKEY);
    WiFi.begin(szSSID, szKEY);
    _delay(1000);
  } else {
    timeNow = millis();
    timeWifiWaitStart = millis();
    do {
      if (WiFi.status() == WL_CONNECTED) {
        _DBGPRINTF(PSTR("global:ConnectWifiConfigured: Successfully connected to %s - %d / %d\n"), szSSID, (timeNow - timeWifiWaitStart), DEFAULT_WIFI_CONNECT_TIMEOUT);
        wifiSuccess = true;
      } else if ( ( WiFi.status() == WL_IDLE_STATUS || 
                    WiFi.status() == WL_DISCONNECTED )
                  && (timeWifiRetry+(DEFAULT_WIFI_CONNECT_TIMEOUT/2)) < millis()) {
        _DBGPRINTF(PSTR("global:ConnectWifiConfigured trigger WIFI Begin ... \n"));
        // WiFi.disconnect();
        WiFi.begin(szSSID, szKEY);
        timeWifiRetry = millis();
      } else {
        // WiFi.begin(szSSID, szKEY);
          _DBGPRINTF(PSTR("global:ConnectWifiConfigured WAIT for connect to %s (Status %s) - %d / %d\n"), szSSID, global::GetWifiStatusTxt(WiFi.status()), (timeNow - timeWifiWaitStart), DEFAULT_WIFI_CONNECT_TIMEOUT);
        _delay(1000);
        global::TogglePwrLed();
        timeNow = millis();
      }
    } while (!wifiSuccess && (timeNow - timeWifiWaitStart) < DEFAULT_WIFI_CONNECT_TIMEOUT); // Loop until success or TIMEOUT hits
  }

  // Enable WIFI Checking
  global::SetWifiConnectCheck(true);

  _DBGPRINTF(PSTR("global:ConnectWifiConfigured: done success=%d finalStatus=%s finalSSID=%s localIP=%s mode=%d\n"),
    (WiFi.status() == WL_CONNECTED ? 1 : 0),
    global::GetWifiStatusTxt(WiFi.status()),
    WiFi.SSID().c_str(),
    WiFi.localIP().toString().c_str(),
    WiFi.getMode());

  if (WiFi.status() == WL_CONNECTED)
    return true;
  else
    return false;
}
void global::WIFIDisconnect(char* pszCurrentSSID) {
  unsigned long timeNow; 
  unsigned long timeWifiWaitStart; 
  int iDbgCount = 1;
  
  WiFi.mode(WIFI_OFF);
  WiFi.disconnect(true);
  _DBGPRINTF(PSTR("global:WIFIDisconnect disconnect from %s \n"), (pszCurrentSSID != NULL ? pszCurrentSSID : "[NULL]") );
  // Wait for disconnect
  timeNow = millis();
  timeWifiWaitStart = millis();
  
  do {
      if (iDbgCount % 10 == 0) {
        _DBGPRINTF(PSTR("global:WIFIDisconnect WAIT for disconnect from %s - %d / %d\n"), (pszCurrentSSID != NULL ? pszCurrentSSID : WiFi.SSID().c_str()), (timeNow-timeWifiWaitStart), DEFAULT_WIFI_CONNECT_TIMEOUT);
      }
      _delay(100);
      timeNow = millis();
      iDbgCount++;
  } while ( WiFi.isConnected() && (timeNow-timeWifiWaitStart) < DEFAULT_WIFI_CONNECT_TIMEOUT ); // Loop until success or TIMEOUT hits

  // Set Connected States
  global::SetWifiDirectConnected(false);
  global::SetWifiConnected(false);
}
void global::WIFIScanChannels() {
  _DBGPRINTF(PSTR("global::WIFIScanChannels: Trigger SCAN\n"));
  unsigned long timeNow = millis();
  int n = WiFi.scanNetworks(true, true);  // run WiFi.scanNetworks ASYNC!
}
bool global::WIFIConnect(char* pszNewSSID, char* pszNewKEY) {
  bool wifiSuccess = false; 
  unsigned long timeNow; 
  unsigned long timeWifiWaitStart;
  unsigned long timeWifiRetry = millis();
  int iDbgCount = 1;

  // Disable WIFI Checking
  global::SetWifiConnectCheck(false);
  
  _DBGPRINTF(PSTR("global:WIFIConnect: Connect to network %s/%s \n"), pszNewSSID, pszNewKEY );

  // Set Mode if needed
  global::HandleWifiMode();
  
  // Begin new WIFI SYNC
  WiFi.begin(pszNewSSID, pszNewKEY);

  // Wait for connect
  timeNow = millis(); 
  timeWifiWaitStart = millis(); 
  do {
    if (WiFi.status() == WL_CONNECTED) {
      _DBGPRINTF(PSTR("global:WIFIConnect: Successfully connected to %s - %d / %d\n"), pszNewSSID, (timeNow - timeWifiWaitStart), DEFAULT_WIFI_CONNECT_TIMEOUT);
      wifiSuccess = true;
    } else if ( ( WiFi.status() == WL_IDLE_STATUS || 
                  WiFi.status() == WL_DISCONNECTED )
                && (timeWifiRetry+(DEFAULT_WIFI_CONNECT_TIMEOUT/2)) < millis()) {
      _DBGPRINTF(PSTR("global:WIFIConnect: Trigger WIFI Begin to network %s/%s \n"), pszNewSSID, pszNewKEY );
      WiFi.begin(pszNewSSID, pszNewKEY);
      timeWifiRetry = millis();
    } else {
      if (iDbgCount % 10 == 0) {
        _DBGPRINTF(PSTR("global:WIFIConnect: WAIT for connect to %s (Status %s) - %d / %d\n"), pszNewSSID, global::GetWifiStatusTxt(WiFi.status()), (timeNow-timeWifiWaitStart), DEFAULT_WIFI_CONNECT_TIMEOUT);
        global::TogglePwrLed();
      }
    }
    _delay(100);
    timeNow = millis(); 
    iDbgCount++;
  } while (!wifiSuccess && (timeNow-timeWifiWaitStart) < DEFAULT_WIFI_CONNECT_TIMEOUT); // Loop until success or TIMEOUT hits 

  // Enable WIFI Checking
  global::SetWifiConnectCheck(true);

  // return results
  _DBGPRINTF(PSTR("global:WIFIConnect: Result: %s - Time %d\n"), (wifiSuccess ? "TRUE" : "FALSE"), (timeNow-timeWifiWaitStart));
  return wifiSuccess;
}

void global::HandleStartServers() {
  // Starting Servers now
  _DBGPRINTF(PSTR("global:HandleStartServers: Start Network Services \n"));  
  if (UdpServer::m_pUdpServer != NULL)
    UdpServer::m_pUdpServer->StartServer();
#if defined(ENABLE_TCP)
  if (TcpServer::m_pTcpServer != NULL)
    TcpServer::m_pTcpServer->StartServer();
#endif
  if (global::m_pWebServer != NULL)
    global::m_pWebServer->StartServer();
#if defined(ENABLE_DREAMSCREEN)
  global::StartServerDreamScreen();
#endif
#if defined(ESP32_DEBUG_FREEMEM)
  global::printEspMemUsage("global:HandleStartServers", "start servers"); 
#endif
#if defined(ENABLE_WIFI_NAT)
  global::HandleWifiAPNAT();
#endif

  /* Special, Read last LED Mode!
  * Autostart CAMERA if enabled last time 
  * If ENABLE_DBLRBT_WPS is defined, LedMode is set to LED_MODE_OFF when doublreboot is detected!
  */
  byte bLastLedMode = global::m_pEepromHelper->GetSettingByte("LedMode", LED_MODE_OFF);
  if (global::GetRestoreLastLedMode() &&
      (bLastLedMode == LED_MODE_CAM || bLastLedMode == LED_MODE_AMBIENT)) {
    global::m_pLedHelper->SetLedMode(bLastLedMode);
  }
}

void global::DeInitSyslogClient() {
  // Free memory
  if (global::m_pSyslog != NULL) {
    delete global::m_pSyslog;
    global::m_pSyslog = NULL;
  }
}

void global::InitSyslogClient() {
  // DeInit first
  global::DeInitSyslogClient();

  if (strlen(global::m_DebugSyslogServer) > 0) {
    // Create new syslog instance with LOG_KERN facility
    global::m_pSyslog = new Syslog(global::m_DebugSyslogServer, global::m_iDebugsyslogport, NETWORK_NAME, global::GetDebugSyslogAppName() /*SYSLOG_APP_NAME*/, LOG_KERN, SYSLOG_PROTO_BSD);
  } else {
    global::m_pSyslog = NULL;
  }
}

void global::SetWifiSSID(char* pszNew, bool bEpprom) {
  size_t strLen = 0;
  if (pszNew != NULL) {
    strLen = strlen(pszNew);
    memcpy(global::m_wifi_ssid, pszNew, strLen);
    global::m_wifi_ssid[strLen] = '\0';
  } else {
    global::m_wifi_ssid[strLen] = '\0';
  }
  _DBGPRINTF(PSTR("global:SetWifiSSID=%s/%s(%d)\n"), (pszNew != NULL ? pszNew : "NULL"), global::m_wifi_ssid, strLen);
  // Store into EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingString(WLAN_SSID_START, global::m_wifi_ssid, WLAN_SSID_MAX);
}

void global::SetWifiPassword(char* pszNew, bool bEpprom) {
  size_t strLen = 0;
  if (pszNew != NULL) {
    strLen = strlen(pszNew);
    memcpy(global::m_wifi_password, pszNew, strLen);
    global::m_wifi_password[strLen] = '\0';
  } else {
    global::m_wifi_password[strLen] = '\0';
  }
  _DBGPRINTF(PSTR("global:SetWifiPassword=%s/%s(%d)\n"), (pszNew != NULL ? pszNew : "NULL"), global::m_wifi_password, strLen);

  // Store into EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingString(WLAN_KEY_START, global::m_wifi_password, WLAN_KEY_MAX);
}

void global::SetDeviceAPssid(char* pszNew, bool bEpprom) {
  size_t strLen = strlen(pszNew);
  if (strLen > 0) {
    // Get SSID from settings
    memcpy(global::m_wifi_ap_ssid, pszNew, strLen);
    global::m_wifi_ap_ssid[strLen] = '\0';
  } else {
    // Use default SSID
    snprintf_P(global::m_wifi_ap_ssid, WLAN_SSID_MAX, PSTR("AmbiBack %s"), global::GetDeviceIDMac());
    bEpprom = true; // SAVE!
  }

  _DBGPRINTF(PSTR("global:SetDeviceAPssid=%s/%s(%d)\n"), pszNew, global::m_wifi_ap_ssid, strLen);
  // Store into EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingString("APSSID", global::m_wifi_ap_ssid, WLAN_SSID_MAX);
}

void global::SetDeviceAPpass(char* pszNew, bool bEpprom) {
  size_t strLen = strlen(pszNew);
  
  if (strLen > 0) {
    // Get KEY from settings
    memcpy(global::m_wifi_ap_password, pszNew, strLen);
    global::m_wifi_ap_password[strLen] = '\0';
  } else {
    global::BuildGeneratedApPasswordFromMac(global::GetDeviceIDMac(), global::m_wifi_ap_password, WLAN_KEY_MAX);
    bEpprom = true; // SAVE!
  }
  _DBGPRINTF(PSTR("global:SetDeviceAPpass=%s/%s(%d)\n"), pszNew, global::m_wifi_ap_password, strLen);

  // Store into EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingString("APKEY", global::m_wifi_ap_password, WLAN_KEY_MAX);
}

void global::SetWifiAPEnable(byte bNew, bool bEpprom) {
  global::m_wifi_ap_enabled = (bNew == 0 ? false : true);
  _DBGPRINTF(PSTR("global:SetWifiAPEnable Value=%d\n"), bNew);
  
  // Write to EEPROM!
  if (bEpprom) {
    global::m_pEepromHelper->SetSettingByte("AmbiBackUseAP", bNew);

    // HANDLE Wifi AP Enable SWITCH only when EEPROM saved!
    if (global::m_wifi_ap_enabled == false) {
      _DBGPRINTF(PSTR("global: SetWifiAPEnable: AP DISABLED\n"));
      global::StopWifiAP();
    } else {
      _DBGPRINTF(PSTR("global: SetWifiAPEnable: AP ENABLED\n"));
      global::StartWifiAP();
    }
  }
}

void global::SetRestoreLastLedMode(byte bNew, bool bEpprom) {
  global::m_bRestoreLastLedMode = (bNew == 0 ? false : true);
  _DBGPRINTF(PSTR("global:SetRestoreLastLedMode Value=%d\n"), bNew);

  if (bEpprom) {
    global::m_pEepromHelper->SetSettingByte("LedModeRest", (global::m_bRestoreLastLedMode ? 1 : 0));
  }
}

void global::SetWifiProvisioningEnabled(byte bNew, bool bEpprom) {
  bool bWasEnabled = global::m_bWifiProvisioningEnabled;
  global::m_bWifiProvisioningEnabled = (bNew == 0 ? false : true);
  _DBGPRINTF(PSTR("global:SetWifiProvisioningEnabled Value=%d\n"), bNew);

  if (bEpprom) {
    global::m_pEepromHelper->SetSettingByte("ProvEnabled", (global::m_bWifiProvisioningEnabled ? 1 : 0));
  }

  if (bWasEnabled && !global::m_bWifiProvisioningEnabled && global::IsWifiProvisioningActive()) {
    global::StopWifiProvisioning();
  }
}

void global::SetWifiProvisioningTimeoutMinutes(byte bNew, bool bEpprom) {
  if (bNew < WIFI_PROVISIONING_BOOT_WINDOW_MIN_MINUTES) {
    bNew = WIFI_PROVISIONING_BOOT_WINDOW_MIN_MINUTES;
  } else if (bNew > WIFI_PROVISIONING_BOOT_WINDOW_MAX_MINUTES) {
    bNew = WIFI_PROVISIONING_BOOT_WINDOW_MAX_MINUTES;
  }

  global::m_bWifiProvisioningTimeoutMinutes = bNew;
  _DBGPRINTF(PSTR("global:SetWifiProvisioningTimeoutMinutes Value=%d\n"), bNew);

  if (bEpprom) {
    global::m_pEepromHelper->SetSettingByte("ProvToutMin", bNew);
  }

  if (global::IsWifiProvisioningActive()) {
    global::m_lWifiProvisioningDeadline = global::m_lWifiProvisioningStarted + global::GetWifiProvisioningTimeoutMs();
    if (global::GetWifiProvisioningExpiresIn() == 0) {
      global::StopWifiProvisioning();
    }
  }
}

void global::SetDeviceName(char* pszNew, bool bEpprom) {
  size_t strLen = strlen(pszNew);
  if ( strLen > (DEVICE_NAME_MAX - 1))
    strLen = (DEVICE_NAME_MAX - 1); // Limit to max size
  memcpy(global::m_devicename, pszNew, strLen);
  global::m_devicename[strLen] = '\0';
  _DBGPRINTF(PSTR("global:SetDeviceName=%s/%s(%d)\n"), pszNew, global::m_devicename, strLen);

  // Store into EEPROM
  if (bEpprom)
    global::m_pEepromHelper->SetSettingString(DEVICE_NAME_START, global::m_devicename, DEVICE_NAME_MAX);
}

void global::TogglePwrLedOn(byte bRed, byte bGreen, byte bBlue, bool bState) {
  global::TogglePwrLedOn(bRed, bGreen, bBlue, bState, false);
}
void global::TogglePwrLedOn(byte bRed, byte bGreen, byte bBlue, bool bState, bool bDirect) {
  if (PWRLED_PIN >= 0) {
#if defined(USE_WS2812_CONTROL_LEDS)
    if (global::m_pLedHelper != NULL) {
      global::m_pLedHelper->setPowerLedOn(bState);
# if defined(USE_WS2812_POWER_LED) && USE_WS2812_POWER_LED == 1
      g_bPwrLedState = bState;
      g_bPwrRed = bRed;
      g_bPwrGreen = bGreen;
      g_bPwrBlue = bBlue;

      // Wait until old task is finished
      int iTimeout = 0;
      while ( global::m_pTaskManager->StatusTask(&runTaskFlashPwrLed) == TaskState_Running && iTimeout < 1000) {
        _delay(1);
        iTimeout++;
      }

      // Debug output
      _DBGPRINTF(PSTR("TogglePwrLedOn: ENTER (waited %d)\n"), iTimeout);

      global::m_pTaskManager->StartTask(&runTaskFlashPwrLed); // start with turning it on
      // Give TASK a chance to start !
      global::m_pTaskManager->Loop(); // Handle Task Scheduler Tasks
#else
      // Peform the LED toggle
      global::m_pLedHelper->toggleLed(LED_PWRIDX, WS2812_CONTROL_LEDS_BRIGHTNESS, 0, bRed, bGreen, bBlue, (bDirect ? 1 : WS2812_CONTROL_LEDS_STEPS), bState);
#endif
    }
#else
    // activate Power LED
    digitalWrite(PWRLED_PIN, PWRLED_PIN_ON);
#endif
  }
}
void global::TogglePwrLedOn() {
  global::TogglePwrLedOn(false);
}
void global::TogglePwrLedOn(bool bDirect) {
  if (PWRLED_PIN >= 0) {
#ifdef USE_WS2812_CONTROL_LEDS
    global::TogglePwrLedOn(0, 255, 0, true, bDirect);
#else
    // activate Power LED
    digitalWrite(PWRLED_PIN, PWRLED_PIN_ON);
#endif
  }
}
void global::TogglePwrLedMsg() {
  if (PWRLED_PIN >= 0) {
    for (int i = 0; i < 4; i++) {
#ifdef USE_WS2812_CONTROL_LEDS
      global::TogglePwrLedOn(0, 255, 0, false);
      delay(125);
      global::TogglePwrLedOn(0, 255, 0, true);
      delay(125);
#else
      // activate Power LED
      digitalWrite(PWRLED_PIN, PWRLED_PIN_OFF);
      delay(125);
      digitalWrite(PWRLED_PIN, PWRLED_PIN_ON);
      delay(125);
#endif
    }
  }
}

void global::TogglePwrLedOff() {
  if (PWRLED_PIN >= 0) {
#ifdef USE_WS2812_CONTROL_LEDS
    global::TogglePwrLedOn(0, 255, 0, false);
#else
    // activate Power LED
    digitalWrite(PWRLED_PIN, PWRLED_PIN_OFF);
#endif
  }
}
void global::TogglePwrLed() {
  if (PWRLED_PIN >= 0) {
#ifdef USE_WS2812_CONTROL_LEDS
    if (global::m_pLedHelper->getPowerLedOn())
      global::TogglePwrLedOff();
    else
      global::TogglePwrLedOn();
#else
    if (digitalRead(PWRLED_PIN) == PWRLED_PIN_OFF) {
      // activate Power LED
      digitalWrite(PWRLED_PIN, PWRLED_PIN_ON);
    } else {
      // deactivate Power LED
      digitalWrite(PWRLED_PIN, PWRLED_PIN_OFF);
    }
#endif
  }
}
void global::TogglePwrLed(byte bRed, byte bGreen, byte bBlue){
  if (PWRLED_PIN >= 0) {
#ifdef USE_WS2812_CONTROL_LEDS
    if (global::m_pLedHelper->getPowerLedOn())
      global::TogglePwrLedOn(bRed, bGreen, bBlue, false);
    else
      global::TogglePwrLedOn(bRed, bGreen, bBlue, true);
#else
    if (digitalRead(PWRLED_PIN) == PWRLED_PIN_OFF) {
      // activate Power LED
      digitalWrite(PWRLED_PIN, PWRLED_PIN_ON);
    } else {
      // deactivate Power LED
      digitalWrite(PWRLED_PIN, PWRLED_PIN_OFF);
    }
#endif
  }
}
void global::ToggleCtlLedOn(byte bRed, byte bGreen, byte bBlue, bool bState){
  if (CTLLED_PIN >= 0) {
#ifdef USE_WS2812_CONTROL_LEDS
    if (global::m_pLedHelper != NULL) {
      global::m_pLedHelper->setControlLedOn(bState);
#if defined(USE_WS2812_CONTROL_LED) && USE_WS2812_CONTROL_LED == 1
      g_bCtlLedState = bState;
      g_bCtlRed = bRed;
      g_bCtlGreen = bGreen;
      g_bCtlBlue = bBlue;

      // Wait until old task is finished
      int iTimeout = 0;
      while ( global::m_pTaskManager->StatusTask(&runTaskFlashCtlLed) == TaskState_Running && iTimeout < 1000) {
        _delay(1);
        iTimeout++;
      }

      // Debug output
      _DBGPRINTF(PSTR("ToggleCtlLedOn: ENTER (waited %d)\n"), iTimeout);

      global::m_pTaskManager->StartTask(&runTaskFlashCtlLed); // start with turning it on
      // Give TASK a chance to start !
      global::m_pTaskManager->Loop(); // Handle Task Scheduler Tasks
#endif
    }
#else
    // activate Control LED
    digitalWrite(CTLLED_PIN, HIGH);
#endif
  }
}
void global::ToggleCtlLedOn() {
  if (CTLLED_PIN >= 0) {
#ifdef USE_WS2812_CONTROL_LEDS
    global::ToggleCtlLedOn(255, 255, 0, true);
#else
    // activate Ctl LED
    digitalWrite(CTLLED_PIN, HIGH);
#endif
  }
}
void global::ToggleCtlLedOff() {
  if (CTLLED_PIN >= 0) {
#ifdef USE_WS2812_CONTROL_LEDS
    global::ToggleCtlLedOn(255, 255, 0, false);
#else
    // activate Power LED
    digitalWrite(CTLLED_PIN, LOW);
#endif
  }
}
void global::ToggleCtlLed() {
  if (CTLLED_PIN >= 0) {
#ifdef USE_WS2812_CONTROL_LEDS
    if (global::m_pLedHelper->getControlLedOn())
      global::ToggleCtlLedOff();
    else
      global::ToggleCtlLedOn();
#else
    if (digitalRead(CTLLED_PIN) == LOW) {
      // activate Ctl LED
      digitalWrite(CTLLED_PIN, HIGH);
    } else {
      // deactivate Ctl LED
      digitalWrite(CTLLED_PIN, LOW);
    }
#endif
  }
}
void global::ToggleCtlLed(byte bRed, byte bGreen, byte bBlue) {
  if (CTLLED_PIN >= 0) {
#ifdef USE_WS2812_CONTROL_LEDS
    if (global::m_pLedHelper->getControlLedOn())
      global::ToggleCtlLedOn(bRed, bGreen, bBlue, false);
    else
      global::ToggleCtlLedOn(bRed, bGreen, bBlue, true);
#else
    if (digitalRead(CTLLED_PIN) == LOW) {
      // activate Ctl LED
      digitalWrite(CTLLED_PIN, HIGH);
    } else {
      // deactivate Ctl LED
      digitalWrite(CTLLED_PIN, LOW);
    }
#endif
  }
}

char* global::GetWifiStatusTxt(int iStatus) {
  char* pszWifiStatus;
  switch (iStatus) {
    case WL_NO_SHIELD:
      pszWifiStatus = (char*) PSTR("No Shield");
      break;
    case WL_IDLE_STATUS:
      pszWifiStatus = (char*) PSTR("Idle");
      break;
    case WL_NO_SSID_AVAIL:
      pszWifiStatus = (char*) PSTR("No SSID available");
      break;
    case WL_SCAN_COMPLETED:
      pszWifiStatus = (char*) PSTR("Scan completed");
      break;
    case WL_CONNECTED:
      pszWifiStatus = (char*) PSTR("Connected");
      break;
    case WL_CONNECT_FAILED:
      pszWifiStatus = (char*) PSTR("Connect failed");
      break;
    case WL_CONNECTION_LOST:
      pszWifiStatus = (char*) PSTR("Connect lost");
      break;
    case WL_DISCONNECTED:
      pszWifiStatus = (char*) PSTR("Disconnected");
      break;
    default:
      pszWifiStatus = (char*) PSTR("UNKNOWN");
  }

  return pszWifiStatus;
}

char* global::GetWifiAuthTxt(int iStatus) {
  char* pszWifiauth;
  switch (iStatus) {
    case WIFI_AUTH_OPEN:
      pszWifiauth = (char*) PSTR("OPEN");
      break;
    case WIFI_AUTH_WEP:
      pszWifiauth = (char*) PSTR("WEP");
      break;
    case WIFI_AUTH_WPA_PSK:
      pszWifiauth = (char*) PSTR("WPA PSK");
      break;
    case WIFI_AUTH_WPA2_PSK:
      pszWifiauth = (char*) PSTR("WPA2 PSK");
      break;
    case WIFI_AUTH_WPA_WPA2_PSK:
      pszWifiauth = (char*) PSTR("WPA WPA2 PSK");
      break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
      pszWifiauth = (char*) PSTR("WPA2 ENTERPRISE");
      break;
    case WIFI_AUTH_WPA3_PSK:
      pszWifiauth = (char*) PSTR("WPA3 PSK");
      break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
      pszWifiauth = (char*) PSTR("WPA2 WPA3 PSK");
      break;
    case WIFI_AUTH_WAPI_PSK:
      pszWifiauth = (char*) PSTR("WIFI AUTH WAPI PSK");
      break;
    default:
      pszWifiauth = (char*) PSTR("WIFI AUTH UNKNOWN");
  }

  return pszWifiauth;
}
