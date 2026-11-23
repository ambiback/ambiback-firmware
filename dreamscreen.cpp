// Include all needed files
#include "globalincludes.h"
#include "dreamscreen.h"
#include "FastLED.h"    // For CRGB

#ifdef ENABLE_DREAMSCREEN

// --- Begin ENUM 
enum DsCommands
{
    WifiSSD = 0x0101,                   /* Found using WIFI Network capture */
    WifiKey = 0x0102,                   /* Found using WIFI Network capture */
    WifiMagic = 0x0103,                   /* Found using WIFI Network capture */
    DsName = 0x0107,
    GroupName = 0x0108,
    GroupNumber = 0x0109,
    CurrentState = 0x010A,              /* Current State R 0x01 0x0A Dump of all attributes Variable
                                            Read current state’ response: The payload received varies by what the device is and the
                                            version of firmware. However, the last index of the payload always contains the productId of the
                                            device, which you should use to determine the proper context of the payload. Currently
                                            DreamScreen can send up to 140 bytes; SideKick can send up to 62.
                                            Product ID Device
                                            0x01 DreamScreen HD
                                            0x02 DreamScreen 4K
                                            0x03 SideKick

                                            DreamScreen HD and 4K Current State Payload
                                            Index Size (Bytes) Attribute
                                            0-15 16 (UTF8) Name
                                            16-31 16 (UTF8) Group Name
                                            32 1 Group Number
                                            33 1 Mode
                                            34 1 Brightness
                                            40-42 3 Ambient Color
                                            62 1 Ambient Scene
                                            73 1 HDMI Input
                                            75-90 16 (UTF8) HDMI Input Name 1
                                            91-106 16 (UTF8) HDMI Input Name 2
                                            107-122 16 (UTF8) HDMI Input Name 3
                                            129 1 HDMI Active Channels

                                            SideKick Current State Payload
                                            Index Size (Bytes) Attribute
                                            0-15 16 (UTF8) Name
                                            16-31 16 (UTF8) Group Name
                                            32 1 Group Number
                                            33 1 Mode
                                            34 1 Brightness
                                            35-37 3 Ambient Color
                                            60 1 Ambient Scene
                                        */
    SubscriptionRequest = 0x010C,       /* subscription request 
                                            Unicast back to the command with a payload of 0x01.
                                        */
    HDMIInputName1 = 0x0123,
    HDMIInputName2 = 0x0124,
    HDMIInputName3 = 0x0125,
    HDMIActiveChannels = 0x012C,
    Mode = 0x0301,                      /*  0x00 - Idle Mode
                                            0x01 - Video Mode
                                            0x02 - Music Mode
                                            0x03 - Ambient Mode
                                            0x04 - Identity Mode
                                            0x05 - Direct Pixel Write
                                        */
    Brightness = 0x0302,
    AmbientColor = 0x0305,
    AmbientModeType = 0x0308,

    MusicModeColors = 0x030A,           // (0-2)(0-2)(0-2) 0-Red, 1 - Green 2 - Blue for Treble, Middle, Base
    MusicModeWeights = 0x030B,          // (0-100)(0-100)(0-100) - Treble, Middle, Bss
    AmbientScene = 0x030D,

    PixelVideoAddressing = 0x0315,      // Pixel Video Mode Addressing  0x0315 RW  (Start Pixel); (length) (RGB),(RGB),(RGB)........
    SubscriptionData = 0x0316,          /* Pixel Video Subscription Data 0x0316 RW  - 36 Bytes of DATA formatted as sectors 1 to 12 each being 24-bit rgb.
                                            7  6   5   4   3
                                            8              2
                                            9  10  11  12  1
                                        */
    App_Music_Data = 0x0322,            // 8 Bytes (LEFT Bass, Mid, Treble, Energy; RIGHT Bass, Mid, Treble, Energy)
    PixelMusicAddressing = 0x0322,      // Music Video Mode Addressing  0x0322 RW  (Start Pixel); (length) (RGB),(RGB),(RGB)........

    Temperature = 0x0329,               // read one byte celsius

    Advanced_Color_Boost = 0x032D,      // 0 - Disable, 1 - Low, 2 - Medium, 3 - High
    Advanced_CEC_Power_Detect = 0x032E, // 0 - Disable, 1 - Enable

    Install_SKU_Setup = 0x0340,         /*  0 - Classic
                                      1 - Mega
                                      2 - Xtreme
                                      3 - Flex/DIY 
                                        */
    Install_Flex_Setup = 0x0341,        /*  (005 - 045)(010-080) (Left Count)(Right Count)(Left Start)(Right Start)- Vertical/Horizontal LEDs
                                            Left/Right Start: (Based on Back of TV)
                                      0x01 - Right Cclock - bottom right up
                                      0x02 - Top Cclock - top right left
                                      0x03 - Left Cclock - top left down
                                      0x04 - Bottom Cclock - bottom left right
                                      0x05 - Right Clock - top right down
                                      0x06 - Top Clock - top left right
                                      0x07 - Left Clock - bottom left up
                                      0x08 - Bottom Clock - bottom right left 
                                        */
};
enum DsFlags {
    Receive = 0x30, /*BroadCast?!*/
    Read = 0x11,
    Write = 0x21,
};
enum DsModes {
    Sleep = 0x0,
    Video = 0x1,
    Music = 0x2,
    Ambient = 0x3,
};
enum DsHdmiInput {
    Input1 = 0x0,
    Input2 = 0x1,
    Input3 = 0x2,
};
enum DsDeviceType {
    DreamScreenHD = 0x01, 
    DreamScreen4K = 0x02,
    DreamScreen4KSolo = 0x07,
    DreamScreenSideKick = 0x03,
};
enum DsDeviceSkuSetup {
    Classic     = 0x00,
    Mega        = 0x01,
    Xtreme      = 0x02,
    FlexDIY     = 0x03,
};
// --- END Enum

