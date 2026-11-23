#ifndef udphelper_h
#define udphelper_h

#include <WiFi.h>
#include "AsyncUDP.h"

#define UDP_HEADLEN 8
#define UDP_CHUNKSIZE 4


class UdpServer {
public:
    // Constructors
    UdpServer(int nPort);

    void StartServer();
    AsyncUDP* GetAsyncUdp() { return m_pAsyncUdp; }
    int GetPort() { return m_iPort; }
    inline unsigned long GetLastUpdate() { return m_lLastUpdate; }
    void SetLastUpdate(void);
#if defined(ENABLE_RELAY)
    bool SendUdpDiscoverBroadcast(void);
#endif
    /** Run discovery: send broadcast, collect replies for ~2.5s, return JSON string (caller frees) */
    static size_t RunHttpDiscover(char* pszOut, size_t stMax);
    bool SendUdpBroadcast(byte* pbBuf, size_t stBufLen);
    bool SendUdpMessage(byte* pbBuf, size_t stBufLen, IPAddress ip, int iPort);

    // Universal Pointer to UdpServer Obj
    static UdpServer* m_pUdpServer;
    esp_err_t GetLastErr(void) { return (m_pAsyncUdp != NULL ? m_pAsyncUdp->lastErr() : ERR_OK); };
private:
    // Declare helper variables
    AsyncUDP* m_pAsyncUdp;
    int m_iPort; 
    unsigned long m_lLastUpdate;
};


#endif 
