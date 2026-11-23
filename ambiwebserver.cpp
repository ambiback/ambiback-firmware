// Include all needed files
#include "globalincludes.h"
#include "ambiwebserver.h"
#include "ambiwebserver_handlers.h"
#include "ambiwebserver_html.h"

#include <Update.h>
#include "esp_task_wdt.h"
#include "esp_wifi.h"
#include "esp_heap_caps.h"

// Task handle for OTA (used by pages; set in constructor)
TaskHandle_t handleTaskUpdate;

#if defined(ENABLE_CAM)
#   include "camprocessor.h"
#   include <esp32cam.h>
#   include <esp_camera.h>
#   include "sensor.h"
#   include "img_converters.h"

TaskHandle_t handleTaskStream;
void TaskStreamCode(void * pvParameters);
AsyncClient* taskStreamclient;
AsyncWebServerRequest *taskStreamRequest;
byte taskImageMode = HANDLE_IMG_NONE;
esp32cam::MjpegConfig strmcfg;
# define CAMPOINTSIZE 20

/* Rate limit cam.jpg to avoid stack overflow from rapid resolution switching (OV3660) */
static unsigned long s_lastCamJpgServedMs = 0;
# define CAM_JPG_COOLDOWN_MS 1000

static const int BMP_HEADER_LEN = 54;

typedef struct {
    uint32_t filesize;
    uint32_t reserved;
    uint32_t fileoffset_to_pixelarray;
    uint32_t dibheadersize;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitsperpixel;
    uint32_t compression;
    uint32_t imagesize;
    uint32_t ypixelpermeter;
    uint32_t xpixelpermeter;
    uint32_t numcolorspallette;
    uint32_t mostimpcolor;
} bmp_header_t;

#define BOUNDARY "e8b8c539-047d-4777-a985-fbba6edff11e"

#endif

const char* PARAM_TEST PROGMEM = "test";
const char* HELLO_STR PROGMEM = "Welcome to AmbiBack Controller"; 

// Universal Pointer to UdpServer Obj
AmbiWebServer* AmbiWebServer::m_pWebServer = NULL;

# if defined(ENABLE_CAM)
typedef struct {
        camera_fb_t * fb;
        size_t index;
} camera_frame_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* STREAM_CONTENT_TYPE PROGMEM = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY PROGMEM = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART PROGMEM = "Content-Type: %s\r\nContent-Length: %u\r\n\r\n";
static const char* JPG_CONTENT_TYPE PROGMEM = "image/jpeg";

// Helper pointer for running MJPEG Stream
void* pCurrentStreamResponse;

/*  Image Helper Classes
 *   Taken from here: 
 *    https://gist.github.com/me-no-dev/d34fba51a8f059ac559bf62002e61aa3
 *    
 *    Helper class to send single JPEG Frame
 */
class AsyncFrameResponse: public AsyncAbstractResponse {
    private:
        camera_fb_t * fb;
        size_t _index;
    public:
        AsyncFrameResponse(camera_fb_t * frame, const char * contentType){
            _callback = nullptr;
            _code = 200;
            _contentLength = frame->len;
            _contentType = contentType;
            _index = 0;
            fb = frame;
        }
        ~AsyncFrameResponse(){
          if(fb != nullptr){
              esp_camera_fb_return(fb);
          }
        }
        bool _sourceValid() const { return fb != nullptr; }
        virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override{
            size_t ret = _content(buf, maxLen, _index);
            if(ret != RESPONSE_TRY_AGAIN){
                _index += ret;
            }
            return ret;
        }
        size_t _content(uint8_t *buffer, size_t maxLen, size_t index){
            memcpy(buffer, fb->buf+index, maxLen);
            if((index+maxLen) == fb->len){
                esp_camera_fb_return(fb);
                fb = nullptr;
            }
            return maxLen;
        }
};
/*
 *    Helper class to send MJPEG Stream 
 */
class AsyncJpegStreamResponse: public AsyncAbstractResponse {
    private:
        camera_frame_t _frame;
        size_t _index;
        size_t _jpg_buf_len;
        uint8_t * _jpg_buf;
        uint64_t lastAsyncRequest;

        long _startTime = 0;
        long _lastCapture = 0;
        int _minInterval = 0;       // minimum interval between frame captures.
        int _maxFrames = -1;        // maximum number of frames before disconnecting.
        int _frameTimeout = 10000;  // time limit of writing one frame in millis.
        int _nFrames = 0;           // Frames counter
        int _nNullFrames = 0;       // Nullframes counter
    public:
        bool _bAbort = false;       // Abort flags
        AsyncJpegStreamResponse(int minInterval, int maxFrames, int frameTimeout){
            _callback = nullptr;
            _code = 200;
            _contentLength = 0;
            _contentType = STREAM_CONTENT_TYPE;
            _sendContentLength = false;
            _chunked = true;
            _index = 0;
            _jpg_buf_len = 0;
            _jpg_buf = NULL;
            lastAsyncRequest = 0;
            _minInterval = minInterval;
            _maxFrames = maxFrames;
            _frameTimeout = frameTimeout;
            _startTime = millis();
            
            memset(&_frame, 0, sizeof(camera_frame_t));
        }
        ~AsyncJpegStreamResponse(){
          long duration = millis() - _startTime;
          if(_frame.fb){
              if(_frame.fb->format != PIXFORMAT_JPEG){
                  free(_jpg_buf);
              }
              esp_camera_fb_return(_frame.fb);
          }
          
          _DBGPRINTF(PSTR("AsyncJpegStreamResponse: STREAM END , %dfrm %0.2ffps\n"), _nFrames, 1000.0 * _nFrames / duration);

          if (pCurrentStreamResponse == this) {
            pCurrentStreamResponse = NULL;
            _DBGPRINTF(PSTR("AsyncJpegStreamResponse: STREAM END !!!!!!!!!!!!\n"));
          }
        }
        bool _sourceValid() const {
            return true;
        }
        virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override {
            size_t ret = _content(buf, maxLen, _index);
            if(ret != RESPONSE_TRY_AGAIN){
                _index += ret;
            }
            return ret;
        }
        size_t _content(uint8_t *buffer, size_t maxLen, size_t index){
          // Check for sleeptime
            long timeSinceLastCapture = millis() - _lastCapture;
            if (_minInterval > 0 && _lastCapture > 0 && static_cast<int>(timeSinceLastCapture) < _minInterval) {
              return RESPONSE_TRY_AGAIN;
            }

            // Abort Streaming if canceled from outside!
            if (_bAbort) {
              return 0;
            }

            // Start frameprocessing
            if(!_frame.fb || _frame.index == _jpg_buf_len){
                if(index && _frame.fb){
                    uint64_t end = (uint64_t)micros();
                    int fp = (end - lastAsyncRequest) / 1000;
                    _DBGPRINTF_ULTRA(PSTR("AsyncJpegStreamResponse: Size: %uKB, Time: %ums (%.1ffps)\n"), _jpg_buf_len/1024, fp);
                    lastAsyncRequest = end;
                    if(_frame.fb->format != PIXFORMAT_JPEG){
                        free(_jpg_buf);
                    }
                    esp_camera_fb_return(_frame.fb);
                    _frame.fb = NULL;
                    _jpg_buf_len = 0;
                    _jpg_buf = NULL;
                }
                if(maxLen < (strlen(STREAM_BOUNDARY) + strlen(STREAM_PART) + strlen(JPG_CONTENT_TYPE) + 8)){
                    //log_w("Not enough space for headers");
                    return RESPONSE_TRY_AGAIN;
                }
                
                // Abort if frame counter is reached
                if (_maxFrames > 0 && _nFrames > _maxFrames) {
                  _DBGPRINTF(PSTR("AsyncJpegStreamResponse: maxFrames %d reached "), _maxFrames);
                  return 0;
                }

                //get frame
                _frame.index = 0;

                // Capture Single image
                if ( global::m_pCamProcessor->camMutexLock()) {
                  _frame.fb = esp_camera_fb_get();
                  global::m_pCamProcessor->camMutexRelease();
                } else {
                  global::errprintf_P(PSTR("AsyncJpegStreamResponse: Error, Camera capture failed - MUTEX STILL LOCKED!\n"));
                  return 0;
                }

                if (_frame.fb == NULL) {
                    _DBGPRINTF(PSTR("AsyncJpegStreamResponse: Camera frame failed"));
                    return 0;
                } else {
                  _nFrames++;
                }

                if(_frame.fb->format != PIXFORMAT_JPEG){
                    unsigned long st = millis();
                    bool jpeg_converted = frame2jpg(_frame.fb, 80, &_jpg_buf, &_jpg_buf_len);
                    if(!jpeg_converted){
                        log_e("JPEG compression failed");
                        esp_camera_fb_return(_frame.fb);
                        _frame.fb = NULL;
                        _jpg_buf_len = 0;
                        _jpg_buf = NULL;
                        return 0;
                    }
                    log_i("JPEG: %lums, %uB", millis() - st, _jpg_buf_len);
                } else {
                    _jpg_buf_len = _frame.fb->len;
                    _jpg_buf = _frame.fb->buf;
                }

                //send boundary
                size_t blen = 0;
                if(index){
                    blen = strlen(STREAM_BOUNDARY);
                    memcpy(buffer, STREAM_BOUNDARY, blen);
                    buffer += blen;
                }
                //send header
                size_t hlen = sprintf((char *)buffer, STREAM_PART, JPG_CONTENT_TYPE, _jpg_buf_len);
                buffer += hlen;
                //send frame
                hlen = maxLen - hlen - blen;
                if(hlen > _jpg_buf_len){
                    maxLen -= hlen - _jpg_buf_len;
                    hlen = _jpg_buf_len;
                }
                memcpy(buffer, _jpg_buf, hlen);
                _frame.index += hlen;

                
                return maxLen;
            }

            size_t available = _jpg_buf_len - _frame.index;
            if(maxLen > available){
                maxLen = available;
            }
            memcpy(buffer, _jpg_buf+_frame.index, maxLen);
            _frame.index += maxLen;

            // Set last Capture time
            _lastCapture = millis();

            return maxLen;
        }
};
# endif

