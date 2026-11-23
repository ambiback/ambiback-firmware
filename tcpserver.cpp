// Include all needed files
#include "globalincludes.h"
#if defined(ENABLE_TCP)
# include "tcpserver.h"

# define TCP_TIMEOUT 60000

extern "C" {
# include "lwip/pbuf.h"
};
void handleNewClient(void* arg, AsyncClient* client);

void handleData(void* arg, AsyncClient* client, void *data, size_t len);
void handleError(void* arg, AsyncClient* client, int8_t error);
void handleDisconnect(void* arg, AsyncClient* client);
void handleTimeOut(void* arg, AsyncClient* client, uint32_t time);
byte* handlePacket(TcpServer* pThis, byte* pData,  AsyncClient* client); 

// a list to hold all clients
static std::vector<AsyncClient*> clients; 

// Static helpers
unsigned long timeTCPCheck = 0; 

// Universal Pointer to UdpServer Obj
TcpServer* TcpServer::m_pTcpServer = NULL;

TcpServer::TcpServer(int nPort){
  m_bInit = false;
  m_nPort = nPort;
  pTcpServer = new AsyncServer(m_nPort);
  TcpServer::m_pTcpServer = this;
  m_lLastUpdate = 0;
  
  /* Set last PacketTime to now */
  m_lLastPacketTime = millis(); 

#if TCP_MSS < MAX_PAKET_LENGTH
  /* terminate tcp buffer */
  m_bBuf[0] = 0;
#endif
  m_tzBufLen = 0; 
  m_asyncCurClient = NULL; 
}

void TcpServer::StartServer() {
  if (!m_bInit) 
    this->InitServer(); 
    
  _DBGPRINTF(PSTR("TcpServer: Successfully started TCP Server on Port: %d\n"), m_nPort);
}

void TcpServer::InitServer() {
  // Starts TcpServer
  pTcpServer->onClient(&handleNewClient, this);  
  pTcpServer->begin();
  m_bInit = true; 
}

void TcpServer::DisconnectClient() {
  global::SetClientConnected(false); 
  global::SetClientConnectedIpStr(NULL);
  m_asyncCurClient = NULL;
}

void TcpServer::setAsyncCurClient(AsyncClient* newClient) { 
  if (m_asyncCurClient != NULL) {
    m_asyncCurClient->close(true); 
    if (!clients.empty())
      clients.pop_back(); 
  }
  
  // Copy pointer to tcp client
  m_asyncCurClient = newClient; 
  _DBGPRINTF(PSTR("TcpServer: setCurClient=%s\n"), newClient->remoteIP().toString().c_str());

#ifdef ENABLE_SSD1306
  global::oledDrawStringQueue(String(PSTR("New Client connected:")), newClient->remoteIP().toString(), "");
#endif
} 

void TcpServer::SetLastUpdate() {
  m_lLastUpdate = millis(); 
  global::m_pLedHelper->SetLastUpdate();
}

// --- Static Callbacks
/* server events */
void handleNewClient(void* arg, AsyncClient* client) {
  IPAddress remoteClient = client->remoteIP();
  _DBGPRINTF(PSTR("TcpServer: handleNewClient: new client has been connected to server, ip: %s\n"), remoteClient.toString().c_str());
  TcpServer* pThis = (TcpServer*) arg; 

  /* Make sure only ONE TCL Client can be connected */
  if (global::GetClientConnected()) {
    if ((millis()-TCP_TIMEOUT) > pThis->GetLastUpdate() ) {
      global::errprintf_P(PSTR("TcpServer: handleNewClient: client '%s' is already connected to the server, but timedout now (%d)!\n"), global::GetClientConnectedIpStr(), TCP_TIMEOUT);
    } else {
      global::errprintf_P(PSTR("TcpServer: handleNewClient: ERROR, client '%s' is already connected to the server\n"), global::GetClientConnectedIpStr());
      client->stop();
      return; 
    }
  } 

  // Save reference
  pThis->setAsyncCurClient(client);
  global::SetClientConnected(true);
  global::SetClientConnectedIpStr(&remoteClient);
  
  // Set Last Updated
  pThis->SetLastUpdate(); 
    
  // Add to clientslist
  clients.push_back(client);
    
  // register events
  client->onData(&handleData, pThis);
  client->onError(&handleError, pThis);
  client->onDisconnect(&handleDisconnect, pThis);
  client->onTimeout(&handleTimeOut, pThis);

  // Reset Counters
  global::SetCurrentPkts(0);
  global::SetCurrentPktsDropped(0);
  global::SetCurrentFramesCtr(0);
  global::SetCurrentDroppedCtr(0);
  
  _DBGPRINTF(PSTR("TcpServer: handleNewClient: callbacks set for client ip: %s\n"), remoteClient.toString().c_str());
}

