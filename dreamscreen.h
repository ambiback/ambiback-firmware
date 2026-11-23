#ifndef dreamscreen_h
#define dreamscreen_h

#include <WiFi.h>
#include "AsyncUDP.h"

// --- STRUCTS 
struct DreamScreenPacket
{
    byte bStartPacket;   // Start of Packet - is used to provide synchronization when parsing packets. Always 0xFC
    byte bPacketLength;  // Packet Length - Packet Length from Group Address (inclusive) to CRC (inclusive)
    byte bGroupAddress;  // Group Address - the group number which the device belongs. 0x00 indicates ‘No specified Group’, 0x01 indicates group 1, 0x02 indicates group 2, etc. If the Group Address is incorrect, DreamScreen will discard the message.
    byte bFlags;         // Flags - provides context for handling the message
    uint16_t u16Command; // Command Upper/lower combined - specifies command namespace
    byte* pbPayload;     // Pointer to Byte Payload - variable length, depending upon the context of the command
    byte bChecksum;      // CRC - 8 bit CRC for error detection. If incorrect, DreamScreen will discard the message.
};
// Example, Set Mode 0:
//  FC:06:01:21:03:01:00:C6.Command is 03:01, with payload 0x00, packet length 0x06    
// ---

#define DREAMSCREEN_MAX_PACKET_LENGTH 256
#define DREAMSCREEN_HEADER_LENGTH 6
#define DREAMSCREEN_MIN_PACKET_LENGTH 5
#define DREAMSCREEN_MAX_STRING 16
#define DREAMSCREEN_MAX_GROUPS 16
#define DREAMSCREEN_MIN_SUBSCRIBERDATA_LENGTH 36
#define DREAMSCREEN_UDP_PORT 8888

class DreamScreen {
public:
  // Constructors
  DreamScreen(int nPort);

  void StartServer();
  AsyncUDP* GetAsyncUdp() { return m_pAsyncUdp; }
  int GetPort() { return m_iPort; }
  int GetConnected() { return m_bConnected; }
  void SetConnected(bool bNew) { m_bConnected = bNew; }

  // Helper functions
  bool AnalyzeUdpPacket(DreamScreenPacket* pDsPacket, AsyncUDPPacket* pPacket);
  size_t CreateUdpPacket(byte* pbBuf, size_t stBufLen, short shCommand, byte* pbPayLoad, size_t stPayLoadLength, byte bFlags, byte bGroupNumer); 
  bool SendUdpBroadcast(byte* pbBuf, size_t stBufLen);
  bool SendUdpMessage(byte* pbBuf, size_t stBufLen, IPAddress ip, int iPort);
  byte uartCommCalculateCRC8(byte crcIn, byte* pData, byte bSize); 

  byte GetDreamscreenGroupsCount();
  byte GetDreamscreenGroupID(int iIndex); 
  const char* GetDreamscreenGroupName(int iIndex);

  bool AddDreamScreenToNetwork(void); 

  void SetAddDreamScreenToNetwork(const char* szNetwork); 
  void SetAddDreamScreenToNetwork(const char* szNetwork, int szLen); 
  inline char* GetAddDreamScreenToNetwork(void) { return m_szAddDreamScreenToNetwork; } 
  
  inline unsigned long GetLastUpdate() { return m_lLastUpdate; }
  void SetLastUpdate(void);
  inline byte GetCurrentFps() { return m_bCurrentFps; }
  inline void SetCurrentFps(byte bNew) { m_bCurrentFps = bNew; }
  inline byte GetCurrentFramesCtr() { return m_bCurrentFramesCtr; }
  inline void SetCurrentFramesCtr(byte bNew) { m_bCurrentFramesCtr = bNew; }
  inline unsigned long GetTimeFPSCheck() { return m_timeFPSCheck; }
  inline void SetTimeFPSCheck(unsigned long lNew) { m_timeFPSCheck = lNew; }
  
  inline void IncrementCurrentFramesCtr() { m_bCurrentFramesCtr++; }

  // Support, emulation, group, low color smoothing (persisted via EEPROM)
  byte GetSupport() const { return m_bSupport; }
  void SetSupport(byte bNew, bool bEpprom, bool bInitDreamScreen = false);
  void SetSupport(byte bNew, bool bEpprom) { SetSupport(bNew, bEpprom, false); }
  byte GetEmulation() const { return m_bEmulation; }
  void SetEmulation(byte bNew, bool bEpprom);
  byte GetGroup() const { return m_bGroup; }
  void SetGroup(byte bNew, bool bEpprom);
  byte GetLowColorSmoothing() const { return m_bColorSmoothing; }
  void SetLowColorSmoothing(byte bNew, bool bEpprom);
  void DiscoverDevices();

  static DreamScreen* m_pDreamScreen;
private:
  // Declare helper variables
  AsyncUDP* m_pAsyncUdp;
  int m_iPort; 
  bool m_bConnected;
  char* m_szAddDreamScreenToNetwork;
  unsigned long m_lLastUpdate;
  byte m_bCurrentFps;           // Save current FPS 
  byte m_bCurrentFramesCtr;     // Counter for current FPS
  unsigned long m_timeFPSCheck;
  byte m_bSupport;
  byte m_bEmulation;
  byte m_bGroup;
  byte m_bColorSmoothing;
};

#endif 
