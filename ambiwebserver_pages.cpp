/*
 * AmbiWebServer: Pages, main UI, OTA, Hue and DreamScreen handlers.
 * Route registration remains in ambiwebserver.cpp (InitServer).
 */
#include "globalincludes.h"
#include "ambiwebserver.h"
#include "ambiwebserver_handlers.h"
#include "ambiwebserver_html.h"

#include <Update.h>
#include <HTTPClient.h>
#include "esp_task_wdt.h"
#include "esp_wifi.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#if __has_include("esp_idf_version.h")
# include "esp_idf_version.h"
#endif

#if defined(ENABLE_CAM)
# include "camprocessor.h"
# include "sectorgeometry.h"
# define CAMPOINTSIZE 20
#endif

extern TaskHandle_t handleTaskUpdate;
void TaskUpdateCode(void * pvParameters);

// --- Template Helper (used by handleIndex and handleInclude*) ---
String indexProcessor(const String& var) {
  if(var == "V")
    return F(AMBIBACK_VERSION);
  else if(var == "DT")
    return F(DEVICE_NAME);
  else if(var == "AN")
    return F(AMBIBACK_NAME);
  else if(var == "FW")
    return String(F("Arduino " AMBIBACK_CORE_VERSION " / IDF ")) + esp_get_idf_version();
  else if(var == "BID")
    return F(AMBIBACK_BUILD_ID);
  else if(var == "DID")
    return String(global::GetDeviceID());
  else if(var == "DMAC")
    return String(global::GetDeviceIDMac());
  else if(var == "DN"){
    return String(global::GetDeviceName());
  } else if(var == "SS"){
    return String(global::GetDebugSyslogServer());
  } else if(var == "SP"){
    return String(global::GetDebugSyslogPort());
  } else if(var == "IP") {
    return WiFi.localIP().toString();
  } else if(var == "CS") {
#if defined(ENABLE_CAM)
    return F("Yes");
#else
    return F("No");
#endif
  } else if(var == "SST"){
    return (global::m_pLedHelper->GetSetupSideTop() == 1 ? F("checked") : String() );
  } else if(var == "SSL"){
    return (global::m_pLedHelper->GetSetupSideLeft() == 1 ? F("checked") : String() );
  } else if(var == "SSR"){
    return (global::m_pLedHelper->GetSetupSideRight() == 1 ? F("checked") : String() );
  } else if(var == "SSB"){
    return (global::m_pLedHelper->GetSetupSideBottom() == 1 ? F("checked") : String() );
  } else if(var == "DFL"){
    return String(DEFAULT_FRAME_LENGTH);
  } else if(var == "DFS"){
    return String(DEFAULT_FRAME_SMOOTHING);
  } else if(var == "DFM"){
    return String(DEFAULT_FRAME_MAX);
  } else if(var == "DSE"){
    return (global::GetSerialEnabled() ? F("checked") : String() );
  } else if(var == "DSYE"){
    return (global::GetSyslogEnabled() ? F("checked") : String() );
  } else if(var == "DUE"){
    return (global::GetUltraDebugEnabled() ? F("checked") : String() );
#if defined(ENABLE_CAM)
  } else if(var == "camera_awbgain"){
    return (global::GetCamAwbGainControl() == 1 ? F("checked") : String() );
  } else if(var == "camera_hmirror"){
    return (global::GetCamHMirror() == 1 ? F("checked") : String() );
  } else if(var == "camera_vflip"){
    return (global::GetCamVFlip() == 1 ? F("checked") : String() );
  } else if(var == "camera_DetailSmooth"){
    return (global::m_pCamProcessor->GetCamDetailSmooth() == 1 ? F("checked") : String() );
  } else if(var == "camera_CamSmoothEnable"){
    return (global::m_pCamProcessor->GetCamSmoothEnable() == 1 ? F("checked") : String() );
  } else if(var == "camera_HighQuality"){
    return (global::GetCamHighQuality() == 1 ? F("checked") : String() );
  } else if(var == "camera_bpc"){
    return (global::GetCamBPC() == 1 ? F("checked") : String() );
  } else if(var == "camera_wpc"){
    return (global::GetCamWPC() == 1 ? F("checked") : String() );
  } else if(var == "camera_denoise"){
    return (global::GetCamDenoise() == 1 ? F("checked") : String() );
  } else if(var == "camera_lenc"){
    return (global::GetCamLenc() == 1 ? F("checked") : String() );
  } else if(var == "camera_rawgma"){
    return (global::GetCamRawGMA() == 1 ? F("checked") : String() );
  } else if(var == "camera_aec"){
    return (global::GetCamAutoExposurecontrol() == 1 ? F("checked") : String() );
  } else if(var == "camera_aec2"){
    return (global::GetCamAec2() == 1 ? F("checked") : String() );
  } else if(var == "camera_adptexp"){
    return (global::GetCamAdaptiveExposurecontrol() == 1 ? F("checked") : String() );
  } else if(var == "camera_dcw"){
    return (global::GetCamDcw() == 1 ? F("checked") : String() );
  } else if(var == "camera_whitebalance"){
    return (global::GetCamWhitebalance() == 1 ? F("checked") : String() );
  } else if(var == "camera_gaincontrol"){
    return (global::GetCamGaincontrol() == 1 ? F("checked") : String() );
  } else if(var == "camera_autoonoff"){
    return (global::GetCamAutoOnOff() == 1 ? F("checked") : String() );
  } else if(var == "camera_adptgain"){
    return (global::GetCamAdaptiveGaincontrol() == 1 ? F("checked") : String() );
# if CAM_SENSOR == CAM_SENSOR_OV3660
  } else if(var == "camera_overclock"){
    return (global::GetCamOverclock() == 1 ? F("checked") : String() );
# endif
#endif
#if defined(ENABLE_HUE)
  } else if(var == "hue_enabled"){
    return (global::GetHueSupport() == 1 ? F("checked") : String() );
  } else if(var == "HIP"){
    return String(global::GetHueBridgeIP());
  } else if(var == "HUSR"){
    return (isDeviceClaimedForWeb() ? String() : String(global::GetHueBridgeUser()));
#else
  } else if(var == "HIP"){
    return String();
  } else if(var == "HUSR"){
    return String();
#endif
#if defined(ENABLE_DREAMSCREEN)
  } else if(var == "dreamscreen_support"){
    return (global::GetDreamScreenSupport() == 1 ? F("checked") : String() );
  } else if(var == "dreamscreen_emu"){
    return (global::GetDreamScreenEmulation() == 1 ? F("checked") : String() );
#endif
#if defined(ENABLE_RELAY)
  } else if(var == "system_relayto_direct"){
    return (global::GetAmbibackRelayModeDirect() == 1 ? F("checked") : String() );
  } else if(var == "system_relay_wifidirect"){
    return (global::GetAmbibackWifiDirectMode() == 1 ? F("checked") : String() );
#endif
  } else if(var == "system_restore_last_led_mode"){
    return (global::GetRestoreLastLedMode() ? F("checked") : String() );
#if defined(ENABLE_WIFI_AP)
  } else if(var == "system_wifi_ap_enable"){
    return (global::GetWifiAPEnable() ? F("checked") : String() );
  } else if(var == "system_provisioning_enable"){
    return (global::GetWifiProvisioningEnabled() ? F("checked") : String() );
  } else if(var == "system_wifi_direct_ssid"){
    return String(global::GetDeviceAPssid());
  } else if(var == "system_wifi_direct_pass"){
    return (isDeviceClaimedForWeb() ? String() : String(global::GetDeviceAPpass()));
  } else if(var == "system_provisioning_timeout"){
    return String(global::GetWifiProvisioningTimeoutMinutes());
#endif
  } else if(var == "MILLIS")
    return String(millis());
  return String("ERROR-" + var + "$");
}

