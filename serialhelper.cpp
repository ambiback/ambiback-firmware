#include "globalincludes.h"
#if defined(ENABLE_SERIAL_INPUT)
# include "serialhelper.h"
# if defined(ENABLE_IMPROV_SERIAL)
#   include "improvserial.h"
# endif
# if defined(CONFIG_BT_ENABLED)
#   if !defined(DISABLE_BLUETOOTH)
#     include "BluetoothSerial.h"
      BluetoothSerial SerialBT;
#   endif
# endif

// Static helpers

SerialHelper::SerialHelper(){
  m_bInit = false;

  m_lLastUpdate = 0;
  /* Set last received serial to now */
  m_lLastSerialTime = millis();
  m_lTimeSerialCheck = 0;

# if defined(CONFIG_BT_ENABLED)
#   if !defined(DISABLE_BLUETOOTH)
  SerialBT.begin("AmbiBackBT");
#   endif
# endif
  

  m_tzBufLen = 0; 
}

void SerialHelper::checkSerial() {
  // _DBGPRINTF_ULTRA(PSTR("SerialHelper: CheckSerial ENTER\n"));

  /* --- Handle Serial Input here! --- */
  if (Serial.available() != 0) {
    // Process all Serial queued data
    int iByte = 0;
    unsigned long timeCmdData = millis(); 
    char szCmd[SERIAL_CMD_MAXLENGTH];
    size_t iIdx = 0; 
    bool bFound = false; 
    
    do {
      iByte = Serial.read();
      _DBGPRINTF_ULTRA(PSTR("SerialHelper:CheckSerial: Char 0x%02X\n"), (byte) iByte);
      
#if defined(ENABLE_IMPROV_SERIAL)
      // Improv packet in progress, or a byte outside a "%" command: Improv gets it
      if (iByte >= 0 && (iByte != '%' || ImprovSerial::IsReceiving())) {
        ImprovSerial::FeedByte((uint8_t) iByte);
      } else
#endif
      if (iByte == '%') {
        // terminate string first!
        szCmd[0] = '\0'; 
        
        // Set to invalid byte!
        do {
            // Read Byte in Loop with Timeout!
            iByte = SERIAL_INVALIDBYTE;
            while(  iByte <= SERIAL_INVALIDBYTE &&
                  (millis() - timeCmdData) < SERIAL_CMD_TIMEOUT) {
            iByte = Serial.read(); // try again
            _yield(); // Process other tasks
          }

          if (iByte != SERIAL_INVALIDBYTE) {
            // Check Command Byte!
            _DBGPRINTF_ULTRA(PSTR("SerialHelper:CheckSerial: Char Cmd 0x%02X\n"), (byte) iByte);
            if (  iByte == 0x04 /* EOT */ || 
                  iByte == ':' || 
                  iByte == ' ' || 
                  iByte == '\0') {
              // Command finished
              bFound = true; 
            } else {
              // Copy char!
              szCmd[iIdx++] = (uint8_t)iByte;
            }
          } else {
            // Break Loop!
            break;
          }
        } while(!bFound && iIdx < SERIAL_CMD_MAXLENGTH); // Abort if no more data avail or buffer size reached!
        szCmd[iIdx] = '\0'; //terminate command

        // Output DEBUG
        if (global::m_bUltraDebugEnabled) { 
          _DBGPRINTF(PSTR("SerialHelper:CheckSerial: Command read '%s/%d'\n"), szCmd, iIdx); 
        }
        
        // Parse the command
        if ( strcmp(szCmd, "INFO") == 0) {
          this->commandInfo();
        } else if ( strcmp(szCmd, "REBOOT") == 0) {
          // Perform Software Reset of device
          global::RebootController();
          _delay(1000);
        } else if ( strcmp(szCmd, "FACTORYDEFAULT") == 0) {
          /* WE DO A Factory reset! */ 
          global::m_pEepromHelper->FactoryReset(); 
          // Perform Software Reset of device
          global::RebootController();
          _delay(1000);
        } else if ( strcmp(szCmd, "STARTLEDDATA") == 0) {
          // Flush existing buffer
          Serial.flush(); 
          if (global::GetSerialEnabled()) {
            global::SetSerialEnabled(false);
            Serial.updateBaudRate(MAX_SERIAL_RATE);
            global::SetSerialEnabled(true);
            global::SetSerialConnected(true);
          } else {
            // Begin default Serial rate
            Serial.updateBaudRate(MAX_SERIAL_RATE);
          }
        } else if ( strcmp(szCmd, "ENDLEDDATA") == 0) {
          // Flush existing buffer
          Serial.flush(); 
          if (global::GetSerialEnabled()) {
            global::SetSerialEnabled(false);
            Serial.updateBaudRate(DEFAULT_SERIAL_RATE);// Begin default Serial rate
            global::SetSerialEnabled(true);
            global::SetSerialConnected(false); 
          } else {
            // Begin default Serial rate
            Serial.updateBaudRate(DEFAULT_SERIAL_RATE);// Begin default Serial rate
          }
        } else if ( strcmp(szCmd, "LEDDATA") == 0) {
          // --- HANDLE LEDDATA REQUEST
          byte bBuf[SERIAL_LEDDATA_MAXLENGTH];
          unsigned long timeLedData = millis(); 
          int iExpPktLen = -1;
          iIdx = 0;

          do {
            // Read Byte in Loop with Timeout!
            iByte = SERIAL_INVALIDBYTE;
            
            while(  iByte <= SERIAL_INVALIDBYTE &&
                    (millis() - timeLedData) < SERIAL_LEDDATA_TIMEOUT) {
              iByte = Serial.read(); // try again
              _yield(); // Process other tasks
            }

            // Process if valid byte
            if (iByte != SERIAL_INVALIDBYTE) {
              /* Calc length first time */
              if (iIdx == 0) {
                if (iByte != 0) {
                  iExpPktLen = ((byte)iByte * SERIAL_CHUNKSIZE) + SERIAL_HEADERSIZE /* + Header */;
                  bBuf[iIdx++] = (byte)iByte; // Copy byte
                }
              } else {
                bBuf[iIdx++] = (byte)iByte; // Copy byte
              }
            } else {
              /* Flush all existing buffer */
              this->FlushSerialData();
            }
          } while(iByte != SERIAL_INVALIDBYTE && 
                  (millis() - timeLedData) < SERIAL_LEDDATA_TIMEOUT && 
                  (iExpPktLen == -1 || (iIdx) < iExpPktLen) && 
                  (iIdx) < SERIAL_LEDDATA_MAXLENGTH); // Abort if no more data avail or buffer size reached!
            
          if (iIdx != iExpPktLen) {
            _DBGPRINTF(PSTR("SerialHelper:CheckSerial: LedData read failed expected %d bytes but received %d bytes\n"), iExpPktLen, (iIdx));
          }

          // Debug every second!
          if (global::m_bUltraDebugEnabled) { 
            _DBGPRINTF(PSTR("SerialHelper:CheckSerial: LedData expected %d bytes, read %d bytes (First 0x%02X, Last 0x%02X), Time %d process now\n"), 
              iExpPktLen, iIdx, bBuf[0], 
              (iIdx > 0 ? bBuf[iIdx-1] : -1), 
              (millis() - timeLedData));
          }

          // --- Check LED Data!
          if ((iIdx) != iExpPktLen) {
              global::errprintf_P(PSTR("SerialHelper:CheckSerial: LedData did not receive expected dasta length, DISCARD - Received: %d Expected: %d\n"), (iIdx), iExpPktLen);
          } else {
            // Process LED Data
            if (this->processLEDData(&bBuf[0], iIdx, iExpPktLen) == false) {
              /* Flush all existing buffer */
              this->FlushSerialData();
            }
          }
          // ---
          
          // --- Serial Timer Check!
          if (m_lTimeSerialCheck == 0) {
            m_lTimeSerialCheck = millis() + 1000;
          } else {
            /* Handle FPS counting after 1000ms period of time */
            if (millis() > m_lTimeSerialCheck ) {
              global::CalcFramesCtr();
              m_lTimeSerialCheck = millis() + 1000;
            }
          }  
          // --- 
        // --- 
        } else if ( strcmp(szCmd, "WIFI") == 0) {
          // --- HANDLE WIFI REQUEST
          unsigned long timeData = millis(); 
          char szSSID[WLAN_SSID_MAX];
          char szKEY[WLAN_KEY_MAX];
          szSSID[0] = '\0'; // terminate string
          szKEY[0] = '\0'; // terminate string
          
          // Get SSID
          bFound = false;
          iIdx = 0;

          do {
            // Read Byte in Loop with Timeout!
            iByte = SERIAL_INVALIDBYTE;
            
            while(  iByte == SERIAL_INVALIDBYTE &&
                    (millis() - timeData) < SERIAL_LEDDATA_TIMEOUT) {
              iByte = Serial.read(); // try again
              _yield(); // Process other tasks
            }

            // Process if valid byte
            if (iByte == '\0') {
              bFound = true;
            } else if (iByte != SERIAL_INVALIDBYTE && iByte > 0x1F) {
              // Copy char!
              szSSID[iIdx++] = (uint8_t)iByte;
            }
          }  while(iByte != SERIAL_INVALIDBYTE && 
                  (millis() - timeData) < SERIAL_LEDDATA_TIMEOUT && 
                  !bFound && 
                  iIdx < WLAN_SSID_MAX); // Abort if no more data avail or buffer size reached!
          szSSID[iIdx] = '\0'; //terminate SSID          

          // Get KEY
          bFound = false;
          iIdx = 0;

          do {
            // Read Byte in Loop with Timeout!
            iByte = SERIAL_INVALIDBYTE;
            
            while(  iByte == SERIAL_INVALIDBYTE &&
                    (millis() - timeData) < SERIAL_LEDDATA_TIMEOUT) {
              iByte = Serial.read(); // try again
              _yield(); // Process other tasks
            }

            // Process if valid byte
            if (iByte == '\0') {
              bFound = true;
            } else if (iByte != SERIAL_INVALIDBYTE && iByte > 0x1F) {
              // Copy char!
              szKEY[iIdx++] = (uint8_t)iByte;
            }
          }  while(iByte != SERIAL_INVALIDBYTE && 
                  (millis() - timeData) < SERIAL_LEDDATA_TIMEOUT && 
                  !bFound && 
                  iIdx < WLAN_KEY_MAX); // Abort if no more data avail or buffer size reached!
          szKEY[iIdx] = '\0'; //terminate SSID

          // Try WIFI Connect
          this->commandSetWifi((char*) &szSSID[0],(char*) &szKEY[0]);
        } else if ( strcmp(szCmd, "RESETWIFI") == 0) {
          // Reset Wifi
          this->commandResetWifi();
        } else if ( strcmp(szCmd, "LEDMODE") == 0) {
          // --- HANDLE LEDMODE REQUEST
          unsigned long timeData = millis(); 
          char szMODE[INT_VARIABLE_MAX];
          szMODE[0] = '\0'; // terminate string
          
          // Get MODE Num
          bFound = false;
          iIdx = 0;

          do {
            // Read Byte in Loop with Timeout!
            iByte = SERIAL_INVALIDBYTE;
            
            while(  iByte == SERIAL_INVALIDBYTE &&
                    (millis() - timeData) < SERIAL_LEDDATA_TIMEOUT) {
              iByte = Serial.read(); // try again
              _yield(); // Process other tasks
            }

            // Process if valid byte
            if (iByte == '\0') {
              bFound = true;
            } else if (iByte != SERIAL_INVALIDBYTE && iByte > 0x1F) {
              // Copy char!
              szMODE[iIdx++] = (uint8_t)iByte;
            }
          }  while(iByte != SERIAL_INVALIDBYTE && 
                  (millis() - timeData) < SERIAL_LEDDATA_TIMEOUT && 
                  !bFound && 
                  iIdx < INT_VARIABLE_MAX); // Abort if no more data avail or buffer size reached!
          szMODE[iIdx] = '\0'; //terminate SSID          

          if (strlen(szMODE) > 0) {
            int iNewMode = atoi(szMODE);
            if (iNewMode == LED_MODE_DEMO){
              global::m_pLedHelper->SetSetupVertical((byte) 47, false);
              global::m_pLedHelper->SetSetupHorizontal((byte) 80, false);
              global::m_pLedHelper->SetSetupDirection((byte) Right_Cclock_bottom_right_up, false);
    
              // Temporary overwrite FRAME Settings
              global::m_pLedHelper->SetFrameSmoothing(1, false);
              global::m_pLedHelper->SetFrameLength(DEFAULT_FRAME_LENGTH, false);
              global::m_pLedHelper->SetDelayLength(DEFAULT_DELAY_LENGTH, false);
            }
            global::m_pLedHelper->SetChangeLedMode((byte)iNewMode);

            // Output DEBUG
            _DBGPRINTF(PSTR("SerialHelper:CheckSerial: SERIAL_MODE_LEDMODE set to MODE '%s'\n"), szMODE);
          } else {
            // Output ERROR
            global::errprintf_P(PSTR("SerialHelper:CheckSerial: SERIAL_MODE_LEDMODE failed to set LEDMODE\n"));
          }
# if defined(ENABLE_RELAY)
        } else if ( strcmp(szCmd, "SETRELAY") == 0) {
          // --- HANDLE RELAY REQUEST
          unsigned long timeData = millis(); 
          char szRELAY[AMBIBACK_MAX_IDLEN];
          szRELAY[0] = '\0'; // terminate string
          
          // Get RELAY ID
          bFound = false;
          iIdx = 0;

          do {
            // Read Byte in Loop with Timeout!
            iByte = SERIAL_INVALIDBYTE;
            
            while(  iByte == SERIAL_INVALIDBYTE &&
                    (millis() - timeData) < SERIAL_LEDDATA_TIMEOUT) {
              iByte = Serial.read(); // try again
              _yield(); // Process other tasks
            }

            // Process if valid byte
            if (iByte == '\0') {
              bFound = true;
            } else if (iByte != SERIAL_INVALIDBYTE && iByte > 0x1F) {
              // Copy char!
              szRELAY[iIdx++] = (uint8_t)iByte;
            }
          }  while(iByte != SERIAL_INVALIDBYTE && 
                  (millis() - timeData) < SERIAL_LEDDATA_TIMEOUT && 
                  !bFound && 
                  iIdx < AMBIBACK_MAX_IDLEN); // Abort if no more data avail or buffer size reached!
          szRELAY[iIdx] = '\0'; //terminate SSID          

          if (strlen(szRELAY) > 0) {
            global::SetAmbibackSelectedDevice((char*) szRELAY);
            global::SetAmbibackRelayModeDirect(1);
            global::SetAmbibackRelayMode(RELAY_UDP);
            // Output DEBUG
            _DBGPRINTF(PSTR("SerialHelper:CheckSerial: SERIAL_MODE_SETRELAY set to RELAYID '%s'\n"), szRELAY);
          } else {
            // Output ERROR
            global::errprintf_P(PSTR("SerialHelper:CheckSerial: SERIAL_MODE_SETRELAY failed to set RELAYID\n"));
          }
# endif
        } else {
          if (strlen(szCmd) > 0) {
            _DBGPRINTF(PSTR("SerialHelper:CheckSerial: UNKNOWN COMMAND '%s' \n"), szCmd);
            this->FlushSerialData();
          }
        }
      }          

      _yield(); // Process other tasks
    } while(iByte >= 0); 
  }
  /* --- --- --- */
}