// --- Static variables 
static uint8_t uartComm_crc8_table[] = {
    0x00 , 0x07 , 0x0E , 0x09 , 0x1C , 0x1B , 0x12 , 0x15 , 0x38 , 0x3F , 0x36 , 0x31 , 0x24 , 0x23 , 0x2A , 0x2D , 
    0x70 , 0x77 , 0x7E , 0x79 , 0x6C , 0x6B , 0x62 , 0x65 , 0x48 , 0x4F , 0x46 , 0x41 , 0x54 , 0x53 , 0x5A , 0x5D , 
    0xE0 , 0xE7 , 0xEE , 0xE9 , 0xFC , 0xFB , 0xF2 , 0xF5 , 0xD8 , 0xDF , 0xD6 , 0xD1 , 0xC4 , 0xC3 , 0xCA , 0xCD , 
    0x90 , 0x97 , 0x9E , 0x99 , 0x8C , 0x8B , 0x82 , 0x85 , 0xA8 , 0xAF , 0xA6 , 0xA1 , 0xB4 , 0xB3 , 0xBA , 0xBD , 
    0xC7 , 0xC0 , 0xC9 , 0xCE , 0xDB , 0xDC , 0xD5 , 0xD2 , 0xFF , 0xF8 , 0xF1 , 0xF6 , 0xE3 , 0xE4 , 0xED , 0xEA , 
    0xB7 , 0xB0 , 0xB9 , 0xBE , 0xAB , 0xAC , 0xA5 , 0xA2 , 0x8F , 0x88 , 0x81 , 0x86 , 0x93 , 0x94 , 0x9D , 0x9A , 
    0x27 , 0x20 , 0x29 , 0x2E , 0x3B , 0x3C , 0x35 , 0x32 , 0x1F , 0x18 , 0x11 , 0x16 , 0x03 , 0x04 , 0x0D , 0x0A , 
    0x57 , 0x50 , 0x59 , 0x5E , 0x4B , 0x4C , 0x45 , 0x42 , 0x6F , 0x68 , 0x61 , 0x66 , 0x73 , 0x74 , 0x7D , 0x7A , 
    0x89 , 0x8E , 0x87 , 0x80 , 0x95 , 0x92 , 0x9B , 0x9C , 0xB1 , 0xB6 , 0xBF , 0xB8 , 0xAD , 0xAA , 0xA3 , 0xA4 ,
    0xF9 , 0xFE , 0xF7 , 0xF0 , 0xE5 , 0xE2 , 0xEB , 0xEC , 0xC1 , 0xC6 , 0xCF , 0xC8 , 0xDD , 0xDA , 0xD3 , 0xD4 , 
    0x69 , 0x6E , 0x67 , 0x60 , 0x75 , 0x72 , 0x7B , 0x7C , 0x51 , 0x56 , 0x5F , 0x58 , 0x4D , 0x4A , 0x43 , 0x44 , 
    0x19 , 0x1E , 0x17 , 0x10 , 0x05 , 0x02 , 0x0B , 0x0C , 0x21 , 0x26 , 0x2F , 0x28 , 0x3D , 0x3A , 0x33 , 0x34 , 
    0x4E , 0x49 , 0x40 , 0x47 , 0x52 , 0x55 , 0x5C , 0x5B , 0x76 , 0x71 , 0x78 , 0x7F , 0x6A , 0x6D , 0x64 , 0x63 , 
    0x3E , 0x39 , 0x30 , 0x37 , 0x22 , 0x25 , 0x2C , 0x2B , 0x06 , 0x01 , 0x08 , 0x0F , 0x1A , 0x1D , 0x14 , 0x13 , 
    0xAE , 0xA9 , 0xA0 , 0xA7 , 0xB2 , 0xB5 , 0xBC , 0xBB , 0x96 , 0x91 , 0x98 , 0x9F , 0x8A , 0x8D , 0x84 , 0x83 , 
    0xDE , 0xD9 , 0xD0 , 0xD7 , 0xC2 , 0xC5 , 0xCC , 0xCB , 0xE6 , 0xE1 , 0xE8 , 0xEF , 0xFA , 0xFD , 0xF4 , 0xF3
};

static char m_dreamscreen_groups[DREAMSCREEN_MAX_GROUPS][DREAMSCREEN_MAX_STRING];
static byte m_dreamscreen_groupids[DREAMSCREEN_MAX_GROUPS];
static int m_dreamscreen_groups_count = 0;
// --- 

// Universal Pointer to UdpServer Obj
DreamScreen* DreamScreen::m_pDreamScreen = NULL;

void handleDreamScreenPackets(AsyncUDPPacket packet);
DreamScreen* pLocalDreamScreen = NULL; 