// --- OTA & generic ---
void handleUpdate(AsyncWebServerRequest *request) {
  if (!requireAdmin(request)) {
    return;
  }
  _DBGPRINTF(PSTR("WebServer:handleUpdate: %s\n"), !Update.hasError()?"OK":"FAIL");
  AsyncWebServerResponse *response = request->beginResponse(200, TEXT_HTML, !Update.hasError() ?
    PSTR( "<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
          "<script>function countdown() { var i = document.getElementById('counter'); if (parseInt(i.innerHTML)<=0) { location.href = '/'; } else { i.innerHTML = parseInt(i.innerHTML)-1; } }setInterval(function(){ countdown(); },1000);</script></head>"
          "<body style=\"padding:5px;\"><h2 style='color:green'>Firmware update: SUCCESS</h2> </br><center><b><a href='/'>return to controller in <span id='counter'>5</span> second(s).</a></b></center></body></html>") :
    PSTR( "<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
          "<script>function countdown() { var i = document.getElementById('counter'); if (parseInt(i.innerHTML)<=0) { location.href = '/'; } else { i.innerHTML = parseInt(i.innerHTML)-1; } }setInterval(function(){ countdown(); },1000);</script></head>"
          "<body style=\"padding:5px;\"><h2 style='color:red'>Firmware update: FAILED</h2></br><center><b><a href='/'>return to controller in <span id='counter'>5</span> second(s).</a></b></center></body></html>"));
  response->addHeader("Connection", "close");
  request->send(response);
  global::SetFirmwareRunning(false);
  global::SetShouldReboot(!Update.hasError());
}

void handleUpdateUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!isAdminRequest(request)) {
    return; // handleUpdate answers with 403
  }
#if defined(ESP32)
  if (xPortGetCoreID() == CONFIG_ASYNC_TCP_RUNNING_CORE) {
    if(index == 0) {
      _DBGPRINTF(PSTR("WebServer:handleUpdateUpload: Disable WDT on CORE0\n"));
      global::SetFirmwareRunning( true );
    }
    disableCore0WDT();
  }
#endif

  if(index == 0) {
    _DBGPRINTF(PSTR("WebServer:handleUpdateUpload: Start Update: %s\n"), filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    } else {
      _DBGPRINTF(PSTR("WebServer:handleUpdateUpload: Update begin succeeded\n"));
    }
  }

  if(!Update.hasError()){
    if(Update.write(data, len) != len){
      _DBGPRINTF(PSTR("WebServer:handleUpdateUpload: Update error #2: \n"));
      Update.printError(Serial);
    } else {
      _DBGPRINTF_ULTRA(PSTR("WebServer:handleUpdateUpload: index=%d written %dB IsFinal=%s succeeded\n"), index, len, (final ? "TRUE" : "FALSE") );
    }
  }
  if(final){
    if(Update.end(true)){
      _DBGPRINTF(PSTR("WebServer:handleUpdateUpload: Update Success: %uB\n"), index+len);
    } else {
      _DBGPRINTF(PSTR("WebServer:handleUpdateUpload: Update error #3: \n"));
      Update.printError(Serial);
    }
  }

  if (xPortGetCoreID() == CONFIG_ASYNC_TCP_RUNNING_CORE) {
    if(final) { _DBGPRINTF(PSTR("WebServer:handleUpdateUpload: Disable WDT on CORE0\n")); }
    enableCore0WDT();
  }
}

void notFound(AsyncWebServerRequest *request) {
  _DBGPRINTF(PSTR("WebServer: notFound: URL unknown: %s\n"), request->url().c_str());
  if (request->method() == HTTP_OPTIONS) {
    AsyncWebServerResponse *response = request->beginResponse(204);
    response->addHeader(PSTR("Access-Control-Allow-Origin"), PSTR("*"));
    response->addHeader(PSTR("Access-Control-Allow-Headers"), PSTR("Content-Type, Accept, X-Requested-With"));
    response->addHeader(PSTR("Access-Control-Allow-Methods"), PSTR("GET, POST, OPTIONS"));
    request->send(response);
    return;
  }
  AsyncWebServerResponse *r = request->beginResponse(404, TEXT_PLAIN, PSTR("File Not found"));
  r->addHeader(PSTR("Access-Control-Allow-Origin"), PSTR("*"));
  request->send(r);
}

void handleEmpty(AsyncWebServerRequest *request) {
  _DBGPRINTF(PSTR("WebServer: handleEmpty: URL returns empty: %s\n"), request->url().c_str());
  request->send(200, TEXT_PLAIN, PSTR(""));
}

void handleReboot(AsyncWebServerRequest *request) {
  if (!requireAdmin(request)) {
    return;
  }
  String strResult;
  bool bReboot = false;
  _DBGPRINTF(PSTR("\nWebServer: handleReboot: Check for user wanted reboot\n"));

  if (request->hasParam("forreal")) {
    String strReal = request->getParam("forreal")->value();
    int iNewReal = strReal.toInt();
    if (iNewReal == 1) {
      bReboot = true;
      strResult = PSTR("REBOOTING");
    } else {
      strResult = PSTR("ERROR, invalid value forreal");
    }
  } else {
    strResult = PSTR("ERROR, forreal value missing");
  }

  _DBGPRINTF(PSTR("WebServer: handleReboot: %s\n"), strResult.c_str());
  request->send(200, TEXT_PLAIN, request->url() + ": " + strResult);

  if (bReboot) {
    global::SetShouldReboot(true);
  }
}

