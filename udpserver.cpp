// Include all needed files
#include "globalincludes.h"
#include "udpserver.h"
#include <cJSON.h>

unsigned long timeSerialCheck = 0;
UdpServer* UdpServer::m_pUdpServer = NULL;

#define HTTP_DISCOVER_MAX 20
#define HTTP_DISCOVER_STR 48
static bool s_httpDiscoverActive = false;
static byte s_httpDiscoverCount = 0;
static char s_httpDiscoverName[HTTP_DISCOVER_MAX][HTTP_DISCOVER_STR];
static char s_httpDiscoverId[HTTP_DISCOVER_MAX][HTTP_DISCOVER_STR];
static char s_httpDiscoverMac[HTTP_DISCOVER_MAX][HTTP_DISCOVER_STR];
static char s_httpDiscoverIp[HTTP_DISCOVER_MAX][HTTP_DISCOVER_STR];

void handleUdpPacket(AsyncUDPPacket packet);

UdpServer::UdpServer(int nPort){
  m_pAsyncUdp = new AsyncUDP();
  m_iPort = nPort;
  UdpServer::m_pUdpServer = this;
  m_lLastUpdate = 0;
}

void UdpServer::StartServer(){
  if(m_pAsyncUdp->listen(m_iPort)) {
    _DBGPRINTF(PSTR("UdpServer: Successfully started Udp Server on Port: %d\n"), m_iPort);
  } else {
    _DBGPRINTF(PSTR("UdpServer: Failed starting Udp Server on Port: %d\n"), m_iPort);
  }

  m_pAsyncUdp->onPacket(&handleUdpPacket);   
}

void UdpServer::SetLastUpdate() {
  m_lLastUpdate = millis(); 
  global::m_pLedHelper->SetLastUpdate();
}

#if defined(ENABLE_RELAY)
bool UdpServer::SendUdpDiscoverBroadcast() {
  char szBroadcast[NET_BROADCAST_SIZE];
  size_t stLen = snprintf_P(szBroadcast, NET_BROADCAST_SIZE, PSTR("%s\n"), BROADCAST_DISCOVER_MSG);

  // Reset devices
  global::SetAmbibackDeviceCount(0);
  
  // Send UDP Broadcast  
  return this->SendUdpBroadcast((byte*)szBroadcast, stLen);
}
#endif