// Start Class Functions
AmbiWebServer::AmbiWebServer(int nPort){
  m_bInit = false; 
  m_nPort = nPort; 
  pWebServer = new WEBSERVERCLASS(m_nPort);
  m_szLastStatusMessage = NULL;
  m_szUpdateError = NULL;
  m_szUpdateVersion = NULL;
  AmbiWebServer::m_pWebServer = this;
  
  // May needed 
//  DefaultHeaders::Instance().addHeader( PSTR("Access-Control-Allow-Origin"), "*");  
//  DefaultHeaders::Instance().addHeader( PSTR("Access-Control-Allow-Headers"), PSTR("Accept, Accept-Language,  Content-Language, Content-Type, X-Requested-With"));  

#if defined(ESP32)
  handleTaskUpdate = NULL; 
# if defined(ENABLE_CAM)
  handleTaskStream = NULL; 
  taskStreamclient = NULL; 
  taskStreamRequest = NULL;
  pCurrentStreamResponse = NULL;
  taskImageMode = HANDLE_IMG_NONE;
# endif
#endif
}

void AmbiWebServer::SetLastStatusMessage(const char* szNewmessage) {
  this->SetLastStatusMessage(szNewmessage, (szNewmessage != NULL ? strlen(szNewmessage) : 0) ); 
}
void AmbiWebServer::SetLastStatusMessage(const char* szNewmessage, int szLen) {
  // Delete old mem
  if (m_szLastStatusMessage != NULL) { 
    delete[] m_szLastStatusMessage; 
    m_szLastStatusMessage = NULL;
  }
  // Fill new Status message if set
  if (szNewmessage != NULL) {
    if ( szLen > (MESSAGE_MAX_STRING-1)) 
      szLen = (MESSAGE_MAX_STRING-1); // Limit to max size
    
    m_szLastStatusMessage = new char[szLen + 1];
    memcpy(m_szLastStatusMessage, szNewmessage, szLen);
    *(m_szLastStatusMessage+szLen) = '\0';
  }
}

void AmbiWebServer::SetUpdateError(const char* szUpdateError) {
  this->SetUpdateError(szUpdateError, (szUpdateError != NULL ? strlen(szUpdateError) : 0) ); 
}
void AmbiWebServer::SetUpdateError(const char* szUpdateError, int szLen) {
  // Delete old mem
  if (m_szUpdateError != NULL) { 
    delete[] m_szUpdateError; 
    m_szUpdateError = NULL;
  }
  // Fill new Status message if set
  if (szUpdateError != NULL) {
    if ( szLen > (MESSAGE_MAX_STRING-1)) 
      szLen = (MESSAGE_MAX_STRING-1); // Limit to max size
    
    m_szUpdateError = new char[szLen + 1];
    memcpy(m_szUpdateError, szUpdateError, szLen);
    *(m_szUpdateError+szLen) = '\0';
  }
}

void AmbiWebServer::SetUpdateVersion(const char* szUpdateVersion) {
  this->SetUpdateVersion(szUpdateVersion, (szUpdateVersion != NULL ? strlen(szUpdateVersion) : 0) ); 
}
void AmbiWebServer::SetUpdateVersion(const char* szUpdateVersion, int szLen) {
  // Delete old mem
  if (m_szUpdateVersion != NULL) { 
    delete[] m_szUpdateVersion; 
    m_szUpdateVersion = NULL;
  }
  // Fill new Status message if set
  if (szUpdateVersion != NULL) {
    if ( szLen > (MESSAGE_MAX_STRING-1)) 
      szLen = (MESSAGE_MAX_STRING-1); // Limit to max size
    
    m_szUpdateVersion = new char[szLen + 1];
    memcpy(m_szUpdateVersion, szUpdateVersion, szLen);
    *(m_szUpdateVersion+szLen) = '\0';
  }
}

/**
 * This is lets-encrypt-x3-cross-signed.pem
 */
const char* rootCACertificate PROGMEM = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIEkjCCA3qgAwIBAgIQCgFBQgAAAVOFc2oLheynCDANBgkqhkiG9w0BAQsFADA/\n" \
"MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n" \
"DkRTVCBSb290IENBIFgzMB4XDTE2MDMxNzE2NDA0NloXDTIxMDMxNzE2NDA0Nlow\n" \
"SjELMAkGA1UEBhMCVVMxFjAUBgNVBAoTDUxldCdzIEVuY3J5cHQxIzAhBgNVBAMT\n" \
"GkxldCdzIEVuY3J5cHQgQXV0aG9yaXR5IFgzMIIBIjANBgkqhkiG9w0BAQEFAAOC\n" \
"AQ8AMIIBCgKCAQEAnNMM8FrlLke3cl03g7NoYzDq1zUmGSXhvb418XCSL7e4S0EF\n" \
"q6meNQhY7LEqxGiHC6PjdeTm86dicbp5gWAf15Gan/PQeGdxyGkOlZHP/uaZ6WA8\n" \
"SMx+yk13EiSdRxta67nsHjcAHJyse6cF6s5K671B5TaYucv9bTyWaN8jKkKQDIZ0\n" \
"Z8h/pZq4UmEUEz9l6YKHy9v6Dlb2honzhT+Xhq+w3Brvaw2VFn3EK6BlspkENnWA\n" \
"a6xK8xuQSXgvopZPKiAlKQTGdMDQMc2PMTiVFrqoM7hD8bEfwzB/onkxEz0tNvjj\n" \
"/PIzark5McWvxI0NHWQWM6r6hCm21AvA2H3DkwIDAQABo4IBfTCCAXkwEgYDVR0T\n" \
"AQH/BAgwBgEB/wIBADAOBgNVHQ8BAf8EBAMCAYYwfwYIKwYBBQUHAQEEczBxMDIG\n" \
"CCsGAQUFBzABhiZodHRwOi8vaXNyZy50cnVzdGlkLm9jc3AuaWRlbnRydXN0LmNv\n" \
"bTA7BggrBgEFBQcwAoYvaHR0cDovL2FwcHMuaWRlbnRydXN0LmNvbS9yb290cy9k\n" \
"c3Ryb290Y2F4My5wN2MwHwYDVR0jBBgwFoAUxKexpHsscfrb4UuQdf/EFWCFiRAw\n" \
"VAYDVR0gBE0wSzAIBgZngQwBAgEwPwYLKwYBBAGC3xMBAQEwMDAuBggrBgEFBQcC\n" \
"ARYiaHR0cDovL2Nwcy5yb290LXgxLmxldHNlbmNyeXB0Lm9yZzA8BgNVHR8ENTAz\n" \
"MDGgL6AthitodHRwOi8vY3JsLmlkZW50cnVzdC5jb20vRFNUUk9PVENBWDNDUkwu\n" \
"Y3JsMB0GA1UdDgQWBBSoSmpjBH3duubRObemRWXv86jsoTANBgkqhkiG9w0BAQsF\n" \
"AAOCAQEA3TPXEfNjWDjdGBX7CVW+dla5cEilaUcne8IkCJLxWh9KEik3JHRRHGJo\n" \
"uM2VcGfl96S8TihRzZvoroed6ti6WqEBmtzw3Wodatg+VyOeph4EYpr/1wXKtx8/\n" \
"wApIvJSwtmVi4MFU5aMqrSDE6ea73Mj2tcMyo5jMd6jmeWUHK8so/joWUoHOUgwu\n" \
"X4Po1QYz+3dszkDqMp4fklxBwXRsW10KXzPMTZ+sOPAveyxindmjkW8lGy+QsRlG\n" \
"PfZ+G6Z6h7mjem0Y+iWlkYcV4PIWL1iwBi8saCbGS5jN2p8M+X+Q7UNKEkROb3N6\n" \
"KOqkqm57TH2H3eDJAkSnh6/DNFu0Qg==\n" \
"-----END CERTIFICATE-----\n";

void AmbiWebServer::handleClient() {
  _yield(); 
}

void AmbiWebServer::StartServer() {
  if (!m_bInit) 
    this->InitServer(); 
    
  // Starts WebServer
  pWebServer->begin();
  _DBGPRINTF(PSTR("WebServer: Successfully started Web Server on Port: %d\n"), m_nPort);
}

void AmbiWebServer::InitServer() {
  // Init HTTP URL handlers 
  DefaultHeaders::Instance().addHeader(PSTR("Access-Control-Allow-Origin"), PSTR("*"));
  DefaultHeaders::Instance().addHeader(PSTR("Access-Control-Allow-Headers"), PSTR("Content-Type, Accept, X-Requested-With"));
  DefaultHeaders::Instance().addHeader(PSTR("Access-Control-Allow-Methods"), PSTR("GET, POST, OPTIONS"));

  // Ambient Commands
  pWebServer->on(PSTR("/setcolormode"), HTTP_GET, &handleSetColorMode);   // Sets ColorMode manually 
  pWebServer->on(PSTR("/setcolor"), HTTP_GET, &handleSetColor); // Sets single Color
  pWebServer->on(PSTR("/setaction"), HTTP_GET, &handleSetAction); // Do whatever action you want to do!

  // Set Funktions
  pWebServer->on(PSTR("/setparam"), HTTP_GET, &handleSetParameter); // Handle Set Parameters
  pWebServer->on(PSTR("/setpreset"), HTTP_GET, &handleSetParameterPresets); // Handle Presets


#if defined(ENABLE_CAM)
  // Cam related 
  pWebServer->on(PSTR("/cam-grid.bmp"), HTTP_GET, &handleCamBmpGrid);
  pWebServer->on(PSTR("/cam.bmp"), HTTP_GET, &handleCamBmp);
  pWebServer->on(PSTR("/cam.jpg"), HTTP_GET, &handleCamJpg);
  pWebServer->on(PSTR("/cam.mjpeg"), HTTP_GET, &handleCamMjpeg);
  pWebServer->on(PSTR("/setcamerapoint"), HTTP_GET, &handleSetCameraPoint); // Handle Set Point!

  // DEBUG / TESTING
  pWebServer->on(PSTR("/setcamclck"), HTTP_GET, &handleCamClck); // Handle Cam Clock Settings


#endif


  // Static JSON URLs
  pWebServer->on(PSTR("/getparamatersasjson"), HTTP_GET, &handleGetParametersAsJson);               // Get all the Parameters as JSON String
  pWebServer->on(PSTR("/getstatusasjson"), HTTP_GET, &handleGetStatusAsJson);                       // Get all the Status vars as JSON String
# if defined(ENABLE_WIFI_AP)
  pWebServer->on(PSTR("/getprovisioningasjson"), HTTP_GET, &handleGetProvisioningAsJson);           // Get provisioning state as JSON String
# endif
  pWebServer->on(PSTR("/discover"), HTTP_GET, &handleDiscover);                                     // UDP discovery for other AmbiBack devices
  pWebServer->on(PSTR("/getscannetworksasjson"), HTTP_GET, &handleGetScanNetworksAsJson);           // Return all the WIFI Networks as JSON String
  pWebServer->on(PSTR("/getlaststatusmessageasjson"), HTTP_GET, &handleGetLastStatusMessageAsJson); // Get last status message as JSON String
  pWebServer->on(PSTR("/ambiback.webmanifest.json"), HTTP_GET, &handleAmbiBackWebManifest );        // Return WebManifest as JSON String 
# if defined(ENABLE_WIFI_AP)
  pWebServer->on(PSTR("/provisioning/start"), HTTP_GET, &handleProvisioningStart);
  pWebServer->on(PSTR("/provisioning/submit"), HTTP_POST, &handleProvisioningSubmit);
  pWebServer->on(PSTR("/provisioning/finish"), HTTP_POST, &handleProvisioningFinish);
  pWebServer->on(PSTR("/provisioning/claim"), HTTP_POST, &handleProvisioningClaim);
# endif

  // Static ACTIONs
  pWebServer->on(PSTR("/"), &handleIndex);
  pWebServer->on(data_index_html_path, &handleIndex);
  pWebServer->on(PSTR("/ui"), HTTP_GET, [] (AsyncWebServerRequest *request){
    request->redirect(PSTR("https://ambiback.github.io/ambiback-ui/"));
  });
  pWebServer->on(PSTR("/reboot"), HTTP_GET, &handleReboot);
  pWebServer->on(PSTR("/startwps"), HTTP_GET, &handleWifiWps);

#if !defined(EMBED_MINIMAL_UI)
#if ESP32_DEVICE_TYPE != ESP32_BRIDGE
  pWebServer->on(data_include_ambient_html_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_include_ambient_html_path);
    request->send_P(200, TEXT_HTML, (const uint8_t *)data_include_ambient_html, DATA_INCLUDE_AMBIENT_HTML_LEN);
  });