void handleWifiWps(AsyncWebServerRequest *request) {
  if (!requireAdmin(request)) {
    return;
  }
  String strResult;
  bool bWpsMode = false;
  _DBGPRINTF(PSTR("\nWebServer: handleWifiWps: Check for user wanted WIFI WPS Mode\n"));

  if (request->hasParam("forreal")) {
    String strReal = request->getParam("forreal")->value();
    int iNewReal = strReal.toInt();
    if (iNewReal == 1) {
      _DBGPRINTF(PSTR("nWebServer: handleWifiWps Trigger WPS MODE \n"));
      bWpsMode = true;
      strResult = PSTR("STARTING WPS MODE");
    } else {
      strResult = PSTR("ERROR, invalid confirmation value");
    }
  } else {
    strResult = PSTR("ERROR, confirmation value missing");
  }

  request->send(200, APP_JSON, PSTR("{\n\t\"status\": 1,\n\t\"result\": \"") + strResult + PSTR("\"\n}\n") );

  if (bWpsMode) {
    global::SetShouldStartWps(true);
  }
}

void handleAutoUpdate(AsyncWebServerRequest *request) {
  if (!requireAdmin(request)) {
    return;
  }
  String strResult;
  bool bUpdate = false;
  _DBGPRINTF(PSTR("handleAutoUpdate: START\n"));

  if (request->hasParam("version")) {
    global::m_pWebServer->SetUpdateVersion(request->getParam("version")->value().c_str());
    global::SetShouldStartFirmwareUpdate(true);
    strResult = PSTR("UPDATING");
    _DBGPRINTF(PSTR("handleAutoUpdate: handleAutoUpdate: %s\n"), strResult.c_str());
    request->send(200, APP_JSON, PSTR("{\n\t\"status\": 1,\n\t\"error\": \"\" \n}\n"));
  } else {
    strResult = PSTR("ERROR, version value missing");
    _DBGPRINTF(PSTR("handleAutoUpdate: handleAutoUpdate: %s\n"), strResult.c_str());
    request->send(200, APP_JSON, PSTR("{\n\t\"status\": 0,\n\t\"error\": \"") + strResult + PSTR("\" \n}\n"));
  }

#if defined(OTA_UPDATE_TASK)
  if (global::GetShouldStartFirmwareUpdate()) {
    if (!global::GetFirmwareRunning()) {
      int iFreeOTAKB = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL) / 1024;
      if (iFreeOTAKB < SYS_FREEMEM_OTA_MIN) {
        global::errprintf_P(PSTR("handleAutoUpdate: triggered OTA update with (%dKB) LOW MEMORY - will be done in LOOP!\n"), iFreeOTAKB);
      } else {
        _DBGPRINTF(PSTR("handleAutoUpdate: triggered OTA update with (%dKB) memory\n"), iFreeOTAKB);
        if (handleTaskUpdate == NULL) {
          xTaskCreatePinnedToCore(&TaskUpdateCode,
                                  "TaskUpdate",
                                  4096,
                                  NULL,
                                  DEFAULT_TASK_PRIORITY,
                                  &handleTaskUpdate,
                                  ARDUINO_RUNNING_CORE);
          _DBGPRINTF(PSTR("handleAutoUpdate: Created TASK for TaskUpdateCode HANDLE %d\n"), handleTaskUpdate);
        } else {
          int iStatus = eTaskGetState(handleTaskUpdate);
          if (iStatus == eSuspended) {
            _DBGPRINTF(PSTR("handleAutoUpdate: TASK for TaskUpdateCode with HANDLE %d is suspended, resuming ... \n"), handleTaskUpdate);
            vTaskResume(handleTaskUpdate);
          } else {
            _DBGPRINTF(PSTR("handleAutoUpdate: TASK for TaskUpdateCode already BUSY ... \n"));
          }
        }
      }
    } else {
      global::errprintf_P(PSTR("handleAutoUpdate: triggered OTA update but already running!\n"));
    }
  }
#endif
}

void handleGetFirmwareIndex(AsyncWebServerRequest *request) {
  _DBGPRINTF(PSTR("handleGetFirmwareIndex: fetch %s/index.json\n"), HTTP_BASE_URL);
  HTTPClient http;
  String url = String(HTTP_BASE_URL) + F("/index.json");
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    http.end();
    AsyncResponseStream *response = request->beginResponseStream(APP_JSON, payload.length());
    response->addHeader(PSTR("Access-Control-Allow-Origin"), PSTR("*"));
    response->print(payload);
    request->send(response);
  } else {
    http.end();
    char szErr[64];
    snprintf_P(szErr, sizeof(szErr), PSTR("{\"error\":\"HTTP %d\"}\n"), httpCode);
    AsyncWebServerResponse *r = request->beginResponse(502, APP_JSON, szErr);
    r->addHeader(PSTR("Access-Control-Allow-Origin"), PSTR("*"));
    request->send(r);
  }
}

void handleUpdateStatus(AsyncWebServerRequest *request) {
  _DBGPRINTF_ULTRA(PSTR("handleUpdateStatus: ENTER\n"));

  AsyncResponseStream *response = request->beginResponseStream(APP_JSON);
  response->printf_P(PSTR("{\n"));
  response->printf_P(PSTR("\t\"status\": %d,\n"),   global::GetFirmwareRunning() );
  response->printf_P(PSTR("\t\"reboot\": %d,\n"),   global::GetShouldReboot() );
  response->printf_P(PSTR("\t\"error\": \"%s\" \n"),(global::m_pWebServer->GetUpdateError() != NULL ? global::m_pWebServer->GetUpdateError() : ""));
  response->printf_P(PSTR("}\n"));
  request->send(response);
}

void handleAmbiBackWebManifest(AsyncWebServerRequest *request) {
  _DBGPRINTF_ULTRA(PSTR("handleAmbiBackWebManifest: ENTER\n"));

  AsyncResponseStream *response = request->beginResponseStream(APP_JSON);
  response->printf_P(PSTR("{\n"
                            "\"background_color\": \"#000\","
                            "\"Theme_color\": \"#000\","
                            "\"description\": \"%s Configuration.\","
                            "\"display\": \"standalone\","
                            "\"icons\": ["
                            "  {"
                            "    \"src\": \"images/app_logo.png\","
                            "    \"sizes\": \"192x192\","
                            "    \"type\": \"image/png\""
                            "  }"
                            "],"
                            "\"name\": \"%s Configuration\","
                            "\"short_name\": \"%s\","
                            "\"start_url\": \"index.html\""
                          "}"),
                          DEVICE_NAME,
                          DEVICE_NAME,
                          SHORTCUT_NAME
                          );
  request->send(response);
}