DreamScreen::DreamScreen(int nPort){
  DreamScreen::m_pDreamScreen = this;

  m_iPort = nPort;  
  m_pAsyncUdp = new AsyncUDP();
  pLocalDreamScreen = this; 
  m_bConnected = false; 
  m_szAddDreamScreenToNetwork = NULL;
  m_lLastUpdate = 0;
  m_bCurrentFps = 0; 
  m_bCurrentFramesCtr = 0;
  m_timeFPSCheck = 0;
  m_bSupport = 0;
  m_bEmulation = 0;
  m_bGroup = 0;
  m_bColorSmoothing = 0;
}

void DreamScreen::SetSupport(byte bNew, bool bEpprom, bool bInitDreamScreen) {
  m_bSupport = bNew;
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingByte("DsSupport", m_bSupport);
  if (m_bSupport == 1)
    _DBGPRINTF(PSTR("DreamScreen:SetSupport:Enabled\n"));
  else
    _DBGPRINTF(PSTR("DreamScreen:SetSupport:Disabled\n"));
  (void)bInitDreamScreen;
}
void DreamScreen::SetEmulation(byte bNew, bool bEpprom) {
  m_bEmulation = bNew;
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingByte("DsEmulation", m_bEmulation);
  if (m_bEmulation == 1)
    _DBGPRINTF(PSTR("DreamScreen:SetEmulation:Enabled\n"));
  else
    _DBGPRINTF(PSTR("DreamScreen:SetEmulation:Disabled\n"));
}
void DreamScreen::SetGroup(byte bNew, bool bEpprom) {
  m_bGroup = bNew;
  _DBGPRINTF_VERBOSE(PSTR("DreamScreen:SetGroup Value=%d\n"), bNew);
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingByte("DsGroup", m_bGroup);
}
void DreamScreen::SetLowColorSmoothing(byte bNew, bool bEpprom) {
  m_bColorSmoothing = bNew;
  _DBGPRINTF_VERBOSE(PSTR("DreamScreen:SetLowColorSmoothing Value=%d\n"), bNew);
  if (bEpprom && global::m_pEepromHelper)
    global::m_pEepromHelper->SetSettingByte("DsClrSmt", m_bColorSmoothing);
}
void DreamScreen::DiscoverDevices() {
  byte bufPacket[DREAMSCREEN_MAX_PACKET_LENGTH];
  size_t bufLen = DREAMSCREEN_MAX_PACKET_LENGTH;
  bufLen = CreateUdpPacket(&bufPacket[0], bufLen, 0x010A /*CurrentState*/, NULL, 0, 0x30, 0xFF);
  if (bufLen > 0) {
    _DBGPRINTF(PSTR("DreamScreen:DiscoverDevices Send Broadcast\n"));
    SendUdpBroadcast(&bufPacket[0], bufLen);
  } else {
    global::errprintf_P(PSTR("DreamScreen:DiscoverDevices failed to create Broadcast\n"));
  }
}

void DreamScreen::StartServer(){
  if(m_pAsyncUdp->listen(m_iPort)) {
    _DBGPRINTF(PSTR("DreamScreen: Successfully started Udp Server on Port: %d\n"), m_iPort);
  } else {
    _DBGPRINTF(PSTR("DreamScreen: Failed starting Udp Server on Port: %d\n"), m_iPort);
  }

  // Start receiving DreamScreen UDP Packets
  m_pAsyncUdp->onPacket(&handleDreamScreenPackets);   
}

bool DreamScreen::AnalyzeUdpPacket(DreamScreenPacket* pDsPacket, AsyncUDPPacket* pPacket) {
  bool bReturn = false;
  size_t stLength = pPacket->length(); 

  // Check for valid packet length  
  if (stLength >= 5 ) {
    byte* pBytes = pPacket->data(); 
    pDsPacket->bStartPacket = *(pBytes+0);
    pDsPacket->bPacketLength = *(pBytes+1) - DREAMSCREEN_MIN_PACKET_LENGTH; // Only limit to actual PayLoad length!
    pDsPacket->bGroupAddress = *(pBytes+2);
    pDsPacket->bFlags = *(pBytes+3);
    pDsPacket->u16Command = (uint16_t)( (*(pBytes+4) << 8 ) | (*(pBytes+5) & 0xff) );//  (*(pBytes+4) & 0xFF) << 8 | (*(pBytes+5) & 0xFF) >> 8);

    int iPayloadLen = stLength - 5 /*Head*/ - 1 /*CRC*/; 
    if (iPayloadLen > 0 && iPayloadLen < DREAMSCREEN_MAX_PACKET_LENGTH ) {
      // Check Checksum now!
      pDsPacket->bChecksum = *(pBytes+(stLength-1));

      // Calc Checksum for comparison
      byte bChksumCompare = 0x00; 
      bChksumCompare = this->uartCommCalculateCRC8(bChksumCompare, pBytes, stLength-1);
      if (bChksumCompare == pDsPacket->bChecksum) {
        _DBGPRINTF_ULTRA(PSTR("DreamScreen:AnalyzeUdpPacket VALID Packet/Payload length %d - Checksum=%d Compare=%d\n"), iPayloadLen, pDsPacket->bChecksum, bChksumCompare);
        // Return VALID packet
        bReturn = true; 
      } else {
        _DBGPRINTF(PSTR("DreamScreen:AnalyzeUdpPacket Invalid Packet/Payload CHECKSUM FAIL %d/%d\n"), pDsPacket->bChecksum, bChksumCompare);
      }
    } else {
      global::errprintf_P(PSTR("DreamScreen:AnalyzeUdpPacket Invalid Packet/Payload length %d/%d\n"), stLength, iPayloadLen);
    }
    
  }
  return bReturn; 
}