#endif

  // HANDLE OWN CSS/JAVASCRIPT/IMG
  pWebServer->on(PSTR("/controllersettings.js"), &handleControllersettings);
  pWebServer->on(data_include_ambiback_script_js_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_include_ambiback_script_js_path);
    request->send_P(200, TEXT_JAVASCRIPT, (const uint8_t *)data_include_ambiback_script_js, DATA_INCLUDE_AMBIBACK_SCRIPT_JS_LEN);
  });
  pWebServer->on(data_include_ambiback_styles_css_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_include_ambiback_styles_css_path);
    request->send_P(200, TEXT_CSS, (const uint8_t *)data_include_ambiback_styles_css, DATA_INCLUDE_AMBIBACK_STYLES_CSS_LEN);
  });
  pWebServer->on(data_include_ambiback_worker_js_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_include_ambiback_worker_js_path);
    request->send_P(200, TEXT_JAVASCRIPT, (const uint8_t *)data_include_ambiback_worker_js, DATA_INCLUDE_AMBIBACK_WORKER_JS_LEN);
  });
  pWebServer->on(data_images_app_logo_png_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_images_app_logo_png_path);
    request->send_P(200, IMAGE_PNG, (const uint8_t *)data_images_app_logo_png, DATA_IMAGES_APP_LOGO_PNG_LEN);
  });

#if defined(ENABLE_CAM)
  pWebServer->on(data_include_ambiback_script_camera_js_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_include_ambiback_script_camera_js_path);
    request->send_P(200, TEXT_JAVASCRIPT, (const uint8_t *)data_include_ambiback_script_camera_js, DATA_INCLUDE_AMBIBACK_SCRIPT_CAMERA_JS_LEN);
  });
  pWebServer->on(data_include_camera_html_path, &handleIncludeCamera);
#endif

#if defined(ENABLE_HUE)
  pWebServer->on(data_include_hue_html_path, &handleIncludeHue);
  pWebServer->on(data_include_ambiback_script_hue_js_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_include_ambiback_script_hue_js_path);
    request->send_P(200, TEXT_JAVASCRIPT, (const uint8_t *)data_include_ambiback_script_hue_js, DATA_INCLUDE_AMBIBACK_SCRIPT_HUE_JS_LEN);
  });
  pWebServer->on(data_include_jshue_js_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_include_jshue_js_path);
    request->send_P(200, TEXT_JAVASCRIPT, (const uint8_t *)data_include_jshue_js, DATA_INCLUDE_JSHUE_JS_LEN);
  });
#endif

#if defined(ENABLE_DREAMSCREEN)
  pWebServer->on(data_include_dreamscreen_html_path, &handleIncludeDreamScreen);
  pWebServer->on(data_include_ambiback_script_dreamscreen_js_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_include_ambiback_script_dreamscreen_js_path);
    request->send_P(200, TEXT_JAVASCRIPT, (const uint8_t *)data_include_ambiback_script_dreamscreen_js, DATA_INCLUDE_AMBIBACK_SCRIPT_DREAMSCREEN_JS_LEN);
  });
#endif

#if defined(HTTP_GOOGLE_JQUERYFILES)
  pWebServer->on(data_jquery_min_js_path, HTTP_GET, [] (AsyncWebServerRequest *request){
  //    request->redirect(PSTR("https://ajax.googleapis.com/ajax/libs/jquery/1.10.2/jquery.min.js")); // 3.2.1 not working with jquery.mobile
    request->redirect(PSTR("https://cdnjs.cloudflare.com/ajax/libs/jquery/1.10.2/jquery.min.js")); // 3.2.1 not working with jquery.mobile
  });
  pWebServer->on(data_jquery_mobile_min_css_path, HTTP_GET, [] (AsyncWebServerRequest *request) {
  //    request->redirect(PSTR("https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.css"));
    request->redirect(PSTR("https://cdnjs.cloudflare.com/ajax/libs/jquery-mobile/1.4.5/jquery.mobile.min.css"));
  });
  pWebServer->on(PSTR("/jquery.mobile.min.map"), HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->redirect(PSTR("https://cdnjs.cloudflare.com/ajax/libs/jquery-mobile/1.4.5/jquery.mobile.min.map"));
  });
  pWebServer->on(data_jquery_mobile_min_js_path, HTTP_GET, [] (AsyncWebServerRequest *request) {
  //    request->redirect(PSTR("https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.js"));
    request->redirect(PSTR("https://cdnjs.cloudflare.com/ajax/libs/jquery-mobile/1.4.5/jquery.mobile.min.js"));
  });
  pWebServer->on(PSTR("/images/ajax-loader.gif"), HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->redirect(PSTR("https://cdnjs.cloudflare.com/ajax/libs/jquery-mobile/1.4.5/images/ajax-loader.gif"));
  });
#else
  // HANDLE CSS/JAVASCRIPT
  pWebServer->on(data_jquery_min_js_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_jquery_min_js_path);
    request->send_P(200, TEXT_JAVASCRIPT, (const uint8_t *)data_jquery_min_js, DATA_JQUERY_MIN_JS_LEN);
  });
  // HANDLE CSS for JQUERY UI MOBILE
  pWebServer->on(data_jquery_mobile_min_css_path, HTTP_GET, [] (AsyncWebServerRequest *request) {
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_jquery_ui_css_path);
    request->send_P(200, TEXT_CSS, (const uint8_t *)data_jquery_mobile_min_css, DATA_JQUERY_MOBILE_MIN_CSS_LEN);
  });
  pWebServer->on(data_jquery_mobile_min_js_path, HTTP_GET, [] (AsyncWebServerRequest *request) {
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_jquery_mobile_min_js_path);
    request->send_P(200, TEXT_JAVASCRIPT, (const uint8_t *)data_jquery_mobile_min_js, DATA_JQUERY_MOBILE_MIN_JS_LEN);
  });
  // HANDLE IMAGE FILES
  pWebServer->on(data_images_ui_icons_222222_256x240_png_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_images_ui_icons_222222_256x240_png_path);
    request->send_P(200, IMAGE_PNG, (const uint8_t *)data_images_ui_icons_222222_256x240_png, DATA_IMAGES_UI_ICONS_222222_256X240_PNG_LEN);
  });
  pWebServer->on(data_images_ui_icons_4b8e0b_256x240_png_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_images_ui_icons_4b8e0b_256x240_png_path);
    request->send_P(200, IMAGE_PNG, (const uint8_t *)data_images_ui_icons_4b8e0b_256x240_png, DATA_IMAGES_UI_ICONS_4B8E0B_256X240_PNG_LEN);
  });
  pWebServer->on(data_images_ui_icons_a83300_256x240_png_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_images_ui_icons_a83300_256x240_png_path);
    request->send_P(200, IMAGE_PNG, (const uint8_t *)data_images_ui_icons_a83300_256x240_png, DATA_IMAGES_UI_ICONS_A83300_256X240_PNG_LEN);
  });
  pWebServer->on(data_images_ui_icons_cccccc_256x240_png_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_images_ui_icons_cccccc_256x240_png_path);
    request->send_P(200, IMAGE_PNG, (const uint8_t *)data_images_ui_icons_cccccc_256x240_png, DATA_IMAGES_UI_ICONS_CCCCCC_256X240_PNG_LEN);
  });
  pWebServer->on(data_images_ui_icons_ffffff_256x240_png_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_images_ui_icons_ffffff_256x240_png_path);
    request->send_P(200, IMAGE_PNG, (const uint8_t *)data_images_ui_icons_ffffff_256x240_png, DATA_IMAGES_UI_ICONS_FFFFFF_256X240_PNG_LEN);
  });
  // Include Loader GFX!
  pWebServer->on(PSTR("/images/ajax-loader.gif"), HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), "/images/ajax_loader.gif" /*data_images_ajax_loader_gif_path*/);
    request->send_P(200, IMAGE_GIF, (const uint8_t *)data_images_ajax_loader_gif, DATA_IMAGES_AJAX_LOADER_GIF_LEN);
  });
#endif