#if defined(ESP32)
void TaskUpdateCode(void * pvParameters) {
  _DBGPRINTF(PSTR("TaskUpdateCode: ENTER\n"));

  do {
    global::StartHttpUpdate();
    _DBGPRINTF(PSTR("TaskUpdateCode: Suspend TASK for ourself %d \n"), handleTaskUpdate);
    vTaskSuspend(handleTaskUpdate);
  } while(true);

  _DBGPRINTF(PSTR("TaskUpdateCode: EXIT\n"));
}
#endif

void handleIndex(AsyncWebServerRequest *request) {
  _DBGPRINTF(PSTR("WebServer: requested %s \n"), data_index_html_path);
  request->send(200, TEXT_HTML, (const uint8_t *)data_index_html, DATA_INDEX_HTML_LEN, indexProcessor);
}

#if defined(ENABLE_CAM)
void handleIncludeCamera(AsyncWebServerRequest *request) {
#if defined(EMBED_MINIMAL_UI)
  request->send(404, TEXT_PLAIN, PSTR("Not available in minimal UI"));
#else
  _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_include_camera_html_path);
  request->send_P(200, TEXT_HTML, (const uint8_t *)data_include_camera_html, DATA_INCLUDE_CAMERA_HTML_LEN, indexProcessor);
#endif
}
#endif

#if defined(ENABLE_HUE)
void handleIncludeHue(AsyncWebServerRequest *request) {
#if defined(EMBED_MINIMAL_UI)
  request->send(404, TEXT_PLAIN, PSTR("Not available in minimal UI"));
#else
  _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_include_hue_html_path);
  request->send_P(200, TEXT_HTML, (const uint8_t *)data_include_hue_html, DATA_INCLUDE_HUE_HTML_LEN, indexProcessor);
#endif
}
#endif

#if defined(ENABLE_DREAMSCREEN)
void handleIncludeDreamScreen(AsyncWebServerRequest *request) {
#if defined(EMBED_MINIMAL_UI)
  request->send(404, TEXT_PLAIN, PSTR("Not available in minimal UI"));
#else
  _DBGPRINTF(PSTR("WebServer: requested %s\n"), data_include_dreamscreen_html_path);
  request->send_P(200, TEXT_HTML, (const uint8_t *)data_include_dreamscreen_html, DATA_INCLUDE_DREAMSCREEN_HTML_LEN, indexProcessor);
#endif
}
#endif