bool UdpServer::SendUdpBroadcast(byte* pbBuf, size_t stBufLen) {
#if defined(ENABLE_WIFI_AP)
  if ( /* Send UDP Broadcast because we have AP Mode */
#else
  if (WiFi.isConnected() && 
#endif
      global::GetNetworkStarted()) {
    // Send broadcast into network
    size_t stBytesSend = m_pAsyncUdp->broadcastTo((uint8_t*) pbBuf, stBufLen, m_iPort);
    return (stBytesSend > 0 ? true : false);
  } else {
    return false;
  }
}

bool UdpServer::SendUdpMessage(byte* pbBuf, size_t stBufLen, IPAddress ip, int iPort) {
  _DBGPRINTF_ULTRA(PSTR("UdpServer:SendUdpMessage %p/%p To:%s:%d, Length:%d\n"),
      UdpServer::m_pUdpServer, this, 
      ip.toString().c_str(), iPort, stBufLen);
#if defined(ENABLE_WIFI_AP)
  if ( /* Send UDP Broadcast because we have AP Mode */
#else
  if (WiFi.isConnected() && 
#endif
      global::GetNetworkStarted()) {
    // Send broadcast into network
    size_t stBytesSend = this->GetAsyncUdp()->writeTo((const uint8_t*) pbBuf, stBufLen, ip, iPort);
    return (stBytesSend > 0 ? true : false);
  } else {
    return false;
  }
}

void handleUdpPacket(AsyncUDPPacket packet) {
  bool bSendSuccess = false;
  uint8_t* pData = packet.data();
  IPAddress remoteClient = packet.remoteIP();
  String strIpRemote = remoteClient.toString();
  String strIpLocal = packet.localIP().toString();

  _DBGPRINTF_ULTRA(PSTR("UdpServer:handleUdpPacket Received UDP Packet Type: %s, From: %s:%d, To:%s:%d, Length:%d, Data: %.*s\n"), 
      packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast",
      strIpRemote.c_str(), 
      packet.remotePort(),
      strIpLocal.c_str(), 
      packet.localPort(),
      packet.length(),
      packet.length(),
      pData
      );
  
  // Handle Broadcast!
  if (true) { // We handle ALL types now! packet.isBroadcast()) {
    size_t stSize = strlen(BROADCAST_DISCOVER_MSG); 
    if (packet.length() < stSize)
      stSize = packet.length();

    if (stSize < (UDP_HEADLEN+UDP_CHUNKSIZE)) {
      // Package to small, discard!
      // _DBGPRINTF(PSTR("UdpServer:handleUdpPacket Package to short(%d/%d) %s:%d Data: %.*s\n"), 
      //   stSize, 
      //   (UDP_HEADLEN+UDP_HEADLEN), 
      //   strIpRemote.c_str(), 
      //   packet.remotePort(), 
      //   packet.length(),
      //   pData);
      return;
    }

    // Check for UDP LEDDATA first!
    if (*pData == 0x00 && *(pData+1) == 0xFF && *(pData+2) == 0x13 && *(pData+3) == 0x37) {
      // Move pointer
      pData+=UDP_CHUNKSIZE;

      // Try to process AmbiBack Header
      byte bLeds = *(pData);
      byte bSequenceLow = *(pData+1); // previous bStartIndex but now bSequenceLow
      byte bCrcCheck = *(pData+2);
      byte bCrc8 = *(pData+3);

      // Payload after magic: 4-byte header + nPixels*4. Derive count from length for OLD/NEW sender compat:
      // OLD: bLeds = actual count (0-255). NEW: bLeds = stored count-1 (0=1 LED .. 255=256 LEDs).
      size_t stExpectedLen = packet.length() - UDP_CHUNKSIZE;
      size_t nPixels = (stExpectedLen >= 4) ? (stExpectedLen - UDP_CHUNKSIZE) / UDP_CHUNKSIZE : 0;
      if (stExpectedLen < 8 || (stExpectedLen - UDP_CHUNKSIZE) % UDP_CHUNKSIZE != 0) {
          _DBGPRINTF(PSTR("UdpServer:handleUdpPacket Invalid length %d - DISCARD\n"), (int)stExpectedLen);
          return;
      }

      // Generate our own checksum (over pixel data)
      byte bCrcCompare = (byte) global::crc_8(pData+UDP_CHUNKSIZE, stExpectedLen-UDP_CHUNKSIZE);
      _DBGPRINTF_ULTRA(PSTR("UdpServer:handleUdpPacket process len=%d pixels=%d bCrc8=0x%x crcCompare=0x%x\n"), (int)stExpectedLen, (int)nPixels, bCrc8, bCrcCompare ); 

      /* perform CrcCheck only if bCrcCheck is not NULL! */
      if (bCrcCheck == 0x01 && bCrc8 == bCrcCompare) {
        // Combine sequence number
        byte bSequenceHigh = *(pData+UDP_CHUNKSIZE);
        unsigned short shNetSequence = (unsigned short) ((bSequenceHigh << 8) | (bSequenceLow & 0xff));

        // Valid Data, set Client Connected (Also if ClientConnectedAddr is 0) !
        if (!global::GetClientConnected() || global::GetClientConnectedAddr() == 0 ) {
          global::SetClientConnected(true);
          global::SetClientConnectedIpStr(&remoteClient);
          _DBGPRINTF(PSTR("UdpServer:handleUdpPacket New Client connected with IP '%s' / '%s' \n"), strIpRemote.c_str(), strIpLocal.c_str());
        } else {
          if ( strcmp(strIpRemote.c_str(), global::GetClientConnectedIpStr()) != 0) {
            global::errprintf_P(PSTR("UdpServer:handleUdpPacket Client '%s' already sending data, DROPPING packet from '%s'\n"), 
              global::GetClientConnectedIpStr(), strIpRemote.c_str());
              
            // Increment Counter
            global::IncrementCurrentDroppedCtr();
            
            return;
          }
        }

#if defined(ESP32)
        // Ok if this takes a little longer and we are on SYSTEM_RUNNING_CORE, disable WATCHDOG!
        global::DisableSysCoreWatchDog();
#endif

        // Still not processed, DROP Packet
        if (global::m_pLedHelper->GetIsNewLedData()) {
          _DBGPRINTF_ULTRA(PSTR("UdpServer:handleUdpPacket received new data, but old data has not been processed - DROP LED DATA\n")); 

          /* Make sure we stay in VIDEO Mode!*/
          global::m_pLedHelper->SetChangeLedMode(LED_MODE_VIDEO); 

#if defined(ESP32)
          // Make sure our Process Task is running
          global::m_pLedHelper->EnableTaskProcessNextStep(); 
#endif 

          // Increment Dropped Counter
          global::IncrementCurrentDroppedCtr();

#if defined(ESP32)
          // Enable fucking Watchdog!
          global::EnableSysCoreWatchDog();
#endif
          return;
        }

        // Calculate dynamic Delay Length now!
        global::CalcDynamicDelayLength( UdpServer::m_pUdpServer->GetLastUpdate() );

        // Set Last Updated
        UdpServer::m_pUdpServer->SetLastUpdate();

        /* Set LED to VIDEO */
        global::m_pLedHelper->SetChangeLedMode(LED_MODE_VIDEO); 
        /* Set current LED Count: use length-derived count for OLD/NEW sender compat */
        global::m_pLedHelper->SetCurrentLedCount((uint16_t)nPixels); 

        // --- Now Perform Sequence Handling here
        if (shNetSequence > 0) {
          if (shNetSequence < global::m_pLedHelper->GetSequenceNumber() ) {
            if ( (global::m_pLedHelper->GetSequenceNumber()-shNetSequence) < NET_MAXSEQUENCE_DIFF) {
              // Something is not right with the frame
              global::errprintf_P(PSTR("UdpServer:handleUdpPacket LedData received but sequence not ok, Leds=0x%02X SequenceLow=0x%02X SequenceHigh=0x%02X NetSequence=0x%04X LedSequence=0x%04X - DROPPING\n"), 
                bLeds, bSequenceLow, bSequenceHigh, shNetSequence, global::m_pLedHelper->GetSequenceNumber());
    
              // Increment Counter
              global::IncrementCurrentDroppedCtr();

#if defined(ESP32)
              // Enable fucking Watchdog!
              global::EnableSysCoreWatchDog();
#endif
              // Abort further processing
              return;
            } else {
              // Sequence difference is way to high, Fixing SequenceNumber to received value
              global::errprintf_P(PSTR("UdpServer:handleUdpPacket LedData received but sequence difference (NetSequence=0x%04X LedSequence=0x%04X) to high (more than %d), setting sequence to received one\n"), 
                shNetSequence, global::m_pLedHelper->GetSequenceNumber(), NET_MAXSEQUENCE_DIFF);
            }
          }
        }
        
        // Reset SequenceNumber to received value
        global::m_pLedHelper->SetSequenceNumber(shNetSequence);
        // ---

        /* Process Color Bytes */
        int iColorIndex = 0; 
        for(size_t st=UDP_CHUNKSIZE; (st < stExpectedLen); st+=UDP_CHUNKSIZE) {
          /* Set Color LED Code   | White, Red, Green, Blue */
          global::m_pLedHelper->SetLedColorByIdx(iColorIndex++, *(pData+st), *(pData+st+1), *(pData+st+2), *(pData+st+3)); 
        }

        /* ZERO unconfigured LEDs */
        global::m_pLedHelper->ZeroConfiguredLeds();

        /* Process new LED Data */
        global::m_pLedHelper->SetIsNewLedData(true); 

        // Increment Counter
        global::IncrementCurrentFramesCtr();

#if defined(ESP32)
        // Enable fucking Watchdog!
        global::EnableSysCoreWatchDog();
#endif
      } else {
        global::errprintf_P(PSTR("UdpServer:handleUdpPacket LedData received but not ok, checksum error: header=0x%02X CrcCheck=0x%02X Crc8=0x%02X Calced=0x%02X len=%d\n"), 
          bLeds, bCrcCheck, bCrc8, bCrcCompare, (int)stExpectedLen);

        // Increment Counter
        global::IncrementCurrentDroppedCtr();
      }

      // --- Serial Timer Check!
      if (timeSerialCheck == 0) {
        timeSerialCheck = millis() + 1000;
      } else {
        /* Handle FPS counting after 1000ms period of time */
        if (millis() > timeSerialCheck ) {
          global::CalcFramesCtr();
          timeSerialCheck = millis() + 1000;
        }
      }  
      // --- 
    } else if (*pData == '{' && *(pData+1) == '\n') { // Check for JSON Str
      // Discover answer detected, try process JSON
      // make sure pData is terminate!
      if (*(pData+(packet.length()-1)) != '\0')
        *(pData+(packet.length()-1)) = '\0';
      cJSON *json = cJSON_Parse((const char*) pData);
      if (json != NULL) {
        // Try to read data!
        cJSON *json_name = cJSON_GetObjectItemCaseSensitive(json, "DeviceName");
        cJSON *json_id = cJSON_GetObjectItemCaseSensitive(json, "DeviceID");
        cJSON *json_mac = cJSON_GetObjectItemCaseSensitive(json, "DeviceMAC");
        cJSON *json_ip = cJSON_GetObjectItemCaseSensitive(json, "DeviceIP");

        _DBGPRINTF(PSTR("UdpServer: Discovered AmbiBack Device '%s' with id '%s', mac '%s', IP '%s'\n"), 
          (json_name != NULL && json_name->valuestring != NULL ? json_name->valuestring : "NULL"),
          (json_id != NULL && json_id->valuestring != NULL ? json_id->valuestring : "NULL"),
          (json_mac != NULL && json_mac->valuestring != NULL ? json_mac->valuestring : "NULL"),
          (json_name != NULL && json_ip->valuestring != NULL ? json_ip->valuestring : "NULL") );

        if (  json_name != NULL && json_name->valuestring != NULL && 
              json_id != NULL && json_id->valuestring != NULL && 
              json_mac != NULL && json_mac->valuestring != NULL && 
              json_ip != NULL && json_ip->valuestring != NULL) {

          if (s_httpDiscoverActive && s_httpDiscoverCount < HTTP_DISCOVER_MAX) {
            /* Deduplicate by MAC - devices may respond multiple times to the broadcast */
            bool bDuplicate = false;
            for (byte j = 0; j < s_httpDiscoverCount; j++) {
              if (strcmp(s_httpDiscoverMac[j], json_mac->valuestring) == 0) {
                bDuplicate = true;
                break;
              }
            }
            if (!bDuplicate) {
              int idx = s_httpDiscoverCount;
              snprintf(s_httpDiscoverName[idx], HTTP_DISCOVER_STR, "%s", json_name->valuestring);
              snprintf(s_httpDiscoverId[idx], HTTP_DISCOVER_STR, "%s", json_id->valuestring);
              snprintf(s_httpDiscoverMac[idx], HTTP_DISCOVER_STR, "%s", json_mac->valuestring);
              snprintf(s_httpDiscoverIp[idx], HTTP_DISCOVER_STR, "%s", json_ip->valuestring);
              s_httpDiscoverCount++;
            }
          }

#if defined(ENABLE_RELAY)
          // First find the AmbiBack values
          int iFoundDeviceID = DEVICE_NOT_FOUND;
          byte bLastNum = global::GetAmbibackDeviceCount();
          
          if (bLastNum > 0) {
            for(byte i = 0; i < bLastNum && i < AMBIBACK_MAX_DEVICES; i++) {
              if ( strcmp(json_mac->valuestring, global::GetAmbibackDeviceMac(i) ) == 0) {
                iFoundDeviceID = i;
                break;
              }
            }
          }

          if (iFoundDeviceID == DEVICE_NOT_FOUND) {
            if (bLastNum < AMBIBACK_MAX_DEVICES) {
              global::SetAmbibackDeviceName(bLastNum, json_name->valuestring);
              global::SetAmbibackDeviceID(bLastNum, json_id->valuestring);
              global::SetAmbibackDeviceMac(bLastNum, json_mac->valuestring);
              global::SetAmbibackDeviceIP(bLastNum, json_ip->valuestring);
              
              // Save incremented count
              global::SetAmbibackDeviceCount(byte (bLastNum+1));
            } else {
              global::errprintf_P(PSTR("UdpServer: ERROR REACHED AMBIBACK_MAX_DEVICES LIMIT of '%d', not adding AmbiBack Device '%s'\n"), bLastNum, json_name->valuestring);
            }
          } else {
            // update name and ip
            global::SetAmbibackDeviceName(iFoundDeviceID, json_name->valuestring);
            global::SetAmbibackDeviceIP(iFoundDeviceID, json_ip->valuestring);
            
            _DBGPRINTF(PSTR("UdpServer: Discovered AmbiBack Device '%s' with MAC '%s' already known (%d,%d)!\n"), global::GetAmbibackDeviceName(iFoundDeviceID), global::GetAmbibackDeviceMac(iFoundDeviceID), bLastNum, iFoundDeviceID);
          }
#endif
#if defined(ENABLE_WIFI_AP)
          if (strstr(json_ip->valuestring, "192.168.4.") != NULL) {
            // Found internal IP - RELAY the UDP Packet
            _DBGPRINTF(PSTR("UdpServer: RELAY BROADCAST PACKET: %.*s\n"), (packet.length() < 924 ? packet.length() : 924), packet.data());
            UdpServer::m_pUdpServer->GetAsyncUdp()->broadcastTo((uint8_t*) packet.data(), packet.length(), UdpServer::m_pUdpServer->GetPort());
          }
#endif

          // --- Check for broadcast from connected client
          if ( global::GetClientConnectedAddr() != 0 && strcmp(strIpRemote.c_str(), global::GetClientConnectedIpStr()) == 0) {
            cJSON *json_status = cJSON_GetObjectItemCaseSensitive(json, "LedStatus");
            if (json_status != NULL && json_status->valuestring != NULL) {
              int iLedMode = atoi(json_status->valuestring);
              if (iLedMode == LED_MODE_OFF) {
                _DBGPRINTF(PSTR("UdpServer: Client %s changed to LED_MODE_OFF, Disconnect Client\n"), 
                  strIpRemote.c_str());
                
                // Client disconnect!
                global::SetClientConnected(false);
                global::SetClientConnectedIpStr(NULL);

                // Set LED Mode OFF
                global::m_pLedHelper->SetChangeLedMode(LED_MODE_OFF);

                /* Start LED fadeOut */
                global::m_pLedHelper->EnableTaskFadeOut();
              }
            }
          }
          // ---
        } else {
            global::errprintf_P(PSTR("UdpServer: ERROR, not all necessary properties found, device firmware properly to old\n"));
        }
        
        // Free JSON
        cJSON_Delete(json);
      }
    } else if ( strncmp_P((char*) pData, BROADCAST_DISCOVER_MSG, stSize) == 0 ) {       // Check for HELLO Str
      // Discover string detected, send out HELLO 
      _DBGPRINTF(PSTR("UdpServer: Handling BROADCAST_DISCOVER_MSG from %s:%d\n"), remoteClient.toString().c_str(), packet.remotePort() );

      char szBroadcast[512]; 
      size_t stLen = 512; 
      stLen = global::GetDeviceInfo(&szBroadcast[0], stLen); 

      // Send broadcast out        
      _DBGPRINTF(PSTR("UdpServer: Sending BROADCAST answer: %s\n"), szBroadcast );
      UdpServer::m_pUdpServer->GetAsyncUdp()->broadcastTo((uint8_t*) szBroadcast, stLen, UdpServer::m_pUdpServer->GetPort());

      // Send direct UDP
      bSendSuccess = UdpServer::m_pUdpServer->SendUdpMessage((uint8_t*) szBroadcast, stLen, remoteClient, UdpServer::m_pUdpServer->GetPort());
      if (bSendSuccess) {
        _DBGPRINTF(PSTR("UdpServer: UDP BROADCAST answer to %s send SUCCESS!\n"), remoteClient.toString() );
      } else {
        _ERRPRINTF(PSTR("LEDHelper:RelayLedData: UDP BROADCAST answer %s datalen %d send FAIL with Status=%#08x(%s) !\n"), 
          remoteClient.toString(), 
          stLen,
          UdpServer::m_pUdpServer->GetLastErr(), 
          esp_err_to_name(UdpServer::m_pUdpServer->GetLastErr()) );
      }      
    } else {
      pData = packet.data(); // Make sure we start from beginning
      char szDebug[513]; 
      char* szDebugptr = &szDebug[0];
      for (int i = 0; i < 255 && i < stSize; i++) {
         szDebugptr += sprintf(szDebugptr, "%02X", (unsigned char)*pData++);
      }
      *szDebugptr  = '\0';
      // Unexpected message, discard
      _DBGPRINTF(PSTR("UdpServer: Unexpected Data (Len %d) from %s:%d: HEX:%s\n"), stSize, remoteClient.toString().c_str(), packet.remotePort(), szDebug );
    }
  }

}

size_t UdpServer::RunHttpDiscover(char* pszOut, size_t stMax) {
  if (!m_pUdpServer || !pszOut || stMax < 32) return 0;
  s_httpDiscoverActive = true;
  s_httpDiscoverCount = 0;

  char szMsg[NET_BROADCAST_SIZE];
  size_t stLen = snprintf_P(szMsg, sizeof(szMsg), PSTR("%s\n"), BROADCAST_DISCOVER_MSG);
  m_pUdpServer->SendUdpBroadcast((byte*)szMsg, stLen);

  vTaskDelay(pdMS_TO_TICKS(2500));

  s_httpDiscoverActive = false;

  size_t pos = 0;
  pos += snprintf_P(pszOut + pos, stMax - pos, PSTR("["));
  for (byte i = 0; i < s_httpDiscoverCount && pos < stMax - 64; i++) {
    if (i > 0) pos += snprintf_P(pszOut + pos, stMax - pos, PSTR(","));
    pos += snprintf_P(pszOut + pos, stMax - pos, PSTR("{\"DeviceName\":\"%s\",\"DeviceID\":\"%s\",\"DeviceMAC\":\"%s\",\"DeviceIP\":\"%s\"}"),
      s_httpDiscoverName[i], s_httpDiscoverId[i], s_httpDiscoverMac[i], s_httpDiscoverIp[i]);
  }
  pos += snprintf_P(pszOut + pos, stMax - pos, PSTR("]"));
  return pos;
}
