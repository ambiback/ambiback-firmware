#ifndef ambiwebserver_handlers_h
#define ambiwebserver_handlers_h

/* Shared declarations for AmbiWebServer route handlers.
 * Used by ambiwebserver.cpp (InitServer) and by ambiwebserver_*.cpp implementations.
 */
#include <ESPAsyncWebServer.h>

/* Content-type and buffer macros (shared by all webserver TUs) */
#define TEXT_JAVASCRIPT PSTR("text/javascript")
#define TEXT_CSS PSTR("text/css")
#define TEXT_PLAIN PSTR("text/plain")
#define TEXT_HTML PSTR("text/html")
#define APP_JSON PSTR("application/json")
#define IMAGE_PNG PSTR("image/png")
#define IMAGE_GIF PSTR("image/gif")

#define MESSAGE_MAX_STRING 1024
#define DATA_MAX_BUFFER 2048
#define IMG_WAIT_TIMEOUT 1500

/* Template processor for index/include pages (defined in ambiwebserver_pages.cpp) */
String indexProcessor(const String& var);

/* Device claim checks (defined in ambiwebserver_api.cpp) */
bool isDeviceClaimedForWeb();
bool isAdminRequest(AsyncWebServerRequest *request);
bool requireAdmin(AsyncWebServerRequest *request);

/* --- Common handlers --- */
void notFound(AsyncWebServerRequest *request);
void handleEmpty(AsyncWebServerRequest *request);
void handleIndex(AsyncWebServerRequest *request);
void handleReboot(AsyncWebServerRequest *request);
void handleWifiWps(AsyncWebServerRequest *request);
void handleControllersettings(AsyncWebServerRequest *request);
void handleSetColor(AsyncWebServerRequest *request);
void handleSetAction(AsyncWebServerRequest *request);
void handleSetColorMode(AsyncWebServerRequest *request);
void handleSetParameter(AsyncWebServerRequest *request);
void handleSetParameterPresets(AsyncWebServerRequest *request);
void handleGetParameter(AsyncWebServerRequest *request);
void handleGetParametersAsJson(AsyncWebServerRequest *request);
void handleGetStatusAsJson(AsyncWebServerRequest *request);
#if defined(ENABLE_WIFI_AP)
void handleGetProvisioningAsJson(AsyncWebServerRequest *request);
#endif
void handleDiscover(AsyncWebServerRequest *request);
void handleGetScanNetworksAsJson(AsyncWebServerRequest *request);
void handleGetLastStatusMessageAsJson(AsyncWebServerRequest *request);
void handleUpdate(AsyncWebServerRequest *request);
void handleUpdateUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void handleAutoUpdate(AsyncWebServerRequest *request);
void handleGetFirmwareIndex(AsyncWebServerRequest *request);
void handleUpdateStatus(AsyncWebServerRequest *request);
void handleAmbiBackWebManifest(AsyncWebServerRequest *request);
#if defined(ENABLE_WIFI_AP)
void handleProvisioningStart(AsyncWebServerRequest *request);
void handleProvisioningSubmit(AsyncWebServerRequest *request);
void handleProvisioningFinish(AsyncWebServerRequest *request);
void handleProvisioningClaim(AsyncWebServerRequest *request);
#endif

#if defined(ENABLE_CAM)
void handleImageTask(AsyncWebServerRequest *request);
void handleCamBmp(AsyncWebServerRequest *request);
void handleCamBmpGrid(AsyncWebServerRequest *request);
void handleCamJpg(AsyncWebServerRequest *request);
void handleCamMjpeg(AsyncWebServerRequest *request);
void handleSetCameraPoint(AsyncWebServerRequest *request);
void handleControllersettingsCamera(AsyncWebServerRequest *request);
void handleIncludeCamera(AsyncWebServerRequest *request);
void handleCamClck(AsyncWebServerRequest *request);
#endif

#if defined(ENABLE_HUE)
void handleControllerSettingsHue(AsyncWebServerRequest *request);
void handleRefreshHueDevices(AsyncWebServerRequest *request);
void handleIncludeHue(AsyncWebServerRequest *request);
#endif

#if defined(ENABLE_DREAMSCREEN)
void handleControllerSettingsDreamScreen(AsyncWebServerRequest *request);
void handleRefreshDreamScreenDevices(AsyncWebServerRequest *request);
void handleIncludeDreamScreen(AsyncWebServerRequest *request);
void handleAddDreamScreenToNetwork(AsyncWebServerRequest *request);
#endif

#endif /* ambiwebserver_handlers_h */