void handleControllersettings(AsyncWebServerRequest *request) {
  _DBGPRINTF(PSTR("WebServer: requested %s \n"), request->url().c_str());
  AsyncResponseStream *response = request->beginResponseStream(TEXT_JAVASCRIPT);

  response->printf_P(PSTR("window.ab = [];\n"));
  response->printf_P(PSTR("window.ab.ubu = \"%s\";\n"), HTTP_BASE_URL);
  response->printf_P(PSTR("window.ab.m = %d;\n"), AMBIBACK_MAJOR);
  response->printf_P(PSTR("window.ab.n = %d;\n"), AMBIBACK_MINOR);
  response->printf_P(PSTR("window.ab.build = %d;\n"), AMBIBACK_BUILD);

  response->printf_P(PSTR("$(document).on(\"pagecreate\", function( event ) {\r\n"));

  response->printf_P(PSTR("$(\"#mode-video\").addClass(\"ui-disabled\");\r\n"));
  response->printf_P(PSTR("$(\"#mode-relay\").addClass(\"ui-disabled\");\r\n"));

#if defined(ENABLE_RELAY)
  response->printf_P(PSTR("window.ab.dev = [];\n"));
  response->printf_P(PSTR("window.ab.dev.name = [];\n"));
  response->printf_P(PSTR("window.ab.dev.id = [];\n"));
  response->printf_P(PSTR("window.ab.dev.mac = [];\n"));
  response->printf_P(PSTR("window.ab.dev.ip = [];\n"));

  int iDeviceCount = global::GetAmbibackDeviceCount();
  for(int i = 0; i < iDeviceCount; i++) {
    response->printf_P(PSTR("window.ab.dev.name[%d] = \"%s\";\n"), i, global::GetAmbibackDeviceName(i));
    response->printf_P(PSTR("window.ab.dev.id[%d] = \"%s\";\n"), i, global::GetAmbibackDeviceID(i));
    response->printf_P(PSTR("window.ab.dev.mac[%d] = \"%s\";\n"), i, global::GetAmbibackDeviceMac(i));
    response->printf_P(PSTR("window.ab.dev.ip[%d] = \"%s\";\n"), i, global::GetAmbibackDeviceIP(i));
  }

  response->printf_P(PSTR("window.ab.dev.cnt = %d;\n"), iDeviceCount );

  response->printf_P(PSTR("window.ab.selecteddevice = \"%s\";\n"), global::GetAmbibackSelectedDevice() );
  response->printf_P(PSTR("iAD();\r\n"));
  response->printf_P(PSTR("hRTC('%s');\r\n"), global::GetAmbibackSelectedDevice());

  response->printf_P(PSTR("$(\"#system_relay_mode\").val(\"%d\").selectmenu(\"refresh\");\r\n"), global::GetAmbibackRelayMode() );
#else
  response->printf_P(PSTR("$(\'#system_relay\').remove();\r\n"));
#endif

#if defined(ENABLE_WIFI_AP)
  response->printf_P(PSTR("$(\"#system_wifi_direct_ssid\").val(\"%s\");\r\n"), global::GetDeviceAPssid() );
  response->printf_P(PSTR("$(\"#system_wifi_direct_pass\").val(\"%s\");\r\n"), (isAdminRequest(request) ? global::GetDeviceAPpass() : "") );
  response->printf_P(PSTR("$(\"#system_provisioning_timeout\").val(\"%d\").selectmenu(\"refresh\");\r\n"), global::GetWifiProvisioningTimeoutMinutes() );
#else
  response->printf_P(PSTR("$(\'#system_wifi_ap\').remove();\r\n"));
#endif

#if !defined(ENABLE_DREAMSCREEN)
  response->printf_P(PSTR("$(\"#mode-relay\").addClass(\"ui-disabled\");\r\n"));
  response->printf_P(PSTR("$(\'#mode-relay\').closest(\"a\").remove();\r\n"));
#endif

#if !defined(ENABLE_VIDEO)
  response->printf_P(PSTR("rL(\'#tabs-setup\');\r\n"));
  response->printf_P(PSTR("rL(\'include_ambient.html\');\r\n"));

  response->printf_P(PSTR("$(\'#ui_color_saturation\').closest(\"div\").remove();\r\n"));
  response->printf_P(PSTR("$(\"#mode-ambient\").closest(\"a\").remove();\r\n"));
#endif

#if !defined(ENABLE_CAM)
  response->printf_P(PSTR("rL(\'include_camera.html\');\r\n"));
  response->printf_P(PSTR("$(\'#mode-cam\').closest(\"a\").remove();\r\n"));
  response->printf_P(PSTR("$(\'#system_cam\').remove();\r\n"));

  response->printf_P(PSTR("$(\"#system_buttonfunc option[value=2]\").remove();\r\n"));
  response->printf_P(PSTR("$(\"#system_buttonfunc option[value=3]\").remove();\r\n"));
  response->printf_P(PSTR("$(\"#system_buttonfunc option[value=4]\").remove();\r\n"));
  response->printf_P(PSTR("$(\"#system_buttonfunc\").selectmenu(\"refresh\");\r\n"));

  response->printf_P(PSTR("$(\"#status_cam\").hide();\r\n"));
  response->printf_P(PSTR("$(\"#status_cam_xtr\").hide();\r\n"));
#endif

#if !defined(ENABLE_HUE)
  response->printf_P(PSTR("rL(\'include_hue.html\');\r\n"));
  response->printf_P(PSTR("$(\"#status_hue\").addClass(\"ui-disabled\");\r\n"));
  response->printf_P(PSTR("$('#status_hue').remove();\r\n"));

  response->printf_P(PSTR("$(\"#system_buttonfunc option[value=5]\").remove();\r\n"));
  response->printf_P(PSTR("$(\"#system_buttonfunc\").selectmenu(\"refresh\");\r\n"));
#endif

#if !defined(ENABLE_DREAMSCREEN)
  response->printf_P(PSTR("rL(\'include_dreamscreen.html\');\r\n"));
  response->printf_P(PSTR("$(\"#status_dreamScreen\").addClass(\"ui-disabled\");\r\n"));
  response->printf_P(PSTR("$(\"#status_dreamScreen\").addClass(\"ui-disabled\");\r\n"));
  response->printf_P(PSTR("$('#status_dreamScreen').remove();\r\n"));

  response->printf_P(PSTR("$(\"#system_buttonfunc option[value=6]\").remove();\r\n"));
  response->printf_P(PSTR("$(\"#system_buttonfunc\").selectmenu(\"refresh\");\r\n"));
#endif

#if ESP32_DEVICE_TYPE == ESP32_BRIDGE
  response->printf_P(PSTR("rL(\'include_ambient.html\');\r\n"));
  response->printf_P(PSTR("$(\"#system_debug_toggle\").hide();\r\n"));
#endif

  if ( global::m_pLedHelper->GetLedMode() == LED_MODE_VIDEO )
    response->printf_P(PSTR("if ($(\"#mode-video\").hasClass('ui-disabled')) { $(\"#mode-video\").removeClass('ui-disabled'); $(\"#mode-video\").click(); $(\"#mode-video\").addClass('ui-disabled'); } else { $(\"#mode-video\").click(); }\r\n"));
  else if ( global::m_pLedHelper->GetLedMode() == LED_MODE_CAM)
    response->printf_P(PSTR("$(\"#mode-cam\").click();\r\n"));
  else if ( global::m_pLedHelper->GetLedMode() == LED_MODE_AMBIENT)
    response->printf_P(PSTR("$(\"#mode-ambient\").click();\r\n"));
  else if ( global::m_pLedHelper->GetLedMode() == LED_MODE_RELAY)
    response->printf_P(PSTR("if ($(\"#mode-relay\").hasClass('ui-disabled')) { $(\"#mode-relay\").removeClass('ui-disabled'); $(\"#mode-relay\").click(); $(\"#mode-relay\").addClass('ui-disabled'); } else { $(\"#mode-relay\").click(); }\r\n"));
  else
    response->printf_P(PSTR("$(\"#mode-off\").click();\r\n"));

#if defined(ENABLE_CAM)
  response->printf_P(PSTR("window.ab.cmfc = %d;\n"), CAM_BUFFER_COUNT_MAX );
  response->printf_P(PSTR("$(\"#camera_autoonoffdelay\").val(\"%.2d:%.2d\");\r\n"), global::GetCamAutoOnOffDelay() / 60, global::GetCamAutoOnOffDelay() % 60);
#endif

  response->printf_P(PSTR("$(\"#system_buttonfunc\").val(\"%d\").selectmenu(\"refresh\");\r\n"), global::GetButtonFunction() );
  response->printf_P(PSTR("$(\"#system_wififail\").val(\"%d\").selectmenu(\"refresh\");\r\n"), global::GetWifiFailHandling() );

  response->printf_P(PSTR("$(\"#slider_setupVertical\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetSetupVertical() );
  response->printf_P(PSTR("$(\"#slider_setupHorizontal\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetSetupHorizontal() );
  response->printf_P(PSTR("$('#radio_setupDirection').prop('checked', %s).checkboxradio('refresh');\r\n"), (global::m_pLedHelper->GetSetupDirection() == 1 ? "true" : "false") );
  response->printf_P(PSTR("$(\"#slider_setupFrameLength\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetFrameLength() );
  response->printf_P(PSTR("$(\"#slider_setupFramedelay\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetDelayLength() );
  response->printf_P(PSTR("$(\"#slider_setupFramesmoothing\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetFrameSmoothing() );

  response->printf_P(PSTR("$(\"#slider_colorRed\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetColorCorRed() );
  response->printf_P(PSTR("$(\"#slider_colorGreen\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetColorCorGreen() );
  response->printf_P(PSTR("$(\"#slider_colorBlue\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetColorCorBlue() );
  response->printf_P(PSTR("$(\"#slider_colorBoost\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetColorBoost() );
  response->printf_P(PSTR("$(\"#slider_colorBoostBalance\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetColorBoostBalance() );
  response->printf_P(PSTR("$(\"#slider_colorBoostMin\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetColorBoostMin() );
  response->printf_P(PSTR("$(\"#slider_colorBoostRed\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetColorBoostRed() );
  response->printf_P(PSTR("$(\"#slider_colorBoostGreen\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetColorBoostGreen() );
  response->printf_P(PSTR("$(\"#slider_colorBoostBlue\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetColorBoostBlue() );
  response->printf_P(PSTR("$(\"#slider_colorBoostThreshold\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetColorBoostThreshold() );

  response->printf_P(PSTR("$(\"#slider_colorValueGain\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetColorValueGain() );
  response->printf_P(PSTR("$(\"#slider_colorMaxBrightness\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetBrightness() );
  response->printf_P(PSTR("$(\"#slider_colorMinimumLuminosity\").val(\"%d\").slider(\"refresh\");\r\n"), global::m_pLedHelper->GetMinimumLuminosity() );

  response->printf_P(PSTR("$(\"#color_Presets\").val(\"%d\").selectmenu(\"refresh\");\r\n"), global::GetColorPreset() );
  response->printf_P(PSTR("$(\"#brightness_Presets\").val(\"%d\").selectmenu(\"refresh\");\r\n"), global::GetBrightnessPreset() );
  response->printf_P(PSTR("$(\"#motion_Presets\").val(\"%d\").selectmenu(\"refresh\");\r\n"), global::GetMotionPreset() );

  if (global::GetColorPreset() == 999) {
    response->printf_P(PSTR("$(\"#color_enhancement_custom\").show();\r\n"));
  }
  if (global::GetBrightnessPreset() == 999) {
    response->printf_P(PSTR("$(\"#brightness_enhancement_custom\").show();\r\n"));
  }
  if (global::GetMotionPreset() == 999) {
    response->printf_P(PSTR("$(\"#motion_enhancement_custom\").show();\r\n"));
  }

  response->printf_P(PSTR("});\r\n"));

  request->send(response);
}

#if defined(ENABLE_CAM)
void handleControllersettingsCamera(AsyncWebServerRequest *request) {
  _DBGPRINTF(PSTR("WebServer: requested %s \n"), request->url().c_str());
  AsyncResponseStream *response = request->beginResponseStream(TEXT_JAVASCRIPT);

  response->printf_P(PSTR("sCP('#camera_point_1', %d, %d, 0, 0);\r\n"), global::m_pCamProcessor->getCameraPointX(POINT_TOP_LEFT), global::m_pCamProcessor->getCameraPointY(POINT_TOP_LEFT));
  response->printf_P(PSTR("sCP('#camera_point_2', %d, %d, -%d, 0);\r\n"), global::m_pCamProcessor->getCameraPointX(POINT_TOP_RIGHT), global::m_pCamProcessor->getCameraPointY(POINT_TOP_RIGHT), CAMPOINTSIZE);
  response->printf_P(PSTR("sCP('#camera_point_3', %d, %d, 0, -%d);\r\n"), global::m_pCamProcessor->getCameraPointX(POINT_BOTTOM_LEFT), global::m_pCamProcessor->getCameraPointY(POINT_BOTTOM_LEFT), CAMPOINTSIZE);
  response->printf_P(PSTR("sCP('#camera_point_4', %d, %d, -%d, -%d);\r\n"), global::m_pCamProcessor->getCameraPointX(POINT_BOTTOM_RIGHT), global::m_pCamProcessor->getCameraPointY(POINT_BOTTOM_RIGHT), CAMPOINTSIZE, CAMPOINTSIZE);

  response->printf_P(PSTR("$('#camera_CamPresets').val('%d').selectmenu('refresh');\r\n"), global::GetCamPreset() );
  response->printf_P(PSTR("$('#cam_exposure_Presets').val('%d').selectmenu('refresh');\r\n"), global::GetExposurePreset() );
  response->printf_P(PSTR("$('#cam_inner_Presets').val('%d').selectmenu('refresh');\r\n"), global::GetCamInnerPreset() );
  response->printf_P(PSTR("$('#camera_CamMode').val('%d').selectmenu('refresh');\r\n"), global::m_pCamProcessor->GetCamMode() );
  response->printf_P(PSTR("$('#slider_camDetailLevel').val('%d').slider('refresh');\r\n"), global::m_pCamProcessor->GetCamDetailLevel() );
  response->printf_P(PSTR("$('#slider_camInnerHori').val('%d').slider('refresh');\r\n"), int(global::m_pCamProcessor->GetCamInnerHori() * 100) );
  response->printf_P(PSTR("$('#slider_camInnerVert').val('%d').slider('refresh');\r\n"), int(global::m_pCamProcessor->GetCamInnerVert() * 100) );
  response->printf_P(PSTR("$('#camera_CamSmooth').val('%d').selectmenu('refresh');\r\n"), global::m_pCamProcessor->GetCamSmooth() );
  response->printf_P(PSTR("$('#slider_CamSmoothMaxFrames').val('%d').slider('refresh');\r\n"), global::m_pCamProcessor->GetCamSmoothMaxFrames() );
  response->printf_P(PSTR("$('#slider_CamSmoothMaxDiff').val('%d').slider('refresh');\r\n"), global::m_pCamProcessor->GetCamSmoothMaxDiff() );
  response->printf_P(PSTR("$('#camera_Performance').val('%d').selectmenu('refresh');\r\n"), global::GetCamPerformance() );
  response->printf_P(PSTR("$('#camera_resolution').val('%d').selectmenu('refresh');\r\n"), global::GetCamResolution() );
  response->printf_P(PSTR("$('#camera_camFreq').val('%d').selectmenu('refresh');\r\n"), global::GetCamFreq() );
  response->printf_P(PSTR("$('#camera_camFbcount').val('%d').selectmenu('refresh');\r\n"), global::GetCamFrameBuffer() );
  response->printf_P(PSTR("$('#camera_camFlicker').val('%d').selectmenu('refresh');\r\n"), global::GetCamFlicker() );
# if CAM_SENSOR == CAM_SENSOR_OV3660
  response->printf_P(PSTR("$('#camera_overclock').prop('checked', %s).checkboxradio('refresh');\r\n"), (global::GetCamOverclock() == 1 ? "true" : "false") );
#endif
  response->printf_P(PSTR("$('#slider_camQuality').val('%d').slider('refresh');\r\n"), global::GetCamQuality() );
  response->printf_P(PSTR("$('#cam_color_Presets').val('%d').selectmenu('refresh');\r\n"), global::GetColorPreset() );
  response->printf_P(PSTR("$('#cam_slider_colorBoost').val('%d').slider('refresh');\r\n"), global::m_pLedHelper->GetColorBoost() );
  response->printf_P(PSTR("$('#cam_slider_colorBoostBalance').val('%d').slider('refresh');\r\n"), global::m_pLedHelper->GetColorBoostBalance() );
  response->printf_P(PSTR("$('#cam_slider_colorBoostMin').val('%d').slider('refresh');\r\n"), global::m_pLedHelper->GetColorBoostMin() );
  response->printf_P(PSTR("$('#cam_slider_colorBoostRed').val('%d').slider('refresh');\r\n"), global::m_pLedHelper->GetColorBoostRed() );
  response->printf_P(PSTR("$('#cam_slider_colorBoostGreen').val('%d').slider('refresh');\r\n"), global::m_pLedHelper->GetColorBoostGreen() );
  response->printf_P(PSTR("$('#cam_slider_colorBoostBlue').val('%d').slider('refresh');\r\n"), global::m_pLedHelper->GetColorBoostBlue() );
  response->printf_P(PSTR("$('#cam_slider_colorBoostThreshold').val('%d').slider('refresh');\r\n"), global::m_pLedHelper->GetColorBoostThreshold() );
  response->printf_P(PSTR("$('#cam_brightness_Presets').val('%d').selectmenu('refresh');\r\n"), global::GetBrightnessPreset() );
  response->printf_P(PSTR("$('#cam_slider_colorValueGain').val('%d').slider('refresh');\r\n"), global::m_pLedHelper->GetColorValueGain() );
  response->printf_P(PSTR("$('#cam_slider_colorMinimumLuminosity').val('%d').slider('refresh');\r\n"), global::m_pLedHelper->GetMinimumLuminosity() );
  response->printf_P(PSTR("$('#camera_WbMode').val('%d').selectmenu('refresh');\r\n"), global::GetCamWbMode() );
  response->printf_P(PSTR("$('#slider_camColorRed').val('%d').slider('refresh');\r\n"), global::GetCamColorCorRed() );
  response->printf_P(PSTR("$('#slider_camColorGreen').val('%d').slider('refresh');\r\n"), global::GetCamColorCorGreen() );
  response->printf_P(PSTR("$('#slider_camColorBlue').val('%d').slider('refresh');\r\n"), global::GetCamColorCorBlue() );
  response->printf_P(PSTR("$('#slider_camContrast').val('%d').slider('refresh');\r\n"), global::GetCamContrast() );
  response->printf_P(PSTR("$('#slider_camBrightness').val('%d').slider('refresh');\r\n"), global::GetCamBrightness() );
  response->printf_P(PSTR("$('#slider_camSaturation').val('%d').slider('refresh');\r\n"), global::GetCamSaturation() );
  response->printf_P(PSTR("$('#slider_camAgcGain').val('%d').slider('refresh');\n"), global::GetCamAgcGain() );
  response->printf_P(PSTR("$('#slider_camAeLevels').val('%d').slider('refresh');\r\n"), global::GetCamAeLevels() );
  response->printf_P(PSTR("$('#slider_camAecValue').val('%d').slider('refresh');\r\n"), global::GetCamAecValue() );
  response->printf_P(PSTR("$('#slider_camAecMinValue').val('%d').slider('refresh');\r\n"), global::GetCamAecMinValue() );
  response->printf_P(PSTR("$('#slider_camAecMaxValue').val('%d').slider('refresh');\r\n"), global::GetCamAecMaxValue() );
  response->printf_P(PSTR("$('#slider_camWhiteActThreshold').val('%d').slider('refresh');\r\n"), global::GetCamWhiteActivationTreshold() );
  response->printf_P(PSTR("$('#slider_camBeforeWhiteActivationTreshold').val('%d').slider('refresh');\r\n"), global::GetCamBeforeWhiteActivationTreshold() );
  response->printf_P(PSTR("$('#slider_camWhiteMinimumAmount').val('%d').slider('refresh');\r\n"), global::GetCamWhiteMinimumAmount() );
  response->printf_P(PSTR("$('#slider_camAdaptiveChangePercent').val('%d').slider('refresh');\r\n"), global::GetCamAdaptiveChangePercent() );
  response->printf_P(PSTR("$('#slider_camAdaptiveChangeDelay').val('%d').slider('refresh');\r\n"), global::GetCamAdaptiveChangeDelay() );
  response->printf_P(PSTR("$('#slider_camAdaptiveGainDown').val('%d').slider('refresh');\r\n"), global::GetCamAdaptiveGainDown() );
  response->printf_P(PSTR("$('#slider_camAdaptiveGainUp').val('%d').slider('refresh');\r\n"), global::GetCamAdaptiveGainUp() );
  response->printf_P(PSTR("$('#slider_camAdaptiveGainMax').val('%d').slider('refresh');\r\n"), global::GetCamAdaptiveGainMax() );
  response->printf_P(PSTR("$('#slider_camActivationThreshold').val('%d').slider('refresh');\r\n"), global::GetCamActivationThreshold() );
  response->printf_P(PSTR("$('#slider_camsBlackthresholdSmooth').val('%d').slider('refresh');\r\n"), global::m_pCamProcessor->GetCamBlackThresholdSmoothing() );
  response->printf_P(PSTR("$('#slider_camBlackRemoveTreshold').val('%d').slider('refresh');\r\n"), global::GetCamBlackRemoveThreshold() );
  response->printf_P(PSTR("$('#slider_camMinPixelAmount').val('%d').slider('refresh');\r\n"), global::GetCamMinPixelAmount() );
  response->printf_P(PSTR("$('#slider_camAgcGain').slider('%s');\r\n"), ( global::GetCamGaincontrol() == 0 ? "enable" : "disable") );
  response->printf_P(PSTR("$('#camera_WbMode').selectmenu('%s');\r\n"), ( global::GetCamAwbGainControl() == 1 ? "enable" : "disable") );
  global::SetCamPreset(global::GetCamPreset(), response, false, true);
  global::SetExposurePreset(global::GetExposurePreset(), response, false, true);
  global::SetCamInnerPreset(global::GetCamInnerPreset(), response, false, true);
  response->printf_P(PSTR("CamFile = '%s';\r\n"), (psramFound() && global::m_bUltraDebugEnabled ? PSTR("cam.bmp") : PSTR("cam.jpg")) );
  if (!psramFound()) {
    response->printf_P(PSTR("$(':input[name= \"camera_HighQuality\"]').checkboxradio('disable');\r\n"));
  }
  if (global::GetColorPreset() == 999) {
    response->printf_P(PSTR("$('#cam_color_enhancement_custom').show();\r\n"));
  }
  if (global::GetBrightnessPreset() == 999) {
    response->printf_P(PSTR("$('#cam_brightness_enhancement_custom').show();\r\n"));
  }
  if (global::m_pCamProcessor->GetCamMode() == LED_MODE_CAM_DETAIL) {
    response->printf_P(PSTR("$('#camera_CamMode_custom').show();\r\n"));
  }
  if (global::m_pCamProcessor->GetCamSmooth() == CAM_SMOOTH_MODE_CUSTOM) {
    response->printf_P(PSTR("$('#camera_CamSmooth_custom').show();\r\n"));
  }
  response->printf_P(PSTR("setTimeout(function() {\r\n"));
  response->printf_P(PSTR("sCP('#camera_point_1', %d, %d, 0, 0);\r\n"), global::m_pCamProcessor->getCameraPointX(POINT_TOP_LEFT), global::m_pCamProcessor->getCameraPointY(POINT_TOP_LEFT));
  response->printf_P(PSTR("sCP('#camera_point_2', %d, %d, -%d, 0);\r\n"), global::m_pCamProcessor->getCameraPointX(POINT_TOP_RIGHT), global::m_pCamProcessor->getCameraPointY(POINT_TOP_RIGHT), CAMPOINTSIZE);
  response->printf_P(PSTR("sCP('#camera_point_3', %d, %d, 0, -%d);\r\n"), global::m_pCamProcessor->getCameraPointX(POINT_BOTTOM_LEFT), global::m_pCamProcessor->getCameraPointY(POINT_BOTTOM_LEFT), CAMPOINTSIZE);
  response->printf_P(PSTR("sCP('#camera_point_4', %d, %d, -%d, -%d);\r\n"), global::m_pCamProcessor->getCameraPointX(POINT_BOTTOM_RIGHT), global::m_pCamProcessor->getCameraPointY(POINT_BOTTOM_RIGHT), CAMPOINTSIZE, CAMPOINTSIZE);
  response->printf_P(PSTR("}, 500);\r\n"));
  request->send(response);
}
#endif

#if defined(ENABLE_DREAMSCREEN)
void handleControllerSettingsDreamScreen(AsyncWebServerRequest *request) {
  _DBGPRINTF(PSTR("WebServer: requested %s \n"), request->url().c_str());
  AsyncResponseStream *response = request->beginResponseStream(TEXT_JAVASCRIPT);

  response->printf_P(PSTR("window.dreamscreenGroups = [];\n"));
  response->printf_P(PSTR("window.dreamscreenGroups.ids = [];\n"));
  response->printf_P(PSTR("window.dreamscreenGroups.groupnames = [];\n"));

  int iGroupCount = global::m_pDreamScreen->GetDreamscreenGroupsCount();
  for(int i = 0; i < iGroupCount; i++) {
    response->printf_P(PSTR("window.dreamscreenGroups.ids[%d] = %d;\n"), i, global::m_pDreamScreen->GetDreamscreenGroupID(i) );
    response->printf_P(PSTR("window.dreamscreenGroups.groupnames[%d] = \"%s\";\n"), i, global::m_pDreamScreen->GetDreamscreenGroupName(i) );
  }

  response->printf_P(PSTR("window.dreamscreenGroups.groupcount = %d;\n"), iGroupCount );

  response->printf_P(PSTR("window.dreamscreenGroups.selectedgroup = %d;\n"), global::GetDreamScreenGroup() );
  response->printf_P(PSTR("$(\"#dreamscreen_group\").val(\"%d\").selectmenu(\"refresh\");\r\n"), global::GetDreamScreenGroup() );
  response->printf_P(PSTR("$(\"#dreamscreen_lowcolorsmoothing\").val(\"%d\").slider(\"refresh\");\r\n"), global::GetDreamScreenLowColorSmoothing() );

  response->printf_P(PSTR("\n"));

  request->send(response);
}

void handleRefreshDreamScreenDevices(AsyncWebServerRequest *request) {
  _DBGPRINTF(PSTR("WebServer: handleRefreshDreamScreenDevices\n"));

  global::DiscoverDreamScreenDevices();

  request->send(200, TEXT_PLAIN, request->url() + ": Discover requested");
}

void handleAddDreamScreenToNetwork(AsyncWebServerRequest *request) {
  _DBGPRINTF(PSTR("WebServer: handleAddDreamScreenToNetwork\n"));

  AsyncResponseStream *response = request->beginResponseStream(APP_JSON);

  String strParam;
  bool bSuccess = false;
  if (request->hasParam("value")) {
    strParam = request->getParam("value")->value();

    global::m_pDreamScreen->SetAddDreamScreenToNetwork(strParam.c_str());
    global::SetDoAction(ACTION_AddDreamScreenToNetwork);

    response->printf_P(PSTR("{\n\t\"status\": 1,\n\t\"action\": \"startAddDreamScreenToNetwork\"\n}\n"));
  } else {
    response->printf_P(PSTR("{\n\t\"status\": 0,\n\t\"error\": \"Missing VALUE Parameter\"\n}\n"));
  }
  request->send(response);
}
#endif

#if defined(ENABLE_HUE)
void handleControllerSettingsHue(AsyncWebServerRequest *request) {
  _DBGPRINTF(PSTR("WebServer: requested %s \n"), request->url().c_str());
  AsyncResponseStream *response = request->beginResponseStream(TEXT_JAVASCRIPT);

  String strHueID;
  if (request->hasParam("id")) {
    strHueID = request->getParam("id")->value();

    response->printf_P(PSTR("window.hueVars['%s'] = [];\n"), strHueID.c_str());
    response->printf_P(PSTR("window.hueVars['%s'].enabled = %s;\n"), strHueID.c_str(), (global::GetHueLightState((char*) strHueID.c_str()) == 1 ? "true" : "false"));
    response->printf_P(PSTR("window.hueVars['%s'].zones = [];\n"), strHueID.c_str());
    for(int i = 0; i < MAX_HUE_LEDS; i++) {
      response->printf_P(PSTR("window.hueVars['%s'].zones[%d] = %s;\n"), strHueID.c_str(), i, (global::GetHueLightZoneState((char*) strHueID.c_str(), i) == 1 ? "true" : "false"));
    }
    response->printf_P(PSTR("window.hueVars['%s'].colcorred = '%d';\n"), strHueID.c_str(), global::GetHueLightColCorRed((char*) strHueID.c_str()));
    response->printf_P(PSTR("window.hueVars['%s'].colcorgreen = '%d';\n"), strHueID.c_str(), global::GetHueLightColCorGreen((char*) strHueID.c_str()));
    response->printf_P(PSTR("window.hueVars['%s'].colcorblue = '%d';\n"), strHueID.c_str(), global::GetHueLightColCorBlue((char*) strHueID.c_str()));
    response->printf_P(PSTR("window.hueVars['%s'].colbrightness = '%d';\n"), strHueID.c_str(), global::GetHueLightColBrightness((char*) strHueID.c_str()));
  } else {
    response->printf_P(PSTR("$(\"#slider_smart_FrameLength\").val(\"%d\").slider(\"refresh\");\r\n"), global::GetFrameLengthDtls() );
    response->printf_P(PSTR("$(\"#slider_smart_HandshakeTimeout\").val(\"%d\").slider(\"refresh\");\r\n"), global::GetHandshakeTimeout() );
    response->printf_P(PSTR("$(\"#hue_Presets\").val(\"%d\").selectmenu(\"refresh\");\r\n"), global::GetHuePreset() );

    if (global::GetHuePreset() == 999) {
      response->printf_P(PSTR("$(\"#hue_enhancement_custom\").show();\r\n"));
    }
  }

  request->send(response);
}

void handleRefreshHueDevices(AsyncWebServerRequest *request) {
  _DBGPRINTF(PSTR("WebServer: handleRefreshHueDevices\n"));

  global::m_pHueHelper->InitHueLights();

  request->send(200, TEXT_PLAIN, request->url() + ": Init requested");
}
#endif