size_t DreamScreen::CreateUdpPacket(byte* pbBuf, size_t stBufLen, short shCommand, byte* pbPayLoad, size_t stPayLoadLength, byte bFlags, byte bGroupNumer) {
  size_t lenReturn = 0;
  byte* ptr = pbBuf;
  byte bPacketLength = DREAMSCREEN_MIN_PACKET_LENGTH + stPayLoadLength;
  if (bPacketLength <= stBufLen) {
    *ptr++ = 0xFC;          // Fixed Startbyte
    *ptr++ = bPacketLength; // Packetlength
    *ptr++ = bGroupNumer;   // Groupnumber
    *ptr++ = bFlags /* (bGroupNumer == 0xFF ? Receive : bFlags)*/; // Flags
    *ptr++ = ((shCommand & 0xFF00) >> 8); 
    *ptr++ = (shCommand & 0xFF);
    if (stPayLoadLength > 0) {
      // Add Payload to packet
      for(int i=0; i<stPayLoadLength; i++) 
        *ptr++ = *(pbPayLoad+i); 
    }
    // Finally calc checksum
    *ptr = this->uartCommCalculateCRC8(0x0, pbBuf, bPacketLength + 1 /* Add Fixed StartByte */);

    // Return VALID packet
    lenReturn = bPacketLength + 1 + 1; // Fixed StartByte and Checksum Byte need to be added to the length
  } else {
    global::errprintf_P(PSTR("DreamScreen:CreateUdpPacket Invalid Buf length %d/%d\n"), stBufLen, bPacketLength);
  }
  return lenReturn; 
}

bool DreamScreen::SendUdpBroadcast(byte* pbBuf, size_t stBufLen) {
  // Send broadcast into network
  size_t stBytesSend = m_pAsyncUdp->broadcastTo((uint8_t*) pbBuf, stBufLen, m_iPort);
  return (stBytesSend > 0 ? true : false);
}

bool DreamScreen::SendUdpMessage(byte* pbBuf, size_t stBufLen, IPAddress ip, int iPort) {
  // Send broadcast into network
  size_t stBytesSend = m_pAsyncUdp->writeTo((const uint8_t*) pbBuf, stBufLen, ip, iPort);
  return (stBytesSend > 0 ? true : false);
}

byte DreamScreen::uartCommCalculateCRC8(byte crcIn, byte* pData, byte bSize) {
  size_t a;
  byte crc = crcIn;
  byte* ptr = pData;

  // Loop through data
  if ( ptr != NULL ) {
    for (a=0; a<bSize; a++) {
      crc = uartComm_crc8_table[crc ^ (*ptr)];
      ptr++;
    }
  }
  return crc;
}

byte DreamScreen::GetDreamscreenGroupsCount() {
  return m_dreamscreen_groups_count;
}

byte DreamScreen::GetDreamscreenGroupID(int iIndex) {
  if (iIndex <= m_dreamscreen_groups_count) 
    return m_dreamscreen_groupids[iIndex];
  else 
    return 0x00; 
}

const char* DreamScreen::GetDreamscreenGroupName(int iIndex) {
  if (iIndex <= m_dreamscreen_groups_count) 
    return (const char*) &m_dreamscreen_groups[iIndex][0];
  else 
    return PSTR("");
}