byte* handlePacket(TcpServer* pThis, byte* pData,  AsyncClient* client) {
  bool bSuccess = false; 
  bool bAmbiBackHead = false; 
  char reply[CLIENT_REPLY_SIZE];
  IPAddress remoteClient = client->remoteIP();

  if (pData == NULL) {
    _DBGPRINTF(PSTR("TcpServer: handlePacket INVALID DATA(NULL)\n"));
    return NULL; 
  }

  /* Data finished, process paket,   
   * Process on correct packet length */
  if (pThis->m_tzBufLen >= NET_HEADLEN /*&& pThis->m_tzBufLen % NET_CHUNKSIZE == 0*/ ) {
    size_t stStartByte = 0; 
    /* Search for BYTE Header */
    for(size_t st=0; (st < pThis->m_tzBufLen); st+=NET_CHUNKSIZE) {
      if (*(pData+st) == 0x00 && 
          *(pData+st+1) == 0xFF && 
          *(pData+st+2) == 0x13 && 
          *(pData+st+3) == 0x37 ) {
        // Found valid AmbiBack TCP Package
        stStartByte = st+NET_CHUNKSIZE; 
        bAmbiBackHead = true; 
        break;
      }
    }

    // Continue if there are enough bytes remaining
    if (pThis->m_tzBufLen > stStartByte+NET_HEADLEN) {
      // Try to process AmbiBack Header
      byte bLeds = *(pData+stStartByte); 
      byte bStartIndex = *(pData+stStartByte+1); // TODO HANDLE STARTINDEX
      byte bCrcCheck = *(pData+stStartByte+2); 
      byte bCrc8 = *(pData+stStartByte+3); 
      
      // Calc bytes left
      size_t stPacketLen = (bLeds * NET_CHUNKSIZE) + NET_CHUNKSIZE + (/* Add extra NET_CHUNKSIZE bytes */ bAmbiBackHead ? NET_CHUNKSIZE : 0); 
      size_t stBytesLeft = pThis->m_tzBufLen - stPacketLen;

      if (stPacketLen > pThis->m_tzBufLen) {
          _DBGPRINTF_ULTRA(PSTR("TcpServer: handlePacket Packet expected longer then data, wait for more %d/%d\n"), stPacketLen, pThis->m_tzBufLen);
          return NULL;         
      }
  
      // Generate our own checksum
      byte bCrcCompare = (byte) global::crc_8(pData+stStartByte+NET_CHUNKSIZE, stPacketLen-NET_CHUNKSIZE-(/* Remove extra NET_CHUNKSIZE bytes */bAmbiBackHead ? NET_CHUNKSIZE : 0) ); // pThis->m_tzBufLen-stStartByte
      _DBGPRINTF_ULTRA(PSTR("TcpServer: handlePacket process %d/%d, bCrc8=0x%x crcCompare=0x%x\n"), stPacketLen, pThis->m_tzBufLen, bCrc8, bCrcCompare );

      /* perform CrcCheck only if bCrcCheck is not NULL! */
      if (bCrcCheck == 0x00 || bCrc8 == bCrcCompare) {
        // LED Controller is ready for new DATA
#if false // defined(ESP32)
/*!global::m_pLedHelper->GetIsNewLedData()*/
        /* Only allow on running core */
        if ( /*!global::m_pLedHelper->getProcessingLedNextStep() && */
          xPortGetCoreID() == ARDUINO_RUNNING_CORE) {
#endif
          /* Set LED to VIDEO */
          global::m_pLedHelper->SetChangeLedMode(LED_MODE_VIDEO); 
          /* Set current LED Count */
          global::m_pLedHelper->SetCurrentLedCount((uint16_t)bLeds); // network sends actual count 0-255; we store count-1
      
          /* Process Color Bytes */
          int iColorIndex = 0; 
          for(size_t st=stStartByte+NET_CHUNKSIZE; (st < stPacketLen && st < pThis->m_tzBufLen); st+=NET_CHUNKSIZE) {
            /* Get Color Codes */  
            byte bColorWhite = *(pData+st);
            byte bColorRed = *(pData+st+1);
            byte bColorGreen = *(pData+st+2);
            byte bColorBlue = *(pData+st+3);

            /* Set Color LED Code */
            global::m_pLedHelper->SetLedColorByIdx(iColorIndex, bColorWhite, bColorRed, bColorGreen, bColorBlue); 
            iColorIndex++; 
          }
  
          /* ZERO unconfigured LEDs */
          global::m_pLedHelper->ZeroConfiguredLeds(); 
  
          /* Process new LED Data */
          global::m_pLedHelper->SetIsNewLedData(true); 

          // Increment Counter
          global::IncrementCurrentFramesCtr(); 
  
          /* Create TCP response */
          snprintf_P(reply, CLIENT_REPLY_SIZE, PSTR("SUCCESS: BufLen=%d, BytesLeft=%d, Leds=%d, StartIdx=%d, Crc=0x%x, CrcCompare=0x%x"), 
            pThis->m_tzBufLen, stBytesLeft, bLeds, bStartIndex, bCrc8, bCrcCompare);
  
          _DBGPRINTF_ULTRA(PSTR("TcpServer: handlePacket SUCCESS: BufLen=%d, BytesLeft=%d, Leds=%d, StartIdx=%d, Crc=0x%x, CrcCompare=0x%x\n"), 
              pThis->m_tzBufLen, stBytesLeft, bLeds, bStartIndex, bCrc8, bCrcCompare);

          // Valid Data, set Client Connected if not already!
          if (!global::GetClientConnected()) {
            global::SetClientConnected(true);
            global::SetClientConnectedIpStr(&remoteClient);
          }
            
          // Set Last Updated
          TcpServer::m_pTcpServer->SetLastUpdate(); 
          
#if false // defined(ESP32)
        } else {
          // LED Controller still busy with OLD data

          // Increment Counter
          global::IncrementCurrentDroppedCtr(); 

          /* Create TCP response */
          snprintf_P(reply, CLIENT_REPLY_SIZE, PSTR("DROPPED: BufLen=%d, BytesLeft=%d, Leds=%d, StartIdx=%d, Crc=0x%x, CrcCompare=0x%x"), 
            pThis->m_tzBufLen, stBytesLeft, bLeds, bStartIndex, bCrc8, bCrcCompare);
  
          _DBGPRINTF_ULTRA(PSTR("TcpServer: handlePacket DROPPED: BufLen=%d, BytesLeft=%d, Leds=%d, StartIdx=%d, Crc=0x%x, CrcCompare=0x%x\n"), 
              pThis->m_tzBufLen, stBytesLeft, bLeds, bStartIndex, bCrc8, bCrcCompare);
        }
#endif

        /* check if we have remaining data in the buffer  */
        if (stBytesLeft > 0) {
          _DBGPRINTF_ULTRA(PSTR("TcpServer: handlePacket remaining data %d\n"), stBytesLeft);

#if TCP_MSS < MAX_PAKET_LENGTH
          /* Move the remaining bytes to beginning of the buffer */
          memcpy(&pThis->m_bBuf[0], &pThis->m_bBuf[stPacketLen], stBytesLeft);
          pThis->m_tzBufLen = stBytesLeft;
          pData = &pThis->m_bBuf[0];
#else
          /* Move pointer forward */
          pData = pData+stPacketLen; 
          pThis->m_tzBufLen = stBytesLeft;
#endif
        } else {
          /* Reset Buffer */
          pThis->m_tzBufLen = 0;
          pData = NULL; 
        }
      } else {
        /* Create TCP response */
        snprintf_P(reply, CLIENT_REPLY_SIZE, PSTR("ERROR: CHECKSUM\n"));
        _DBGPRINTF_ULTRA(PSTR("TcpServer: handlePacket TCP Checksum (BufLen=%d, BytesLeft=%d, Leds=%d, StartIdx=%d, Crc=0x%x, CrcCompare=0x%x) FAILED, ignore packet!\n"), 
            pThis->m_tzBufLen, stBytesLeft, bLeds, bStartIndex, bCrc8, bCrcCompare );
        
        /* Reset Buffer */
        pThis->m_tzBufLen = 0;
        pData = NULL; 
      }
    } else {
      /* Create TCP response */
      snprintf_P(reply, CLIENT_REPLY_SIZE, PSTR("ERROR: INVALID Packet, too small #1\n"));
      _DBGPRINTF_ULTRA(PSTR("TcpServer: handlePacket TCP Packet length %d to small (Startbyte %d), ignore packet!\n"), pThis->m_tzBufLen, stStartByte);
      
      /* Reset Buffer */
//      pThis->m_tzBufLen = 0;
      pData = NULL; 
    }
  } else {
    snprintf_P(reply, CLIENT_REPLY_SIZE, PSTR("ERROR: INVALID Packet, too small #1\n"));
    _DBGPRINTF(PSTR("TcpServer: handlePacket TCP Packet length %d to small, ignore packet!\n"), pThis->m_tzBufLen);
    
    /* Create TCP response 
    snprintf_P(reply, CLIENT_REPLY_SIZE, PSTR("ERROR: INVALID PACKET LENGTH %d/%d"), pThis->m_tzBufLen, (pThis->m_tzBufLen % 4));
    _DBGPRINTF(PSTR("TcpServer: handlePacket INVALID PACKET LENGTH %d/%d\n"), pThis->m_tzBufLen, (pThis->m_tzBufLen % 4));

    //  Reset Buffer 
    pThis->m_tzBufLen = 0;
*/    
    pData = NULL; 
  }

    /* Output Debug */
  _DBGPRINTF_ULTRA(PSTR("TcpServer: handlePacket reply: %s \n"), reply);
  
  // reply to client
  if (client->space() > CLIENT_REPLY_SIZE && client->canSend()) {
    client->add(reply, strlen(reply));
    client->send();
  }

  // Return Data Pointer
  return pData; 
}