bool SerialHelper::processLEDData(byte* pbBuf, int iIdx, int iExpPktLen) {
  byte bLeds = pbBuf[0];
  byte bStartIndex = pbBuf[1]; // TODO HANDLE STARTINDEX
  byte bCrcCheck = pbBuf[2];
  byte bCrc8 = pbBuf[3];
  int iDataLen = iExpPktLen-SERIAL_HEADERSIZE; 
  
  // Generate our own checksum
  byte bCrcCompare = (byte) global::crc_8( pbBuf+SERIAL_HEADERSIZE, iDataLen );
  _DBGPRINTF_ULTRA(PSTR("SerialHelper:CheckSerial: LedData Checksum Data=0x%02X Calced=0x%02X\n"), bCrc8, bCrcCompare );

  /* perform CrcCheck only if bCrcCheck is not NULL! */
  if (  bCrcCheck == 0x00 || 
      ( bCrcCheck == 0x01 && bCrc8 == bCrcCompare)
     ) {
    // Set Serial Client Connected!
    global::SetSerialConnected(true);
    /* Set Last Updated time! */
    global::m_pLedHelper->SetLastUpdate();
    
    /* Set LED to VIDEO */
    global::m_pLedHelper->SetLedMode(LED_MODE_VIDEO); 
    /* Set current LED Count */
    global::m_pLedHelper->SetCurrentLedCount((uint16_t)bLeds); // network sends actual count 0-255; we store count-1 

    /* Process Color Bytes */
    int iColorIndex = 0; 
    for(size_t st=(SERIAL_HEADERSIZE); (st < iExpPktLen); st+=SERIAL_CHUNKSIZE) {
      /* Set Color LED Code   | White, Red, Green, Blue */
      global::m_pLedHelper->SetLedColorByIdx(iColorIndex++, pbBuf[st], pbBuf[st+1], pbBuf[st+2], pbBuf[st+3]); 
    }

    /* ZERO unconfigured LEDs */
    global::m_pLedHelper->ZeroConfiguredLeds();

    /* Process new LED Data */
    global::m_pLedHelper->SetIsNewLedData(true); 

    // Increment Counter
    global::IncrementCurrentFramesCtr();
    return true;
  } else {
    global::errprintf_P(PSTR("SerialHelper:CheckSerial: LedData received but not ok, checksum error: Leds=0x%02X StartIndex=0x%02X CrcCheck=0x%02X Data=0x%02X Calced=0x%02X Received: %d Expected: %d\n"), 
      bLeds, bStartIndex, bCrcCheck, bCrc8, bCrcCompare, iIdx, iExpPktLen);
    
    // Increment Counter
    global::IncrementCurrentDroppedCtr();
    return false;
  }
}