bool DreamScreen::AddDreamScreenToNetwork() {
  bool dreamscreenSuccess = false; 
  bool wifiSuccess = false; 
  unsigned long timeNow; 
  unsigned long timeWifiWaitStart; 
  char statusmsg[DREAMSCREEN_MAX_PACKET_LENGTH];

  // Disable WIFI Connect Check
  global::SetWifiConnectCheck(false);

  // Disconnect from current WIFI!
  WiFi.mode(WIFI_OFF);
  WiFi.disconnect(true);
  _DBGPRINTF(PSTR("DreamScreen:AddDreamScreenToNetwork disconnect from %s \n"), global::GetWifiSSID());
  // Wait for disconnect 
  timeNow = millis();
  timeWifiWaitStart = millis();
  do {
      _DBGPRINTF(PSTR("DreamScreen:AddDreamScreenToNetwork WAIT for disconnect from %s - %d / %d\n"), global::GetWifiSSID(), (timeNow-timeWifiWaitStart), DEFAULT_WIFI_CONNECT_TIMEOUT);
      _delay(1000);
      timeNow = millis();
  } while ( WiFi.isConnected() && (timeNow-timeWifiWaitStart) < DEFAULT_WIFI_CONNECT_TIMEOUT ); // Loop until success or TIMEOUT hits 
  
  // Start Connect to DreamScreen Network
  _DBGPRINTF(PSTR("DreamScreen:AddDreamScreenToNetwork connect to DreamScreen network %s \n"), m_szAddDreamScreenToNetwork);
  WiFi.mode(WIFI_STA);
  WiFi.begin(m_szAddDreamScreenToNetwork);

  // Wait for connect
  timeNow = millis(); 
  timeWifiWaitStart = millis(); 
  do {
    if (WiFi.status() == WL_CONNECTED) {
      wifiSuccess = true;

      // Now tell DreamScreen Controller which Network to connect to:
      byte bufPacket[DREAMSCREEN_MAX_PACKET_LENGTH];
      size_t bufLen = DREAMSCREEN_MAX_PACKET_LENGTH;

      // --- Create WIFI MAGIC packet
      byte bufPayload[1];
      bufPayload[0] = 0x01; 
      bufLen = global::m_pDreamScreen->CreateUdpPacket(&bufPacket[0], DREAMSCREEN_MAX_PACKET_LENGTH, (short)WifiMagic, &bufPayload[0], 1, 0x01, 0xFF);
      // Send WIFI SSID packet
      if (bufLen > 0) {
        _DBGPRINTF(PSTR("DreamScreen:AddDreamScreenToNetwork Send WIFI MAGIC to %s \n"), WiFi.gatewayIP().toString().c_str());
        global::m_pDreamScreen->SendUdpMessage(&bufPacket[0], bufLen, WiFi.gatewayIP(), DREAMSCREEN_UDP_PORT);
        _delay(500); 
      } else {
        global::errprintf_P(PSTR("global:AddDreamScreenToNetwork failed to create WIFI SSID packet\n"));
      }
      // --- 

      // --- Create WIFI SSID packet
      bufLen = global::m_pDreamScreen->CreateUdpPacket(&bufPacket[0], DREAMSCREEN_MAX_PACKET_LENGTH, (short)WifiSSD, (byte*)global::GetWifiSSID(), strlen(global::GetWifiSSID()), 0x01, 0xFF);
      // Send WIFI SSID packet
      if (bufLen > 0) {
        _DBGPRINTF(PSTR("DreamScreen:AddDreamScreenToNetwork Send WIFI SSID to %s \n"), WiFi.gatewayIP().toString().c_str());
        global::m_pDreamScreen->SendUdpMessage(&bufPacket[0], bufLen, WiFi.gatewayIP(), DREAMSCREEN_UDP_PORT);
        _delay(500); 
      } else {
        global::errprintf_P(PSTR("global:AddDreamScreenToNetwork failed to create WIFI SSID packet\n"));
      }
      // --- 

      // --- Create WIFI Key packet
      bufLen = global::m_pDreamScreen->CreateUdpPacket(&bufPacket[0], DREAMSCREEN_MAX_PACKET_LENGTH, (short)WifiKey, (byte*)global::GetWifiPassword(), strlen(global::GetWifiPassword()), 0x01, 0xFF);
      // Send WIFI SSID packet
      if (bufLen > 0) {
        _DBGPRINTF(PSTR("DreamScreen:AddDreamScreenToNetwork Send WIFI Key to %s \n"), WiFi.gatewayIP().toString().c_str());
        global::m_pDreamScreen->SendUdpMessage(&bufPacket[0], bufLen, WiFi.gatewayIP(), DREAMSCREEN_UDP_PORT);
        _delay(500); 
      } else {
        global::errprintf_P(PSTR("global:AddDreamScreenToNetwork failed to create WIFI Key packet\n"));
      }
      // --- 

      dreamscreenSuccess = true; 
    } else {
      _DBGPRINTF(PSTR("DreamScreen:AddDreamScreenToNetwork WAIT for connect to %s - %d / %d\n"), m_szAddDreamScreenToNetwork, (timeNow-timeWifiWaitStart), DEFAULT_WIFI_CONNECT_TIMEOUT);
      _delay(1000); 
      global::TogglePwrLed();
      timeNow = millis(); 
    }
  } while (!wifiSuccess && (timeNow-timeWifiWaitStart) < DEFAULT_WIFI_CONNECT_TIMEOUT); // Loop until success or TIMEOUT hits 

  // Use function to reconnect to own WIFI
  global::ConnectWifiConfigured(global::GetWifiSSID(), global::GetWifiPassword(), false); 
  
  // Set AddToNetworkRunning Mode
  global::SetWifiConnectCheck(true);

  // Set Last Status!
  snprintf_P(statusmsg, DREAMSCREEN_MAX_PACKET_LENGTH, PSTR("\"status\": \"%d\",\n\"action\": \"AddDreamScreenToNetwork\",\n\"device\": \"%s\" "), 
    (dreamscreenSuccess ? 1 : 0), m_szAddDreamScreenToNetwork );
  global::m_pWebServer->SetLastStatusMessage( statusmsg );
  this->SetAddDreamScreenToNetwork(NULL); 
  _DBGPRINTF(PSTR("DreamScreen:AddDreamScreenToNetwork result: %s - Time %d\n"), statusmsg, (timeNow-timeWifiWaitStart));

  // return success state
  return dreamscreenSuccess;
}

void DreamScreen::SetAddDreamScreenToNetwork(const char* szNetwork) {
  this->SetAddDreamScreenToNetwork(szNetwork, (szNetwork != NULL ? strlen(szNetwork) : 0) ); 
}

void DreamScreen::SetAddDreamScreenToNetwork(const char* szNetwork, int szLen) {
  // Delete old mem
  if (m_szAddDreamScreenToNetwork != NULL) {
    delete[] m_szAddDreamScreenToNetwork; 
    m_szAddDreamScreenToNetwork = NULL;
  }
  // Fill new Status message if set
  if (szNetwork != NULL) {
    if ( szLen > (WLAN_SSID_MAX-1)) 
      szLen = (WLAN_SSID_MAX-1); // Limit to max size
    m_szAddDreamScreenToNetwork = new char[szLen + 1];
    memcpy(m_szAddDreamScreenToNetwork, szNetwork, szLen);
    *(m_szAddDreamScreenToNetwork+szLen) = '\0';
    // strncpy(m_szAddDreamScreenToNetwork, (const char *) szNetwork, WLAN_SSID_MAX);
  }
}