/* clients events */
void handleData(void* arg, AsyncClient* client, void *data, size_t len) {
  TcpServer* pThis = (TcpServer*) arg; 
  // Helper pointer
  byte* pData = NULL; 

  _DBGPRINTF_ULTRA(PSTR("TcpServer: handleData received from client %s, new bytes: %d, maxlength: %d, BufLen: %d\n"), 
      client->remoteIP().toString().c_str(), 
      len, MAX_PAKET_LENGTH, pThis->m_tzBufLen
      );
#if false // defined(ESP32)
  /* Only allow on running core */
  if (xPortGetCoreID() != ARDUINO_RUNNING_CORE) {
    // Increment Counter
    global::IncrementCurrentDroppedCtr(); 
    _DBGPRINTF_ULTRA(PSTR("TcpServer: handleData DROPPED: BufLen=%d, BytesLeft=%d\n"), 
        pThis->m_tzBufLen, len);

# if TCP_MSS < MAX_PAKET_LENGTH
    /* Reset Buffer */
    pThis->m_tzBufLen = 0;
# endif

    return; 
  }
#endif
#if TCP_MSS < MAX_PAKET_LENGTH
  size_t lenDataRemain = len; 

  // Check for sufficient buffer first!
  size_t lenCopyOnly = 0; // 
  if( (pThis->m_tzBufLen+lenDataRemain) > MAX_PAKET_LENGTH) {
    lenCopyOnly = MAX_PAKET_LENGTH - pThis->m_tzBufLen; 
    _DBGPRINTF_ULTRA(PSTR("TcpServer: handleData max packet size of %d/%d/%d exceeded, copy %d bytes and process buffer first\n"), MAX_PAKET_LENGTH, pThis->m_tzBufLen, len, lenCopyOnly);

    /* Copy to only whats possible to static buffer */
    memcpy(&pThis->m_bBuf[pThis->m_tzBufLen], (byte*)data, lenCopyOnly);
    pThis->m_tzBufLen += lenCopyOnly;
    pData = &pThis->m_bBuf[0];

    // Set Helper vars
    lenDataRemain -= lenCopyOnly;
  } else {
    _DBGPRINTF_ULTRA(PSTR("TcpServer: handleData max length: %d, BufLen: %d, NewLen: %d\n"), MAX_PAKET_LENGTH, pThis->m_tzBufLen, len);

    /* Copy to static buffer */
    memcpy(&pThis->m_bBuf[pThis->m_tzBufLen], (byte*)data, lenDataRemain);
    pThis->m_tzBufLen += lenDataRemain;
    pData = &pThis->m_bBuf[0];
  }
#else
  // Just copy len over
  pThis->m_tzBufLen = len;
  // Directly access data buffer
  pData = (byte*)data; 
#endif

  // Loop until buffer is processed!
  while( (pData = handlePacket(pThis, pData, client)) != NULL) {
    _DBGPRINTF_ULTRA(PSTR("TcpServer: handleData #2 processed buffer, remaining %d\n"), pThis->m_tzBufLen);
  }
#if TCP_MSS < MAX_PAKET_LENGTH
  // HANDLE remaining bytes  
  if (lenCopyOnly > 0) {
    global::errprintf_P(PSTR("TcpServer: handleData copy remaining bytes of %d \n"), lenDataRemain);

    /* Copy the remaining bytes to static buffer and reset it */
    memcpy(&pThis->m_bBuf[pThis->m_tzBufLen], (byte*)data+lenCopyOnly, lenDataRemain);
    pThis->m_tzBufLen = lenDataRemain;
  }
#endif
  if (timeTCPCheck == 0) {
    timeTCPCheck = millis() + 1000;
  } else {
    /* Handle FPS counting after 1000ms period of time */
    if (millis() > timeTCPCheck ) {
      global::CalcFramesCtr();
      timeTCPCheck = millis() + 1000;
    }
  }  
}
void handleError(void* arg, AsyncClient* client, int8_t error) {
  TcpServer* pThis = (TcpServer*) arg; 
  global::errprintf_P(PSTR("TcpServer: Connection error %s from client %s \n"), 
    client->errorToString(error), 
    client->remoteIP().toString().c_str());
}
void handleDisconnect(void* arg, AsyncClient* client) {
  TcpServer* pThis = (TcpServer*) arg; 
  _DBGPRINTF(PSTR("TcpServer: Client %s disconnected \n"), client->remoteIP().toString().c_str());

  /* Free Client Connection */
  pThis->DisconnectClient(); 
}
void handleTimeOut(void* arg, AsyncClient* client, uint32_t time) {
  TcpServer* pThis = (TcpServer*) arg; 
  global::errprintf_P(PSTR("TcpServer: Client ACK timeout ip: %s \n"), client->remoteIP().toString().c_str());
}
// ---
#endif