void SerialHelper::FlushSerialData() {
  unsigned long timeLedData = millis();
  /* Flush all existing buffer */
  Serial.flush();
  while(  Serial.available() &&
          (millis() - timeLedData) < SERIAL_LEDDATA_TIMEOUT) {
    Serial.read();
  }
}

void SerialHelper::commandInfo() {
  // --- HANDLE INFO REQUEST
  char szBroadcast[512];
  size_t stLen = 512;
  stLen = global::GetDeviceInfo(&szBroadcast[0], stLen);
  if (global::GetSerialEnabled()) {
    global::SetSerialEnabled(false);
    Serial.printf(PSTR("%%info: %s"), szBroadcast);
    global::SetSerialEnabled(true);
  } else {
    Serial.printf(PSTR("%%info: %s"), szBroadcast);
  }
  _yield(); // Process other tasks
  // --- 
}

void SerialHelper::commandResetWifi() {
  // Reset WIFI Settings
  global::SetWifiSSID(NULL); 
  global::SetWifiPassword(NULL); 
  
  // Perform Software Reset of device
  global::RebootController();
  _delay(1000);
}

void SerialHelper::commandSetWifi(char* pszSSID, char* pszKEY) {
  // Output DEBUG
  _DBGPRINTF(PSTR("SerialHelper:commandSetWifi: SERIAL_MODE_SETWIFI found WifiSSID '%s' with Key '%s' \n"), pszSSID, pszKEY);
  if (strlen(pszSSID) > 0 && strlen(pszKEY) ) {
    // Try to connect to NEW WIFI
    if (global::ConnectWifiConfigured(pszSSID, pszKEY, false)) {
      // Save Settings to EEPROM no done in GLOBAL!
      global::SetWifiSSID((char*) pszSSID); 
      global::SetWifiPassword((char*) pszKEY); 
      
      // Force Write Changes to EEPROM
      global::m_pEepromHelper->CommitChanges();
      _yield(); 
      
      // Perform Software Reset of device
      global::RebootController();
      _delay(1000);
    } else {
      global::errprintf_P(PSTR("SerialHelper:commandSetWifi: SERIAL_MODE_SETWIFI failed to connect to WifiSSID '%s' with Key '%s' \n"), pszSSID, pszKEY);
    }
  }
}

void SerialHelper::SetLastUpdate() {
  m_lLastUpdate = millis(); 
  global::m_pLedHelper->SetLastUpdate();
}


// ---
#endif
