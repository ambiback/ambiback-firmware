#ifndef ambiwebserver_h
#define ambiwebserver_h

// Http Server includes
#include <AsyncTCP.h>

#if defined(ENABLE_CAM)
# define HANDLE_JPEG 0
# define HANDLE_BMP 1
# define HANDLE_MJPEG 2
# define HANDLE_BMP_GRID 3
# define HANDLE_IMG_NONE 255
# define IMAGE_TCP_TIMEOUT 5000

# define HANDLE_IMAGE_DIRECT
// # define HANDLE_BMP_GRID_DIRECT
# define RESIZE_IMAGE_PREVIEW   // Defined if Image should be resized before preview!
#endif

#include <ESPAsyncWebServer.h>
#define WEBSERVERCLASS AsyncWebServer

class AmbiWebServer {
public:
    // Constructors
    AmbiWebServer(int nPort);

    // Declare helper functions
    void StartServer(); 
    WEBSERVERCLASS* GetServer() { return pWebServer; }
    bool GetInitDone() { return m_bInit; }
    void SetLastStatusMessage(const char* szNewmessage); 
    void SetLastStatusMessage(const char* szNewmessage, int szLen); 
    char* GetLastStatusMessage(void) { return m_szLastStatusMessage; } 
    void SetUpdateError(const char* szUpdateError);
    void SetUpdateError(const char* szUpdateError, int szLen); 
    char* GetUpdateError(void) { return m_szUpdateError; } 
    void SetUpdateVersion(const char* szUpdateVersion);
    void SetUpdateVersion(const char* szUpdateVersion, int szLen); 
    char* GetUpdateVersion(void) { return m_szUpdateVersion; } 
    byte GetTaskImageMode(void);
    void handleClient();


    // Universal Pointer to WebServer Obj
    static AmbiWebServer* m_pWebServer;
private:
    // Declare private functions
    void InitServer(); 

    // Declare helper variables
    bool m_bInit;
    int m_nPort; 
    WEBSERVERCLASS* pWebServer;
    char* m_szLastStatusMessage;
    char* m_szUpdateError;
    char* m_szUpdateVersion;
};

#endif