#if defined(ENABLE_CAM)
# if defined(HTTP_GOOGLE_JQUERYFILES)
/*
  pWebServer->on(data_jquery_ui_draggable_js_path, HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->redirect(PSTR("https://cdnjs.cloudflare.com/ajax/libs/jqueryui/1.10.3/jquery.ui.draggable.min.js"));
  });
  pWebServer->on(data_jquery_ui_touch_punch_min_js_path, HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->redirect(PSTR("https://cdnjs.cloudflare.com/ajax/libs/jqueryui-touch-punch/0.2.2/jquery.ui.touch-punch.min.js"));
  });
*/
  // FORCE LOCAL ATM DOESNT WORK FROM CDN!
  pWebServer->on(data_jquery_ui_draggable_js_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_jquery_ui_draggable_js_path);
    request->send_P(200, TEXT_JAVASCRIPT, (const uint8_t *)data_jquery_ui_draggable_js, DATA_JQUERY_UI_DRAGGABLE_JS_LEN);
  });
  pWebServer->on(data_jquery_ui_touch_punch_min_js_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_jquery_ui_touch_punch_min_js_path);
    request->send_P(200, TEXT_JAVASCRIPT, (const uint8_t *)data_jquery_ui_touch_punch_min_js, DATA_JQUERY_UI_TOUCH_PUNCH_MIN_JS_LEN);
  });
# else
  pWebServer->on(data_jquery_ui_draggable_js_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_jquery_ui_draggable_js_path);
    request->send_P(200, TEXT_JAVASCRIPT, (const uint8_t *)data_jquery_ui_draggable_js, DATA_JQUERY_UI_DRAGGABLE_JS_LEN);
  });
  pWebServer->on(data_jquery_ui_touch_punch_min_js_path, HTTP_GET, [] (AsyncWebServerRequest *request){
    _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_jquery_ui_touch_punch_min_js_path);
    request->send_P(200, TEXT_JAVASCRIPT, (const uint8_t *)data_jquery_ui_touch_punch_min_js, DATA_JQUERY_UI_TOUCH_PUNCH_MIN_JS_LEN);
  });
# endif
#endif
#endif
#if defined(ENABLE_HUE)
  pWebServer->on(PSTR("/controllersettings_hue.js"), &handleControllerSettingsHue);
  pWebServer->on(PSTR("/refresh_hue_devices"), &handleRefreshHueDevices);
#endif
#if defined(ENABLE_CAM)
  pWebServer->on(PSTR("/controllersettings_camera.js"), &handleControllersettingsCamera);
#endif
#if defined(ENABLE_DREAMSCREEN)
  pWebServer->on(PSTR("/controllersettings_dreamscreen.js"), &handleControllerSettingsDreamScreen);
  pWebServer->on(PSTR("/refresh_dreamscreen_devices"), &handleRefreshDreamScreenDevices);
  pWebServer->on(PSTR("/dreamscreenaddtonetwork"), &handleAddDreamScreenToNetwork);
#endif
  /* --- --- */
  
  // Send a GET request to <IP>/get?message=<message>
  pWebServer->on(PSTR("/get"), HTTP_GET, [] (AsyncWebServerRequest *request) {
      String message;
      if (request->hasParam(PARAM_TEST)) {
          message = request->getParam(PARAM_TEST)->value();
      } else {
          message = "No message sent";
      }
      request->send(200, TEXT_PLAIN, PSTR("Hello, GET: ") + message);
  });

  // Send a POST request to <IP>/post with a form field message set to <message>
  pWebServer->on(PSTR("/post"), HTTP_POST, [](AsyncWebServerRequest *request){
      String message;
      if (request->hasParam(PARAM_TEST, true)) {
          message = request->getParam(PARAM_TEST, true)->value();
      } else {
          message = "No message sent";
      }
      request->send(200, TEXT_PLAIN, PSTR("Hello, POST: ") + message);
  });

  /* SAMPLE CODE FROM: https://github.com/me-no-dev/ESPAsyncWebServer#setting-up-the-server */
  /* Handling uploading firmware file */
  pWebServer->on(PSTR("/update"), HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, TEXT_HTML, PSTR( "<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><body style=\"padding:5px;\"><h2>Firmware Update</h2>"
                                        "<form method=\"POST\" action=\"/update\" enctype=\"multipart/form-data\" onsubmit=\"var k=document.getElementById('k').value;if(k)this.action='/update?claimkey='+encodeURIComponent(k)\"><input id=\"k\" type=\"password\" placeholder=\"Device key (if claimed)\" autocomplete=\"off\"><br><input type=\"file\" name=\"update\"><input type=\"submit\" value=\"Update\"></form>"
                                        "<br><center><a href=\"/\"><b>return to index</b></a></center></body></html>"));
  });
  
  pWebServer->on(PSTR("/update"), HTTP_POST, &handleUpdate, &handleUpdateUpload );
  pWebServer->on(PSTR("/autoupdate"), HTTP_GET, &handleAutoUpdate );
  pWebServer->on(PSTR("/getfirmwareindex"), HTTP_GET, &handleGetFirmwareIndex );
  pWebServer->on(PSTR("/updatestatus"), HTTP_GET, &handleUpdateStatus );

  // Set 404 message
  pWebServer->onNotFound(&notFound);          

  m_bInit = true; 
}

// --- Page/OTA/Hue/DreamScreen handlers moved to ambiwebserver_pages.cpp ---
// --- API/parameter handlers moved to ambiwebserver_api.cpp ---

#if defined(ENABLE_CAM)
byte AmbiWebServer::GetTaskImageMode(void) {
  return taskImageMode;
}


// --- Static Callbacks
void handleCamClck(AsyncWebServerRequest *request) {
  String strResult; 
  String strMul; 
  String strPre;
  String strPclk; 
  _DBGPRINTF(PSTR("WebServer: handleCamClck: ENTER \n"));

  if (request->hasParam("mul") && 
      request->hasParam("pre") && 
      request->hasParam("pclk")) {
    strMul = request->getParam("mul")->value();
    strPre = request->getParam("pre")->value();
    strPclk = request->getParam("pclk")->value();

    /* Convert to INT */ 
    int iMul = strMul.toInt(); 
    int iPre = strPre.toInt(); 
    int iPclk = strPclk.toInt(); 
    _DBGPRINTF(PSTR("WebServer: handleCamClck: values=%d/%d/%d\n"), iMul, iPre, iPclk);

    global::InitCamClck(iMul,iPre,iPclk);
    strResult = PSTR("Set mul ") + strMul + PSTR(", pre ") + strPre + PSTR(", pclk") + strPclk + PSTR(" ");
  } else {
    // String to short
    strResult = PSTR("ERROR, invalid values, empty or to short");
  }
  
  _DBGPRINTF(PSTR("WebServer: handleCamClck: %s\n"), strResult.c_str());
  request->send(200, TEXT_PLAIN, request->url() + ": " + strResult);  
}

void handleSetCameraPoint(AsyncWebServerRequest *request) {
  String strResult; 
  String strPoint; 
  String strLeft;
  String strTop; 
  _DBGPRINTF(PSTR("WebServer: handleSetCameraPoint: ENTER \n"));

  if (request->hasParam("point") && 
      request->hasParam("left") && 
      request->hasParam("top")) {
    strPoint = request->getParam("point")->value();
    strLeft = request->getParam("left")->value();
    strTop = request->getParam("top")->value();
    _DBGPRINTF(PSTR("WebServer: handleSetCameraPoint: values=%s/%s/%s\n"), strPoint.c_str(),strLeft.c_str(), strTop.c_str());
    
    // Set Camera Point value!
    if (global::m_pLedHelper != NULL) {
      /* Convert to INT */ 
      int iPoint = strPoint.toInt(); 
      int iLeft = strLeft.toInt(); 
      int iTop = strTop.toInt(); 

      /* check range */
      if (  iPoint >= 1 && 
            iPoint <= 4 && 
            iLeft >= 0 && 
            iLeft <= 100 && 
            iTop >= 0 && 
            iTop <= 100 ) {
        global::m_pCamProcessor->setCameraPoint((byte)iPoint, (short)iLeft & 0xffff, (short)iTop & 0xffff); 
        strResult = PSTR("Successfully set point ") + strPoint + PSTR(" to x ") + strLeft + PSTR(" y ") + strTop+ PSTR(" ");
      } else {
        strResult = PSTR("ERROR, invalid values");
      }      
    } else {
      strResult = PSTR("INTERNAL ERROR, LED Handler not set.");
    }
  } else {
    // String to short
    strResult = PSTR("ERROR, invalid values, empty or to short");
  }
  
  _DBGPRINTF(PSTR("WebServer: handleSetCameraPoint: %s\n"), strResult.c_str());
  request->send(200, TEXT_PLAIN, request->url() + ": " + strResult);  
}