void DreamScreen::SetLastUpdate() {
  m_lLastUpdate = millis(); 
  global::m_pLedHelper->SetLastUpdate();
}

void handleDreamScreenPackets(AsyncUDPPacket packet) {
  // Helper variables 
  DreamScreenPacket dsPacket; 
  byte* pBytes = packet.data(); 

  // Validate Packet
  if (pLocalDreamScreen->AnalyzeUdpPacket(&dsPacket, &packet)) {
    _DBGPRINTF_ULTRA(PSTR("DreamScreen:handleDreamScreenPackets VALID UDP Packet Type: %s, From: %s:%d, To:%s:%d, Length:%d, DsCommand: 0x%04X, GroupAddress: 0x%02X, PacketLength: %d\n"), 
        packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast",
        packet.remoteIP().toString().c_str(), 
        packet.remotePort(),
        packet.localIP().toString().c_str(), 
        packet.localPort(),
        packet.length(),
        dsPacket.u16Command,
        dsPacket.bGroupAddress,
        dsPacket.bPacketLength
        );
    
    // Process DreamScreen Packet here
    if (dsPacket.u16Command == (short)CurrentState) {
      if (dsPacket.bPacketLength >= 33) {
        char szDeviceName[DREAMSCREEN_MAX_STRING]; 
        char szGroupName[DREAMSCREEN_MAX_STRING]; 

        // Copy Strings over
        strncpy(&szDeviceName[0], (const char *) pBytes+DREAMSCREEN_HEADER_LENGTH, DREAMSCREEN_MAX_STRING);
        strncpy(&szGroupName[0], (const char *) pBytes+DREAMSCREEN_HEADER_LENGTH+16, DREAMSCREEN_MAX_STRING);

        // Copy GroupNumber
        byte bGroupNumber = *(pBytes+DREAMSCREEN_HEADER_LENGTH+32); 

        // Debug Output
        char bufferDebug[1024];
        int bufEnd = dsPacket.bPacketLength*3; 
        if (bufEnd < 1024) {
          bufferDebug[bufEnd] = 0;
          for(int j = 0; j < dsPacket.bPacketLength; j++)
              sprintf(&bufferDebug[3*j], "%02X:", *(pBytes+DREAMSCREEN_HEADER_LENGTH+j));
          _DBGPRINTF(PSTR("DreamScreen:handleDreamScreenPackets VALID CurrentState received from %s: GroupAddress: 0x%02X, GroupName: %s, Payloaddata: %s\n"), 
            szDeviceName, bGroupNumber, szGroupName, bufferDebug );
        } else {
          _DBGPRINTF(PSTR("DreamScreen:handleDreamScreenPackets VALID CurrentState received from %s, GroupAddress: 0x%02X, GroupName: %s\n"), 
            szDeviceName, dsPacket.bGroupAddress, szGroupName );
        }

        int iIndex = m_dreamscreen_groups_count; 
        bool bNew = true;
        for(int i = 0; i < m_dreamscreen_groups_count; i++) {
          if (m_dreamscreen_groupids[i] == bGroupNumber) {
            iIndex = i; 
            bNew = false;
            break;  
          }
        }
        if (iIndex < DREAMSCREEN_MAX_GROUPS) {
          // Copy Groupname over to Array
          snprintf_P(m_dreamscreen_groups[iIndex], DREAMSCREEN_MAX_STRING, PSTR("%s"), szGroupName); 
          if (bNew) {
            m_dreamscreen_groupids[iIndex] = bGroupNumber; 
            m_dreamscreen_groups_count++; 
          }

          _DBGPRINTF(PSTR("DreamScreen:handleDreamScreenPackets ADDED Group %s with GroupdID %d to Index %d\n"), 
            m_dreamscreen_groups[iIndex], m_dreamscreen_groupids[iIndex], iIndex );
        } else {
          global::errprintf_P(PSTR("global:handleDreamScreenPackets NO MORE FREE MEMORY FOR DREAMSCREEN Groups! %d\n"), m_dreamscreen_groups_count);
        } 
      } else {
        _DBGPRINTF_ULTRA(PSTR("DreamScreen:handleDreamScreenPackets CurrentState received from %s, but no DATA\n"), packet.remoteIP().toString().c_str());
      }
    } else if (dsPacket.u16Command == (short)SubscriptionRequest) {
      if (dsPacket.bGroupAddress == global::GetDreamScreenGroup()) {
        if (global::GetDreamScreenEmulation() == 1) {
          byte bufPacket[DREAMSCREEN_MAX_PACKET_LENGTH]; 
          size_t bufLen = DREAMSCREEN_MAX_PACKET_LENGTH; 
          byte bufPayload[1];
          bufPayload[0] = 0x01; 
          // Create subscribe response packet
          bufLen = global::m_pDreamScreen->CreateUdpPacket(&bufPacket[0], bufLen, (short)SubscriptionRequest, &bufPayload[0], 1, dsPacket.bFlags, dsPacket.bGroupAddress);
  
          // Send subscribe response packet
          if (bufLen > 0) {
            _DBGPRINTF_ULTRA(PSTR("global:handleDreamScreenPackets Send subscribe response to %s \n"), packet.remoteIP().toString().c_str());
            global::m_pDreamScreen->SendUdpMessage(&bufPacket[0], bufLen, packet.remoteIP(), packet.remotePort());

            // ATTENTION - DO NOT SET TO RELAY YET - we actually need to RECEIVE data first 
            
          } else {
            global::errprintf_P(PSTR("global:handleDreamScreenPackets failed to create subscribe response\n"));
          }
        } else {
          // Turn LED Mode off if on RELAY 
          if (global::m_pLedHelper->GetLedMode() == LED_MODE_RELAY) {
            global::errprintf_P(PSTR("global:handleDreamScreenPackets turn LED_MODE_OFF, DreamScreenEmulation disabled.\n"));
            global::m_pLedHelper->SetChangeLedMode(LED_MODE_OFF);
          }
        }
      }
    } else if (dsPacket.u16Command == (short)SubscriptionData) {
      if (dsPacket.bPacketLength >= DREAMSCREEN_MIN_SUBSCRIBERDATA_LENGTH) {
        if (global::GetDreamScreenEmulation() == 1) {
          // Set the correct LED Mode while receiving LED data!

          if (global::m_pLedHelper->GetLedMode() != LED_MODE_VIDEO) {
            // Change LED Mode to RELAY
            global::m_pLedHelper->SetChangeLedMode(LED_MODE_RELAY);
            
            // Calculate dynamic Delay Length now!
            global::CalcDynamicDelayLength( DreamScreen::m_pDreamScreen->GetLastUpdate() );
  
            if (DreamScreen::m_pDreamScreen->GetTimeFPSCheck() == 0)
              DreamScreen::m_pDreamScreen->SetTimeFPSCheck(millis() + 1000);
              
            /* Handle FPS counting after 1000ms period of time */
            if (millis() > DreamScreen::m_pDreamScreen->GetTimeFPSCheck() ) {
              /* Update timeWIFICheck */
              _DBGPRINTF_ULTRA(PSTR("global:handleDreamScreenPackets: FPS: %d\n"), DreamScreen::m_pDreamScreen->GetCurrentFramesCtr());
              DreamScreen::m_pDreamScreen->SetCurrentFps( DreamScreen::m_pDreamScreen->GetCurrentFramesCtr()); 
          
              // Reset back to 0
              DreamScreen::m_pDreamScreen->SetCurrentFramesCtr(0);
              DreamScreen::m_pDreamScreen->SetTimeFPSCheck(millis() + 1000);
            } else {
              // Increment Framecounter
              DreamScreen::m_pDreamScreen->IncrementCurrentFramesCtr(); 
            }
          } else {
              _DBGPRINTF_ULTRA(PSTR("global:handleDreamScreenPackets: IGNORE Paket, LED Mode already in Mode %d\n"), global::m_pLedHelper->GetLedMode());
              // _DBGPRINTF_ULTRA(PSTR("LEDHelper:SetLedMode: Already in VIDEO/RELAY Mode\n"));
          }
  
          // Set Last Updated
          DreamScreen::m_pDreamScreen->SetLastUpdate();
        }
                
        if (global::m_bUltraDebugEnabled) {
          // Debug Output
          char bufferDebug[DREAMSCREEN_MAX_PACKET_LENGTH];
          int bufEnd = dsPacket.bPacketLength*3; 
          if (bufEnd < DREAMSCREEN_MAX_PACKET_LENGTH) {
            bufferDebug[bufEnd] = 0;
            for(int j = 0; j < dsPacket.bPacketLength; j++)
                sprintf(&bufferDebug[3*j], "%02X:", *(pBytes+DREAMSCREEN_HEADER_LENGTH+j));
            _DBGPRINTF(PSTR("DreamScreen:handleDreamScreenPackets VALID SubscriptionData received from %s: %s\n"), packet.remoteIP().toString().c_str(), bufferDebug );
          } else {
            global::errprintf_P(PSTR("global:handleDreamScreenPackets invalid SubscriptionData data received %d\n"), dsPacket.bPacketLength);
          }
        }

        
#if defined(ENABLE_HUE)
        // Get Handle HUE color stuff first
        CRGB* pLedsHue = (CRGB*) global::m_pHueHelper->getLedsHuePtr(); 
        CRGB* pLedFrames = (CRGB*) global::m_pLedHelper->GetVoidLedFramesPtr(); // Get Pointer to Frames data
        byte* ptr = pBytes+DREAMSCREEN_HEADER_LENGTH; // Set start byte pointer
        byte bLowColorSmoothing = global::GetDreamScreenLowColorSmoothing(); 
        
        // Copy Bytes over to hue color leds
        pLedsHue[HUE_LED_BOTTOM_RIGHT].r = *ptr++;
        pLedsHue[HUE_LED_BOTTOM_RIGHT].g = *ptr++;
        pLedsHue[HUE_LED_BOTTOM_RIGHT].b = *ptr++;
        pLedsHue[HUE_LED_MIDDLE_RIGHT].r = *ptr++;
        pLedsHue[HUE_LED_MIDDLE_RIGHT].g = *ptr++;
        pLedsHue[HUE_LED_MIDDLE_RIGHT].b = *ptr++;
        pLedsHue[HUE_LED_TOP_RIGHT].r = *ptr++;
        pLedsHue[HUE_LED_TOP_RIGHT].g = *ptr++;
        pLedsHue[HUE_LED_TOP_RIGHT].b = *ptr++;
        pLedsHue[HUE_LED_TOP_CENTER3].r = *ptr++;
        pLedsHue[HUE_LED_TOP_CENTER3].g = *ptr++;
        pLedsHue[HUE_LED_TOP_CENTER3].b = *ptr++;
        pLedsHue[HUE_LED_TOP_CENTER2].r = *ptr++;
        pLedsHue[HUE_LED_TOP_CENTER2].g = *ptr++;
        pLedsHue[HUE_LED_TOP_CENTER2].b = *ptr++;
        pLedsHue[HUE_LED_TOP_CENTER1].r = *ptr++;
        pLedsHue[HUE_LED_TOP_CENTER1].g = *ptr++;
        pLedsHue[HUE_LED_TOP_CENTER1].b = *ptr++;
        pLedsHue[HUE_LED_TOP_LEFT].r = *ptr++;
        pLedsHue[HUE_LED_TOP_LEFT].g = *ptr++;
        pLedsHue[HUE_LED_TOP_LEFT].b = *ptr++;
        pLedsHue[HUE_LED_MIDDLE_LEFT].r = *ptr++;
        pLedsHue[HUE_LED_MIDDLE_LEFT].g = *ptr++;
        pLedsHue[HUE_LED_MIDDLE_LEFT].b = *ptr++;
        pLedsHue[HUE_LED_BOTTOM_LEFT].r = *ptr++;
        pLedsHue[HUE_LED_BOTTOM_LEFT].g = *ptr++;
        pLedsHue[HUE_LED_BOTTOM_LEFT].b = *ptr++;
        pLedsHue[HUE_LED_BOTTOM_CENTER1].r = *ptr++;
        pLedsHue[HUE_LED_BOTTOM_CENTER1].g = *ptr++;
        pLedsHue[HUE_LED_BOTTOM_CENTER1].b = *ptr++;
        pLedsHue[HUE_LED_BOTTOM_CENTER2].r = *ptr++;
        pLedsHue[HUE_LED_BOTTOM_CENTER2].g = *ptr++;
        pLedsHue[HUE_LED_BOTTOM_CENTER2].b = *ptr++;
        pLedsHue[HUE_LED_BOTTOM_CENTER3].r = *ptr++;
        pLedsHue[HUE_LED_BOTTOM_CENTER3].g = *ptr++;
        pLedsHue[HUE_LED_BOTTOM_CENTER3].b = *ptr++;

        /* Post Process LEDs */
        for (int i = 0; i < MAX_HUE_LEDS; i++) {
          int iRed = pLedsHue[i].r; 
          int iGreen = pLedsHue[i].g; 
          int iBlue = pLedsHue[i].b; 

          /* 1. DreamScreen Low Color Smoothing */
          if (bLowColorSmoothing > 0) {
            global::m_pLedHelper->PreProcessLowColorSmoothing(i, bLowColorSmoothing, &iRed, &iGreen, &iBlue); 
          }

          /* 2. Check for ColorBoost 0-100% */
          global::m_pLedHelper->PreProcessColorBoost(i, &iRed, &iGreen, &iBlue); 
      
          /* 3. Check for Value Gain */
          global::m_pLedHelper->PreProcessValueGain(i, &iRed, &iGreen, &iBlue); 
      
          /* 4. Calc Min Luminosity */
          global::m_pLedHelper->PreProcessMinLuminosity(i, &iRed, &iGreen, &iBlue);

          /* 5. Frame Smoothing */
          global::m_pLedHelper->PreProcessFrameSmoothing(i, &iRed, &iGreen, &iBlue, pLedFrames);

          // Finally copy over  and check if values are out of bounds
          pLedsHue[i].r = (byte)(iRed < 255 ? (iRed > 0 ? iRed : 0) : 255);
          pLedsHue[i].g = (byte)(iGreen < 255 ? (iGreen > 0 ? iGreen : 0) : 255);
          pLedsHue[i].b = (byte)(iBlue < 255 ? (iBlue > 0 ? iBlue : 0) : 255);
        }

        /* 5. Post Process for Frame Smoothing */
        global::m_pLedHelper->PostProcessFrameSmoothing(pLedFrames, pLedsHue, MAX_HUE_LEDS);         
        
        // Set new LED Data!
        global::m_pHueHelper->setNewLedData(true); 

        // For AutoOff handling, set Last Update
        global::m_pLedHelper->SetLastUpdate();

        // Set helper variable to true
        pLocalDreamScreen->SetConnected(true); 
#endif
      } else {
        // TODO DEBUG Perhaps
      }
    } else {
      _DBGPRINTF(PSTR("DreamScreen:handleDreamScreenPackets unknown DreamScreen command %d From: %s:%d, To:%s:%d, Length:%d, Data: %.*s\n"),
        dsPacket.u16Command,
        packet.remoteIP().toString().c_str(), 
        packet.remotePort(),
        packet.localIP().toString().c_str(), 
        packet.localPort(),
        packet.length(),
        packet.length(),
        packet.data()    
      );
    }
  } else {
    _DBGPRINTF(PSTR("DreamScreen:handleDreamScreenPackets INVALID UDP Packet Type: %s, From: %s:%d, To:%s:%d, Length:%d, Data: %.*s\n"), 
      packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast",
      packet.remoteIP().toString().c_str(), 
      packet.remotePort(),
      packet.localIP().toString().c_str(), 
      packet.localPort(),
      packet.length(),
      packet.length(),
      packet.data()    
    );
  }
}

#endif
