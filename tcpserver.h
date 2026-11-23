#if defined(ENABLE_TCP)
# ifndef tcpserver_h
# define tcpserver_h

// TCP includes
#include <WiFi.h>
#include <AsyncTCP.h>

// When a packet is considered timeout
#define PAKET_TIMEOUT 5000
#define CLIENT_REPLY_SIZE 256

class TcpServer {
public:
    // Constructors
    TcpServer (int nPort);

    // Declare helper functions
    void StartServer(); 
    AsyncServer* GetServer() { return pTcpServer; }

    AsyncClient* getAsyncCurClient() { return m_asyncCurClient; } 
    void setAsyncCurClient(AsyncClient* newClient);  
    inline unsigned long GetLastUpdate() { return m_lLastUpdate; }
    void SetLastUpdate();
    void DisconnectClient(); 

    /* Public for now */
    unsigned long m_lLastPacketTime; 
    size_t m_tzBufLen; // Used to store current buffer len
#if TCP_MSS < MAX_PAKET_LENGTH
    byte m_bBuf[MAX_PAKET_LENGTH];
#endif

    // Universal Pointer to UdpServer Obj
    static TcpServer* m_pTcpServer;
private:
    // Declare private functions
    void InitServer(); 

    // Declare helper variables
    bool m_bInit;
    int m_nPort; 
    AsyncServer* pTcpServer;
    
    // Helpers for current TCP client
    AsyncClient* m_asyncCurClient; 
    unsigned long m_lLastUpdate;
};

# endif
#endif