void DeliverBmpGrid(AsyncWebServerRequest* request) {
  /* Mark pixels in BMP data if CamProcessor geometry is ready. */
  if (global::m_pCamProcessor != NULL &&
      global::m_pCamProcessor->GetCamPoints() != NULL &&
      global::m_pCamProcessor->GetRectAreas() != NULL) {
    // Helper variables 
    uint8_t* uidata = new uint8_t[DATA_MAX_BUFFER];
    uint8_t* uitmp = uidata;
    size_t tzdatapos = 0;
    size_t tzsend = 0;

    int iJpgScale = 0;
    int iPixMulti = 1;
    int realWidth = global::m_pCamProcessor->GetRealFrameWidth();
    int realHeight = global::m_pCamProcessor->GetRealFrameHeight();
    int processingWidth = realWidth;   // Needed for BitIndex MACRO
    int processingHeight = realHeight; // Needed for BitIndex MACRO

    // Helper function to calc Jpeg Resolution Compression
    global::m_pCamProcessor->camSetJpegProperties(realWidth, &iJpgScale, &iPixMulti, &processingWidth, &processingHeight);

    // Bitindex helpers
    int lev1size = processingWidth*processingHeight;
    int lev2size = processingWidth;
    rectanglepoints* rectAreas = global::m_pCamProcessor->GetRectAreas();
    
    // More Helper variables
    int iPixCount = processingWidth * processingHeight;
    int iPixBytes = iPixCount * 3;
    int iLineBytes = processingWidth * 3;
    int iPixMarked = 0;
    int iStart = 0;
    int iMaxStep = 1;
    int iStep = 1; 
    if (global::m_pCamProcessor->GetCamMode() == LED_MODE_CAM_BAR) {
      iStart = 1; /* Need to start at index 1 in BAR Mode */
      iMaxStep = CAM_SECTOR_COUNT(global::m_pCamProcessor->GetCamDetailLevel());
      iStep = global::m_pCamProcessor->GetCamDetailLevel();
    } else if (global::m_pCamProcessor->GetCamMode() == LED_MODE_CAM_DETAIL) {
      iStart = 1; /* Need to start at index 1 in DETAIL Mode */
      iMaxStep = CAM_SECTOR_COUNT(global::m_pCamProcessor->GetCamDetailLevel());
    }

    // Only if buffer is sufficient
    if (DATA_MAX_BUFFER > iLineBytes) {
      // Create BMP Header for RGB888 
      *uitmp++ = 'B';
      *uitmp++ = 'M';
      bmp_header_t * bitmapHeader  = (bmp_header_t*) uitmp;
      bitmapHeader->reserved = 0;
      bitmapHeader->filesize = (iPixBytes + BMP_HEADER_LEN);
      bitmapHeader->fileoffset_to_pixelarray = BMP_HEADER_LEN;
      bitmapHeader->dibheadersize = 40;
      bitmapHeader->width = processingWidth;
      bitmapHeader->height = -(processingHeight);//set negative for top to bottom
      bitmapHeader->planes = 1;
      bitmapHeader->bitsperpixel = 24;
      bitmapHeader->compression = 0;
      bitmapHeader->imagesize = iPixBytes;
      bitmapHeader->ypixelpermeter = 0x0B13 ; //2835 , 72 DPI
      bitmapHeader->xpixelpermeter = 0x0B13 ; //2835 , 72 DPI
      bitmapHeader->numcolorspallette = 0;
      bitmapHeader->mostimpcolor = 0;
      
      // Begin WebRespone for image
      AsyncWebServerResponse *response = request->beginResponse(200); 
  
      // Create response to client
      response->setContentLength(bitmapHeader->filesize);
      response->setCode(200);
      response->setContentType("image/bmp");

      // Get direct access on TCP Client
      AsyncClient* client = request->client();
      client->setRxTimeout(IMAGE_TCP_TIMEOUT);

      // Write BMP Header out!
      tzsend = (client->connected() ? client->write((const char*) uidata, BMP_HEADER_LEN) : -1);
      _DBGPRINTF(PSTR("DeliverBmpGrid: Successfully started BMP-GRID image to client with %d bytes.\n"), tzsend, iPixMarked, iPixCount);
     
      // Loop height/width/pixel and mark active PIXELS
      int nPixelHit = -1;
      for(int v=0; v < processingHeight; v++) {
        if (tzdatapos+iLineBytes > DATA_MAX_BUFFER) {
          int iWait = 0;
          while( (!client->canSend() || client->space() < DATA_MAX_BUFFER) && iWait < IMG_WAIT_TIMEOUT){
            _taskDelay(1);
            iWait++;
          }

          // Write BMP Line out!
          tzsend = (client->connected() ? client->write((const char*) uidata, tzdatapos) : -1);
          if (iWait > 10)
            _DBGPRINTF(PSTR("DeliverBmpGrid: DEBUG SENDING (Wait %dms) #1 %d/%d bytes (space %d bytes).\n"), iWait, tzdatapos, tzsend, client->space() );
          
          // Reset values
          tzdatapos = 0;
          uitmp = uidata;
        }

        for(int h=0; h < processingWidth; h++) {
          nPixelHit = -1;
          if (TestBit(global::m_pCamProcessor->GetCamPoints(), BitIndexEx(LEDSINGLECOLOR, v, h, lev1size, lev2size))) {
            for(int n=iStart; n < iMaxStep; n+=(global::m_pCamProcessor->GetCamMode() != LED_MODE_CAM_SINGLE && n == LEDSINGLECOLOR ? 1 : iStep /* Need to process index 0 and 1 in BAR/Detail Mode */) ) {
              // Mark pixels based on CamProcessor geometry.
              if (TestBit(global::m_pCamProcessor->GetCamPoints(), BitIndexEx(n, v, h, lev1size, lev2size))) {
                nPixelHit = n;
                break;
              }
            }
          }

          if (nPixelHit >= 0) {
            // Mark  PIXEL
            *uitmp++ = (nPixelHit % 2 == 0 && nPixelHit > 0 ? nPixelHit * (256 / iMaxStep) : 255); 
            *uitmp++ = (nPixelHit % 2 == 0 ? nPixelHit * (256 / iMaxStep) : 0);
            *uitmp++ = (nPixelHit % 2 == 1 ? nPixelHit * (256 / iMaxStep) : 0);
            iPixMarked++;
          } else {
            // Add Black PIXEL
            *uitmp++ = 0;
            *uitmp++ = 0;
            *uitmp++ = 0;
          }
          tzdatapos += 3;
        }
      }

      // Send remaining bytes
      if (tzdatapos > 0) {
        // Write BMP Line out!
        tzsend = (client->connected() ? client->write((const char*) uidata, tzdatapos) : -1);
_DBGPRINTF(PSTR("DeliverBmpGrid: DEBUG FINAL SENDING %d/%d bytes.\n"), tzdatapos, tzsend);
      }

      // Finally request
      request->send(response);

      // Close Connection for sure
      if (client->connected()) 
        client->close();
      
      // Done with BMP generating
      _DBGPRINTF(PSTR("DeliverBmpGrid: Successfully served image to client. Marked %d of %d pixels\n"), iPixMarked, iPixCount);
    } else {
      // FAIL buffer size
      global::errprintf_P(PSTR("DeliverBmpGrid: Fail, insufficient buffer %d, required is %d to served image is to client.\n"), DATA_MAX_BUFFER, (iPixCount * 3));
      request->send(204); // No Content available
    }
    // Delete helper memory
    delete[] uidata;
  } else {
    request->send(204); // No Content available
  }  
}
bool checkImageTaskRunning() {
  if (handleTaskStream != NULL) {
    int iStatus = eTaskGetState(handleTaskStream);
    if (iStatus == eSuspended) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
}

void handleImageTask(AsyncWebServerRequest *request) {
  // Create new TASKS if necessary 
  if (handleTaskStream == NULL) {
    // Create new Task      
    xTaskCreatePinnedToCore(&TaskStreamCode, 
                            "TaskStream", 
                            4096, 
                            request, 
                            DEFAULT_TASK_PRIORITY, /* default priority */
                            &handleTaskStream,
                            ARDUINO_RUNNING_CORE); /* pin task to core 1 */
    _DBGPRINTF(PSTR("handleImageTask: Created TASK for TaskImageCode with HANDLE %d\n"), handleTaskStream);
  } else {
    int iStatus = eTaskGetState(handleTaskStream);
    if (iStatus == eSuspended) {
      _DBGPRINTF(PSTR("handleImageTask: TASK for TaskImageCode with HANDLE %d is suspended, resuming ... \n"), handleTaskStream);
      vTaskResume(handleTaskStream); 
    } else {
      global::errprintf_P(PSTR("handleImageTask: TASK for TaskImageCode %d already BUSY!\n"), taskImageMode);
      request->send_P(503, IMAGE_PNG, (const uint8_t *)data_images_app_logo_png, DATA_IMAGES_APP_LOGO_PNG_LEN);
    }
  }    
}
void handleCamBmpGrid(AsyncWebServerRequest *request) {
  String strResult; 

  if (global::m_bCameraOK) {
    _DBGPRINTF(PSTR("handleCamBmpGrid: ENTER \n"));
    int iTimeStart = millis();

# if defined(HANDLE_IMAGE_DIRECT)
    if(checkImageTaskRunning() || taskImageMode != HANDLE_IMG_NONE) {
      global::errprintf_P(PSTR("handleCamBmpGrid: Error, taskImageMode %d still busy - abort.\n"), taskImageMode );
      request->send(501);
      return;
    }
# else
    if (taskImageMode == HANDLE_BMP_GRID) {
      // Already called, abort
      global::errprintf_P(PSTR("handleCamBmpGrid: Error, taskImageMode already in HANDLE_BMP_GRID.\n"));
      strResult = PSTR("ERROR: TEMPORARY NOT AVAILABLE"); 
      request->send(503, TEXT_PLAIN, strResult);
      return;
    } else if (handleTaskStream != NULL && 
        taskStreamclient != NULL) {
        _DBGPRINTF(PSTR("handleCamBmpGrid: Force closing streamclient for task HANDLE %d \n"), handleTaskStream);
      strmcfg.bAbort = true;
      _taskDelay(1000);
    } else if (taskStreamRequest != NULL ) {
      _DBGPRINTF(PSTR("handleCamBmpGrid: Wait for StreamRequest to close\n"));
      _taskDelay(1000); 
    }

    // Wait for other image tasks to finish!
    while(checkImageTaskRunning()) {
      if (millis() > (iTimeStart + IMG_WAIT_TIMEOUT)) {
        global::errprintf_P(PSTR("handleCamBmpGrid: Error, taskImageMode %d still busy after %d millis.\n"), taskImageMode, IMG_WAIT_TIMEOUT);
        strResult = PSTR("ERROR: TEMPORARY NOT AVAILABLE"); 
        request->send(503, TEXT_PLAIN, strResult);
        return;
      }
      _taskDelay(100); 
    }
# endif
    
# if defined(HANDLE_BMP_GRID_DIRECT)
    // Disable fucking Watchdog!
    disableCore0WDT();

    // Handle it it's own function
    DeliverBmpGrid(request);

    // Enable fucking Watchdog!
    enableCore0WDT();
# else
    // Set Task Image Mode here!
    taskImageMode = HANDLE_BMP_GRID;

    // Set reference for task client
    taskStreamRequest = request;

    // Start/Resume 
    handleImageTask(request);
# endif
  } else {
    _DBGPRINTF(PSTR("handleCamBmpGrid: Error, Camera module is not working.\n"));
    strResult = PSTR("ERROR: CAMERA MODULE NOT WORKING"); 
    request->send(404, TEXT_PLAIN, strResult);
  }
}
void handleCamBmp(AsyncWebServerRequest *request) {
  String strResult; 
  int iTimeStart = millis();

  if (global::m_bCameraOK) {
    _DBGPRINTF(PSTR("handleCamBmp: ENTER \n"));
# if defined(HANDLE_IMAGE_DIRECT)
    if(checkImageTaskRunning()) {
      global::errprintf_P(PSTR("handleCamBmp: Error, taskImageMode %d still busy - abort.\n"), taskImageMode, IMG_WAIT_TIMEOUT);
      request->send(501);
      return;
    }
# else
    if (taskImageMode == HANDLE_BMP) {
      // Already called, abort
      global::errprintf_P(PSTR("handleCamBmp: Error, taskImageMode already in HANDLE_BMP.\n"));
      strResult = PSTR("ERROR: TEMPORARY NOT AVAILABLE"); 
      request->send(503, TEXT_PLAIN, strResult);
      return;
    } else if (handleTaskStream != NULL && 
        taskStreamclient != NULL) {
        _DBGPRINTF(PSTR("handleCamBmp: Force closing streamclient for task HANDLE %d \n"), handleTaskStream);
      strmcfg.bAbort = true;
      _taskDelay(1000); 
    } else if (taskStreamRequest != NULL ) {
      _DBGPRINTF(PSTR("handleCamJpg: Wait for StreamRequest to close\n"));
      _taskDelay(1000); 
    }

    // Wait for other image tasks to finish!
    while(checkImageTaskRunning()) {
      if (millis() > (iTimeStart + IMG_WAIT_TIMEOUT)) {
        global::errprintf_P(PSTR("handleCamBmp: Error, taskImageMode %d still busy after %d millis.\n"), taskImageMode, IMG_WAIT_TIMEOUT);
        strResult = PSTR("ERROR: TEMPORARY NOT AVAILABLE"); 
        request->send(503, TEXT_PLAIN, strResult);
        return;
      }
      _taskDelay(100); 
    }
# endif

    // Set Task Image Mode here!
    taskImageMode = HANDLE_BMP;

    // Set reference for task client
    taskStreamRequest = request;

    // Start/Resume 
    handleImageTask(request);
  } else {
    _DBGPRINTF(PSTR("handleCamBmp: Error, Camera module is not working.\n"));
    strResult = PSTR("ERROR: CAMERA MODULE NOT WORKING"); 
    request->send(404, TEXT_PLAIN, strResult);
  }
}
void handleCamJpg(AsyncWebServerRequest *request){
  String strResult; 

  if (global::m_bCameraOK) {
    _DBGPRINTF(PSTR("handleCamJpg: ENTER \n"));

# if defined(HANDLE_IMAGE_DIRECT)
    if(taskImageMode == HANDLE_JPEG) {
      global::errprintf_P(PSTR("handleCamJpg: Error, taskImageMode %d still busy - abort.\n"), taskImageMode);
      request->send(501);
      return;
    }

    /* Rate limit: reject if last accepted request was too recent (avoids OV3660 stack overflow from rapid resolution switches) */
    {
      unsigned long now = millis();
      if ((now - s_lastCamJpgServedMs) < CAM_JPG_COOLDOWN_MS) {
        _DBGPRINTF(PSTR("handleCamJpg: Rate limited - reject (cooldown %lu ms)\n"), (unsigned long)(CAM_JPG_COOLDOWN_MS - (now - s_lastCamJpgServedMs)));
        request->send(503, TEXT_PLAIN, F("ERROR: TOO FREQUENT - wait a few seconds"));
        return;
      }
      s_lastCamJpgServedMs = now;
    }

    // Set Task Image Mode here!
    taskImageMode = HANDLE_JPEG;

#   if defined(RESIZE_IMAGE_PREVIEW)
      if (psramFound() && global::m_pLedHelper->GetLedMode() != LED_MODE_CAM) {
        /* Return higher resolution (320x240) only when NOT in CAM mode -
           SetCamResolution during active TaskCam capture causes OV3660 race/crash */
        global::SetCamResolution(4, false, false);
        _taskDelay( 200 );
      }
#   endif

    // Capture Single image
    camera_fb_t* fb = NULL;
    if ( global::m_pCamProcessor->camMutexLock()) {
      fb = esp_camera_fb_get();
      global::m_pCamProcessor->camMutexRelease();
    } else {
      global::errprintf_P(PSTR("handleCamJpg: Error, Camera capture failed #1 - MUTEX STILL LOCKED!\n"));
    }

#   if defined(RESIZE_IMAGE_PREVIEW)
      if (psramFound() && global::m_pLedHelper->GetLedMode() != LED_MODE_CAM) {
        /* Restore original Resolution - only if we changed it */
        global::SetCamResolution(global::m_pEepromHelper->GetSettingInt("CamIndex", 0), false);
      }
      if (fb == NULL) {
        global::errprintf_P(PSTR("handleCamJpg: Error, Camera capture failed - TRY AGAIN with LOWRES.\n"));

        // Capture second try
        if ( global::m_pCamProcessor->camMutexLock()) {
          fb = esp_camera_fb_get();
          global::m_pCamProcessor->camMutexRelease();
        } else {
          global::errprintf_P(PSTR("handleCamJpg: Error, Camera capture failed #2 - MUTEX STILL LOCKED!\n"));
        }
      }
#   endif

    // Process if frame was succesfull!
    if (fb == NULL) {
      global::errprintf_P(PSTR("handleCamJpg: Error, captureRaw failed.\n"));
      strResult = PSTR("ERROR: captureRaw FAILED"); 
      request->send(501);
      taskImageMode = (taskImageMode == HANDLE_JPEG ? HANDLE_IMG_NONE : taskImageMode);
      return;
    } else {
      // Use Helper Class for response!
      AsyncFrameResponse * response = new AsyncFrameResponse(fb, "image/jpeg");
      if (response == NULL) {
        global::errprintf_P(PSTR("handleCamJpg: Error, Response alloc failed!\n"));
        request->send(501);
        taskImageMode = (taskImageMode == HANDLE_JPEG ? HANDLE_IMG_NONE : taskImageMode);
        return;
      } else {
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
      }
      taskImageMode = (taskImageMode == HANDLE_JPEG ? HANDLE_IMG_NONE : taskImageMode);
      return;
    }
# else
    int iTimeStart = millis();
    
    if (taskImageMode == HANDLE_JPEG) {
      // Already called, abort
      global::errprintf_P(PSTR("handleCamJpg: Error, taskImageMode already in HANDLE_JPEG.\n"));
      strResult = PSTR("ERROR: TEMPORARY NOT AVAILABLE"); 
      request->send(503, TEXT_PLAIN, strResult);
      return;
    } else if (handleTaskStream != NULL && 
        taskStreamclient != NULL) {
        _DBGPRINTF(PSTR("handleCamJpg: Force closing streamclient for task HANDLE %d \n"), handleTaskStream);
      strmcfg.bAbort = true;
      _taskDelay(1000); 
    } else if (taskStreamRequest != NULL ) {
      _DBGPRINTF(PSTR("handleCamJpg: Wait for StreamRequest to close\n"));
      _taskDelay(1000); 
    }

    // Wait for other image tasks to finish!
    while(checkImageTaskRunning()) {
      if (millis() > (iTimeStart + IMG_WAIT_TIMEOUT)) {
        global::errprintf_P(PSTR("handleCamJpg: Error, taskImageMode %d still busy after %d millis.\n"), taskImageMode, IMG_WAIT_TIMEOUT);
        strResult = PSTR("ERROR: TEMPORARY NOT AVAILABLE"); 
        request->send(503, TEXT_PLAIN, strResult);
        return;
      }
      _taskDelay(100); 
    }

    // Set Task Image Mode here!
    taskImageMode = HANDLE_JPEG;

    // Set reference for task client
    taskStreamRequest = request;
    
    // Start/Resume 
    handleImageTask(request);
# endif
  } else {
    global::errprintf_P(PSTR("handleCamJpg: Error, Camera module is not working.\n"));
    strResult = PSTR("ERROR: CAMERA MODULE NOT WORKING"); 
    request->send(501, TEXT_PLAIN, strResult);
  }
}
void handleCamMjpeg(AsyncWebServerRequest *request){
  String strResult; 

  if (global::m_bCameraOK) {
    _DBGPRINTF(PSTR("handleCamMjpeg: STREAM BEGIN \n"));

# if defined(HANDLE_IMAGE_DIRECT)
    // Optional limit frames for benchmarking
    int iMax = -1;
    if (request->hasParam("max")) {
      String strMax = request->getParam("max")->value();
      iMax = strMax.toInt();
    }
  
    // Set Abort Flag for current Stream
    if (pCurrentStreamResponse != NULL) {
      AsyncJpegStreamResponse* currentStreamResponse = (AsyncJpegStreamResponse*) pCurrentStreamResponse;
      currentStreamResponse->_bAbort = true; 
    }

    // Create new Stream Response 
    AsyncJpegStreamResponse *response = new AsyncJpegStreamResponse((global::m_pLedHelper->GetLedMode() == LED_MODE_CAM ? 200 : 0), iMax, 1000);
    if(!response){
        global::errprintf_P(PSTR("handleCamMjpeg: Error, failed to start MJPEG STREAM.\n"));
        request->send(501);
        return;
    } else {
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
      
      // Store response pointer
      pCurrentStreamResponse = response;
    }
# else
    int iTimeStart = millis();

    if (taskImageMode == HANDLE_MJPEG) {
      // Already called, abort
      global::errprintf_P(PSTR("handleCamMjpeg: Error, taskImageMode already in HANDLE_MJPEG.\n"));
      strResult = PSTR("ERROR: TEMPORARY NOT AVAILABLE"); 
      request->send(503, TEXT_PLAIN, strResult);
      return;
    } else if (handleTaskStream != NULL && 
        taskStreamclient != NULL) {
        _DBGPRINTF(PSTR("handleCamMjpeg: Force closing streamclient for task HANDLE %d \n"), handleTaskStream);
      strmcfg.bAbort = true;
      _taskDelay(1000);
    } else if (taskStreamRequest != NULL ) {
      _DBGPRINTF(PSTR("handleCamMjpeg: Wait for StreamRequest to close\n"));
      _taskDelay(1000); 
    }

    // Wait for other image tasks to finish!
    while(checkImageTaskRunning()) {
      if (millis() > (iTimeStart + IMG_WAIT_TIMEOUT)) {
        global::errprintf_P(PSTR("handleCamMjpeg: Error, taskImageMode %d still busy after %d millis.\n"), taskImageMode, IMG_WAIT_TIMEOUT);
        strResult = PSTR("ERROR: TEMPORARY NOT AVAILABLE"); 
        request->send(503, TEXT_PLAIN, strResult);
        return;
      }
      _taskDelay(100);
    }

    // Set Task Image Mode here!
    taskImageMode = HANDLE_MJPEG;

    // Set reference for task client
    taskStreamRequest = request;
    taskStreamclient = request->client();

    // Start/Resume 
    handleImageTask(request);
# endif    
  } else {
    global::errprintf_P(PSTR("handleCamMjpeg: Error, Camera module is not working.\n"));
    request->send(501);
  }
}
void TaskStreamCode(void * pvParameters) {
  String strResult; 
  using namespace esp32cam;

  _DBGPRINTF(PSTR("TaskStreamCode: ENTER\n"));

  do {
    // Get current streaming request
    AsyncWebServerRequest* request = taskStreamRequest;
    AsyncClient* client = request->client();
    client->setRxTimeout(10000);
    
    if (taskImageMode == HANDLE_JPEG) {
# if defined(RESIZE_IMAGE_PREVIEW)
      if (psramFound() && global::m_pLedHelper->GetLedMode() != LED_MODE_CAM) {
        /* Return higher resolution (320x240) only when NOT in CAM mode -
           SetCamResolution during active TaskCam causes OV3660 race/crash */
        global::SetCamResolution(4, false, false);
        _taskDelay( 250 );
      }
# endif
      // Lock Mutex first
      if ( !global::m_pCamProcessor->camMutexLock()) {
        global::errprintf_P(PSTR("TaskStreamCode: Error, Camera capture failed HANDLE_JPEG #1 - MUTEX STILL LOCKED!\n"));
        request->send_P(503, IMAGE_PNG, (const uint8_t *)data_images_app_logo_png, DATA_IMAGES_APP_LOGO_PNG_LEN);
        taskImageMode = HANDLE_IMG_NONE;
        continue;
      }

      // Capture Single image
      auto frame = esp32cam::capture();
      
      // Release Mutex
      global::m_pCamProcessor->camMutexRelease();

# if defined(RESIZE_IMAGE_PREVIEW)
      if (psramFound() && global::m_pLedHelper->GetLedMode() != LED_MODE_CAM) {
        /* Restore original Resolution - only if we changed it */
        global::SetCamResolution(global::m_pEepromHelper->GetSettingInt("CamIndex", 0), false);
      }
# endif

# if defined(RESIZE_IMAGE_PREVIEW)
      if (frame == nullptr && psramFound()) {
        global::errprintf_P(PSTR("TaskStreamCode: Error, Camera capture failed - TRY AGAIN with LOWRES.\n"));

        // Capture second try
        if ( global::m_pCamProcessor->camMutexLock()) {
          frame = esp32cam::capture();
          global::m_pCamProcessor->camMutexRelease();
        } else {
          global::errprintf_P(PSTR("TaskStreamCode: Error, Camera capture failed HANDLE_JPEG #2 - MUTEX STILL LOCKED!\n"));
        }
      }
# endif

      if (frame == nullptr) {
        global::errprintf_P(PSTR("TaskStreamCode: Error, Camera capture failed.\n"));
        request->send_P(503, IMAGE_PNG, (const uint8_t *)data_images_app_logo_png, DATA_IMAGES_APP_LOGO_PNG_LEN);
        taskImageMode = HANDLE_IMG_NONE;
        continue;
      }
      _DBGPRINTF(PSTR("TaskStreamCode: CAPTURE OK %dx%d %db\n"), frame->getWidth(), frame->getHeight(), static_cast<int>(frame->size()));
  
      // Begin WebRespone for image
      AsyncWebServerResponse *response = request->beginResponse(200); 
  
      // Create response to client
      response->setContentLength(frame->size());
      response->setCode(200);
      response->setContentType("image/jpeg");
  
      if (frame->writeTo(*client)){
        _DBGPRINTF(PSTR("TaskStreamCode: Successfully served image to client.\n"));
      } else {
        _DBGPRINTF(PSTR("TaskStreamCode: Error while serving image to client (connected:%s).\n"), (client->connected() ? "TRUE" : "FALSE") );
      }

      // Finally request
      request->send(response);
      if (client->connected()) 
        client->close();
        
      // Set task to DONE
      taskImageMode = HANDLE_IMG_NONE;
    } else if (taskImageMode == HANDLE_BMP_GRID) {
      // Handle it it's own function
      DeliverBmpGrid(request);

      // Set task to DONE
      taskImageMode = HANDLE_IMG_NONE;
    } else if (taskImageMode == HANDLE_BMP) {
      // Lock Mutex first
      if ( !global::m_pCamProcessor->camMutexLock()) {
        global::errprintf_P(PSTR("TaskStreamCode: Error, Camera capture failed HANDLE_BMP - MUTEX STILL LOCKED!\n"));
        
        request->send_P(503, IMAGE_PNG, (const uint8_t *)data_images_app_logo_png, DATA_IMAGES_APP_LOGO_PNG_LEN);
        taskImageMode = HANDLE_IMG_NONE;
        continue;
      }

      // Capture Single image
      auto frame = esp32cam::capture();
      
      // Release Mutex
      global::m_pCamProcessor->camMutexRelease();

      if (frame == nullptr) {
        _DBGPRINTF(PSTR("TaskStreamCode: Error, Camera capture failed.\n"));
        request->send_P(503, IMAGE_PNG, (const uint8_t *)data_images_app_logo_png, DATA_IMAGES_APP_LOGO_PNG_LEN);
        taskImageMode = HANDLE_IMG_NONE;
        continue;
        // return;
      }
      _DBGPRINTF(PSTR("TaskStreamCode: CAPTURE OK %dx%d %db FORMAT=%d\n"), frame->getWidth(), frame->getHeight(), static_cast<int>(frame->size()), frame->getPixFormat() );
      int realWidth = frame->getWidth();
      int realHeight = frame->getHeight();

      // Frame data variables
      uint8_t* uiData = frame->data(); 
      size_t tzDataLen = frame->size(); 
  
      // Convert to BMP if JPEG - DO OUR OWN Conversion!
      if (frame->isJpeg() ) {
        // Helper variables
        int pix_count = realWidth * realHeight;
        size_t rgbOutLen = (pix_count * 3) + BMP_HEADER_LEN;
        int iBytePos = 0; 

        // Alloc memory for conversion on PSRAM!        
        uint8_t* pOutbuf = (uint8_t *)_malloc_psram(rgbOutLen);

        _DBGPRINTF(PSTR("TaskStreamCode: CONVERT FROM JPG FinalWidth=%d FinalHeight %d Inlen=%d Outlen=%d ESP.getFreeHeap=%d\n"), realWidth, realHeight, tzDataLen, rgbOutLen, ESP.getFreeHeap() );

        // Create BMP Header for RGB888 
        *(pOutbuf+0)  = 'B';
        *(pOutbuf+1) = 'M';
        bmp_header_t * bitmapHeader  = (bmp_header_t*) (pOutbuf+2);
        bitmapHeader->reserved = 0;
        bitmapHeader->filesize = rgbOutLen;
        bitmapHeader->fileoffset_to_pixelarray = BMP_HEADER_LEN;
        bitmapHeader->dibheadersize = 40;
        bitmapHeader->width = realWidth;
        bitmapHeader->height = -(realHeight);//set negative for top to bottom
        bitmapHeader->planes = 1;
        bitmapHeader->bitsperpixel = 24;
        bitmapHeader->compression = 0;
        bitmapHeader->imagesize = (pix_count * 3);
        bitmapHeader->ypixelpermeter = 0x0B13 ; //2835 , 72 DPI
        bitmapHeader->xpixelpermeter = 0x0B13 ; //2835 , 72 DPI
        bitmapHeader->numcolorspallette = 0;
        bitmapHeader->mostimpcolor = 0;
  
        // Set BytePos after header
        iBytePos += BMP_HEADER_LEN; 
  
        // Convert to RGB888
        if (CamProcessor::camJpg2rgb888(frame->data(), frame->size(), pOutbuf+iBytePos, JPG_SCALE_NONE)){
          _DBGPRINTF_ULTRA(PSTR("TaskStreamCode: SUCCESS fmt2rgb888 conversion (Len=%d, Pixels=%d).\n"), rgbOutLen, realWidth*realHeight);
          // Conversion successfull, use uiData Buf
          uiData = pOutbuf;
          tzDataLen = rgbOutLen;

          // Set Frame Buffer with our own!
          frame->setData(pOutbuf, rgbOutLen, true);
          
          _DBGPRINTF(PSTR("TaskStreamCode: CONVERT OK %dx%d %db\n"), frame->getWidth(), frame->getHeight(), static_cast<int>(frame->size()));
        } else {
          _DBGPRINTF(PSTR("TaskStreamCode: FAIL fmt2rgb888!\n"));
          // Free conversion memory (allocated with _malloc_psram); guard against NULL
          if (pOutbuf) free(pOutbuf);
          pOutbuf = NULL;

          request->send_P(503, IMAGE_PNG, (const uint8_t *)data_images_app_logo_png, DATA_IMAGES_APP_LOGO_PNG_LEN);
          taskImageMode = HANDLE_IMG_NONE;
          continue;
        }        
      } else {
        _DBGPRINTF(PSTR("TaskStreamCode: CONVERT NOT NEEDED\n"));
      }
  
      /* Mark pixels in BMP data if CamProcessor geometry is ready. */
      if (global::m_pCamProcessor != NULL &&
          global::m_pCamProcessor->GetCamPoints() != NULL &&
          global::m_pCamProcessor->GetRectAreas() != NULL) {
        int iJpgScale = 0;
        int iPixMulti = 1;
        int processingWidth = realWidth;   // Needed for BitIndex MACRO
        int processingHeight = realHeight; // Needed for BitIndex MACRO

        // Helper function to calc Jpeg Resolution Compression
        global::m_pCamProcessor->camSetJpegProperties(frame->getWidth(), &iJpgScale, &iPixMulti, &processingWidth, &processingHeight);

        // Bitindex helpers
        int lev1size = processingWidth*processingHeight;
        int lev2size = processingWidth;
        rectanglepoints* rectAreas = global::m_pCamProcessor->GetRectAreas();

        // Helper variables 
        int iPixIndex = 0;
        int iPixMarked = 0;
        int iMaxStep = 1;
        int iStep = 1;
        int iSectorStart = 0;
        if (global::m_pCamProcessor->GetCamMode() == LED_MODE_CAM_BAR) {
          iMaxStep = CAM_SECTOR_COUNT(global::m_pCamProcessor->GetCamDetailLevel());
          iStep = global::m_pCamProcessor->GetCamDetailLevel();
          iSectorStart = 1;
        } else if (global::m_pCamProcessor->GetCamMode() == LED_MODE_CAM_DETAIL) {
          iMaxStep = CAM_SECTOR_COUNT(global::m_pCamProcessor->GetCamDetailLevel());
          iSectorStart = 1;
        }

        // Loop height/width/pixel and mark active PIXELS
        for(int v=0; v < processingHeight; v++) {
          for(int h=0; h < processingWidth; h++) {
            if (TestBit(global::m_pCamProcessor->GetCamPoints(), BitIndexEx(LEDSINGLECOLOR, v, h, lev1size, lev2size))) {
            
              for(int n=iSectorStart; n < iMaxStep; n+=iStep) {
                // Mark pixels based on CamProcessor geometry.
                if (TestBit(global::m_pCamProcessor->GetCamPoints(), BitIndexEx(n, v, h, lev1size, lev2size))) {
                  iPixIndex = BMP_HEADER_LEN + (3 * (
                                ((v * iPixMulti) * realWidth) + 
                                 (h * iPixMulti) ) 
                              );
                  for(int p=0; p < iPixMulti; p++) {
                    // Mark  PIXEL
                    *(uiData+iPixIndex+(p*3)) =   (n % 2 == 0 && n > 0 ? n * (256 / iMaxStep) : 255); // (n % 2 == 0 ? 255 : 0); 
                    *(uiData+iPixIndex+(p*3)+1) = (n % 2 == 0 ? n * (256 / iMaxStep) : 0);
                    *(uiData+iPixIndex+(p*3)+2) = (n % 2 == 1 ? n * (256 / iMaxStep) : 0);
                  }
                  iPixMarked++;
                }
              }

            }
          }
        }

        // Mark pixels based on CamProcessor rect areas.
        for(int n=0; n < iMaxStep; n+=iStep) {
          iPixIndex = BMP_HEADER_LEN + (( ((rectAreas[n].topleft.y * iPixMulti) * realWidth) + (rectAreas[n].topleft.x * iPixMulti) ) * 3); 
          for(int p=0; p < iPixMulti; p++) {
            // Mark PIXEL
            *(uiData+iPixIndex+(p*3)) =  255;
            *(uiData+iPixIndex+(p*3)+1) = 0;
            *(uiData+iPixIndex+(p*3)+2) = 255;
          }
          iPixIndex = BMP_HEADER_LEN + (( ((rectAreas[n].topright.y * iPixMulti) * realWidth) + (rectAreas[n].topright.x * iPixMulti) ) * 3); 
          for(int p=0; p < iPixMulti; p++) {
            // Mark PIXEL
            *(uiData+iPixIndex+(p*3)) =  255;
            *(uiData+iPixIndex+(p*3)+1) = 255;
            *(uiData+iPixIndex+(p*3)+2) = 0;
          }
          iPixIndex = BMP_HEADER_LEN + (( ((rectAreas[n].bottomleft.y * iPixMulti) * realWidth) + (rectAreas[n].bottomleft.x * iPixMulti) ) * 3); 
          for(int p=0; p < iPixMulti; p++) {
            // Mark PIXEL
            *(uiData+iPixIndex+(p*3)) =  0;
            *(uiData+iPixIndex+(p*3)+1) = 255;
            *(uiData+iPixIndex+(p*3)+2) = 255;
          }
          iPixIndex = BMP_HEADER_LEN + (( ((rectAreas[n].bottomright.y * iPixMulti) * realWidth) + (rectAreas[n].bottomright.x * iPixMulti) ) * 3); 
          for(int p=0; p < iPixMulti; p++) {
            // Mark PIXEL
            *(uiData+iPixIndex+(p*3)) =  255;
            *(uiData+iPixIndex+(p*3)+1) = 255;
            *(uiData+iPixIndex+(p*3)+2) = 255;
          }
          iPixMarked+=4;
        }
         
        _DBGPRINTF(PSTR("TaskStreamCode: DEBUG MARK CAPTURE POINTS - BYTES %d, Height %d, Width %d, Multi %d, Marked %d \n"), 
          tzDataLen, realHeight, realWidth, iPixMulti, iPixMarked);
      }

      // Begin WebRespone for image
      AsyncWebServerResponse *response = request->beginResponse(200); 
  
      // Create response to client
      response->setContentLength(tzDataLen);
      response->setCode(200);
      response->setContentType("image/bmp");
  
      AsyncClient* client = request->client();

      // Just write Frame to Client!
      if (frame->writeTo(*client)){
        _DBGPRINTF(PSTR("TaskStreamCode: Successfully served image to client.\n"));
      } else {
        _DBGPRINTF(PSTR("TaskStreamCode: Error while serving image to client (connected:%s).\n"), (client->connected() ? "TRUE" : "FALSE") );
      }

      // Finally request
      request->send(response);
      if (client->connected()) 
        client->close();
      
      // Set task to DONE
      taskImageMode = HANDLE_IMG_NONE;
    } else if (taskImageMode == HANDLE_MJPEG) {
      // Start Client Streaming
      long startTime = millis();

      if (global::m_pLedHelper->GetLedMode() == LED_MODE_CAM) {
        strmcfg.minInterval = 200; /* 8 FPS is sufficient */
      } else {
        strmcfg.minInterval = 0;
      }

      // Optional limit frames for benchmarking
      if (request->hasParam("max")) {
        String strMax = request->getParam("max")->value();
        int iMax = strMax.toInt();
        if (iMax > 0) {
          strmcfg.maxFrames = iMax;
        } else {
          strmcfg.maxFrames = -1;
        }
      } else {
        // No end
        strmcfg.maxFrames = -1;
      }
      strmcfg.frameTimeout = 1000;
      strmcfg.bAbort = false;

      /* Moved streaming code into webserver code for now! */
      taskStreamclient->write(PSTR("HTTP/1.1 200 OK\r\n"
                                   "Content-Type: multipart/x-mixed-replace;boundary=" BOUNDARY "\r\n"
                                   "\r\n"));
      long lastCapture = millis();
      long timeSinceLastCapture = millis();
      char szTmp[256];
      int nFrames;
      int nNullFrames = 0;
      for (nFrames = 0; strmcfg.maxFrames < 0 || nFrames < strmcfg.maxFrames; ++nFrames) {
        lastCapture = millis();

        // Lock Mutex first
        if ( !global::m_pCamProcessor->camMutexLock()) {
          global::errprintf_P(PSTR("TaskStreamCode: Error, Camera capture failed HANDLE_BMP - MUTEX STILL LOCKED!\n"));
          _taskDelay(100);
          continue; 
        }
  
        // Capture Single image
        auto frame = esp32cam::capture();
        
        // Release Mutex
        global::m_pCamProcessor->camMutexRelease();

        // Obtain Frame!
        if (frame == nullptr) {
          nNullFrames++;
          if (nNullFrames > 10)
            break;
          else {
            _taskDelay( static_cast<int>(strmcfg.minInterval) );
            continue;
          }
        } else {
          nNullFrames = 0;
        }
        // Print out Frame to network stream!
        size_t stLen = snprintf_P(szTmp, 256, PSTR( "Content-Type: image/jpeg\r\n"
                                                    "Content-Length: %d\r\n"
                                                    "\r\n"), static_cast<int>(frame->size()) );
        
        // Check if Client is still connected!
        if (!taskStreamclient->disconnected()) {
          taskStreamclient->write((const char*) szTmp, stLen);
          if (!frame->writeTo(*taskStreamclient, strmcfg.frameTimeout)) {
            break;
          }
        } else {
          break;
        }
        taskStreamclient->write("\r\n--" BOUNDARY "\r\n");

        // Check for sleeptime
        timeSinceLastCapture = millis() - lastCapture;
        if (strmcfg.minInterval > 0 && static_cast<int>(timeSinceLastCapture) < strmcfg.minInterval) {
// _DBGPRINTF(PSTR("TaskStreamCode: SLEEP %d \n"),  static_cast<int>(strmcfg.minInterval - timeSinceLastCapture) );
          _taskDelay( static_cast<int>(strmcfg.minInterval - timeSinceLastCapture) );
        } else {
          taskYIELD();
        }

        // Abort Streaming if canceled from outside!
        if (strmcfg.bAbort)
          break;
      }
/*
      int res = esp32cam::Camera.streamMjpeg(*taskStreamclient, &strmcfg);
      if (res <= 0) {
        _DBGPRINTF(PSTR("TaskStreamCode: Error, Stream capture failed with res=%d.\n"), res);
      } else {
        _DBGPRINTF(PSTR("TaskStreamCode: Stream capture finished\n"));
      }
*/
      // Client Streaming ended
      auto duration = millis() - startTime;
      _DBGPRINTF(PSTR("TaskStreamCode: STREAM END , %dfrm %0.2ffps\n"), nFrames, 1000.0 * nFrames / duration);

      // Close Stream for sure
      if (taskStreamclient->connected())
        taskStreamclient->close(); 

      // Set task to DONE
      taskImageMode = HANDLE_IMG_NONE;
    } else if (taskImageMode == HANDLE_IMG_NONE) {
      _DBGPRINTF(PSTR("TaskBmpCode: HANDLE_IMG_NONE\n"));
    } else {
      // UNKNOWN MODE
      global::errprintf_P(PSTR("handleCamJpg: Error, unknown IMAGE Mode\n"));
      request->send_P(404, IMAGE_PNG, (const uint8_t *)data_images_app_logo_png, DATA_IMAGES_APP_LOGO_PNG_LEN);
    }

    // Clear helpers 
    taskStreamclient = NULL; 
    taskStreamRequest = NULL;
  
    // Suspend ourselves.
    _DBGPRINTF(PSTR("TaskStreamCode: Suspend TASK for ourself %d \n"), handleTaskStream);
    vTaskSuspend(handleTaskStream);
  } while(true);

  _DBGPRINTF(PSTR("TaskStreamCode: EXIT\n"));  
}
// --- 
#endif