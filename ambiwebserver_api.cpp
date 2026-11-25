/*
 * AmbiWebServer: REST/API and parameter handlers (setparam, getparam, JSON endpoints).
 * Route registration remains in ambiwebserver.cpp (InitServer).
 */
#include "globalincludes.h"
#include "ambiwebserver.h"
#include "ambiwebserver_handlers.h"
#include "udpserver.h"

#include "esp_wifi.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#if __has_include("esp_idf_version.h")
# include "esp_idf_version.h"
#endif

#if defined(ENABLE_CAM)
# include "camprocessor.h"
#endif

/* Parse one hex digit (0-9, A-F, a-f). Invalid characters yield 0. */
static byte hexNibble(char c) {
  if (c >= '0' && c <= '9') return (byte)(c - '0');
  if (c >= 'A' && c <= 'F') return (byte)(c - 'A' + 10);
  if (c >= 'a' && c <= 'f') return (byte)(c - 'a' + 10);
  return 0;
}
/* Parse the two hex digits at position idx of str into one byte. */
static byte hexByteAt(const String &str, unsigned int idx) {
  return (byte)((hexNibble(str.charAt(idx)) << 4) | hexNibble(str.charAt(idx + 1)));
}

void handleSetColorMode(AsyncWebServerRequest *request) {
  String strResult; 
  String strMode;
  if (request->hasParam("mode")) {
    strMode = request->getParam("mode")->value();

    if (global::m_pLedHelper != NULL) {
      int iNewMode = strMode.toInt(); 
      if (  iNewMode >= LED_MODE_MIN && 
            iNewMode <= LED_MODE_MAX ) {
        global::m_pLedHelper->SetChangeLedMode((byte)iNewMode); 
        strResult = PSTR("Switched to MODE") + strMode;
#ifdef ENABLE_SSD1306
        global::oledDrawStringQueue(strResult, "", "");
#endif
      } else {
        strResult = PSTR("ERROR, invalid MODE.");
      }
    } else {
      strResult = PSTR("INTERNAL ERROR, LED Handler not set.");
    }
  } else {
    // String to short
    strResult = PSTR("ERROR, mode value missing");
  }

  _DBGPRINTF(PSTR("WebServer: handleSetColorMode: %s\n"), strResult.c_str());
  request->send(200, TEXT_PLAIN, request->url() + ": " + strResult);
}
void handleSetColor(AsyncWebServerRequest *request) {
  String strResult; 
  String strColor;
  if (request->hasParam("color")) {
      strColor = request->getParam("color")->value();
      /* Color needs to match 8 bytes */
      if (strColor.length() == 8) {
        if (global::m_pLedHelper == NULL) {
          strResult = PSTR("ERROR, LED Helper not set");
          _DBGPRINTF(PSTR("WebServer: handleSetColor: %s\n"), strResult.c_str());
          request->send(200, TEXT_PLAIN, request->url() + ": " + strResult);
          return; 
        }

        /* Get Color Codes */  
        byte bColorWhite = hexByteAt(strColor, 0);
        byte bColorRed   = hexByteAt(strColor, 2);
        byte bColorGreen = hexByteAt(strColor, 4);
        byte bColorBlue  = hexByteAt(strColor, 6);
                            
        if (global::m_pLedHelper != NULL) {
          /* Set LED to AMBIENT */
          global::m_pLedHelper->SetChangeLedMode(LED_MODE_AMBIENT); 
          global::m_pLedHelper->setFullColor(bColorWhite, bColorRed, bColorGreen, bColorBlue); 
        }

        _DBGPRINTF(PSTR("WebServer: handleSetColor Successfully set Colorcode to %s\n"), strColor.c_str()); 
        strResult = PSTR("Successfully set Colorcode to ") + strColor; 
      } else {
        // String to short
        strResult = PSTR("ERROR, colors invalid string to short"); 
      }
  } else {
      strResult = PSTR("ERROR, Missing color parameter");
  }

  _DBGPRINTF(PSTR("WebServer: handleSetColor: %s\n"), strResult.c_str());
  request->send(200, TEXT_PLAIN, request->url() + ": " + strResult);
}
void handleSetAction(AsyncWebServerRequest *request) {
  /* Get Time start */
  unsigned long timeNow = millis(); 
  
  String strResult;
  String strColors;
  String strMode;

  if (request->hasParam("mode")) {
    strMode = request->getParam("mode")->value();
    if (strMode.equals("setcolorblink")) {
      // Process Color Blink!      
      if (request->hasParam("color")) {
          strColors = request->getParam("color")->value();
          /* Color needs to match 8 bytes */
          if (strColors.length() == 8) {
            if (global::m_pLedHelper == NULL) {
              request->send(200, TEXT_PLAIN, PSTR("setcolorblink: ERROR, LED Helper not set"));
              return; 
            } else {
              _DBGPRINTF(PSTR("handleSetColorBlink: %s\n"), strColors.c_str());
            }
    
            /* Get Color Codes */  
            byte bColorWhite = hexByteAt(strColors, 0);
            byte bColorRed   = hexByteAt(strColors, 2);
            byte bColorGreen = hexByteAt(strColors, 4);
            byte bColorBlue  = hexByteAt(strColors, 6);
                                
            if (global::m_pLedHelper != NULL) {
              /* Set LED to AMBIENT */
              global::m_pLedHelper->SetChangeLedMode(LED_MODE_AMBIENT); 
              global::m_pLedHelper->colorBlink(bColorRed, bColorGreen, bColorBlue, 16, true); 
            }
    
            _DBGPRINTF(PSTR("WebServer: handleSetColorBlink Successfully set Colorcode to %s\n"), strColors.c_str()); 
            strResult = PSTR("Successfully set Colorcode to ") + strColors; 
          } else {
            // String to short
            strResult = PSTR("ERROR, colors invalid string to short");
          }
      } else {
          strResult = PSTR("ERROR, Missing color parameter");
      }
    } else if (strMode.equals("setcolors")) {
      if (request->hasParam("colors")) {
          strColors = request->getParam("colors")->value();
          /* Colors need to devideable through 8! */
          if (strColors.length() % 8 == 0) {
            if (global::m_pLedHelper == NULL) {
              request->send(200, TEXT_PLAIN, PSTR("setcolors: ERROR, LED Helper not set"));
              return; 
            }
    
            /* First 8 Bytes contain HEADER 
            * Byte1: LED Count - Max 255 
            * Byte2: Reserved
            * Byte3: Reserved
            * Byte4: Reserved
            */
            byte bLeds =  hexByteAt(strColors, 0);
            byte bByte2=  hexByteAt(strColors, 2);
            byte bByte3=  hexByteAt(strColors, 4);
            byte bByte4=  hexByteAt(strColors, 6);
    
            /* Set LED to VIDEO */
            global::m_pLedHelper->SetChangeLedMode(LED_MODE_VIDEO); 
            /* Set current LED Count */
            global::m_pLedHelper->SetCurrentLedCount((uint16_t)bLeds); // actual count 0-255; we store count-1 
    
            /* Process Color Bytes */
            int iColorIndex = 0; 
            for(int i=8; i < strColors.length(); i+=8) {
              /* Get Color Codes */  
              byte bColorWhite = hexByteAt(strColors, i+0);
              byte bColorRed   = hexByteAt(strColors, i+2);
              byte bColorGreen = hexByteAt(strColors, i+4);
              byte bColorBlue  = hexByteAt(strColors, i+6);
    
              /* Set Color LED Code */
              global::m_pLedHelper->SetLedColorByIdx(iColorIndex, bColorWhite, bColorRed, bColorGreen, bColorBlue); 
              iColorIndex++; 
            }
            /* Process new LED Data */
            global::m_pLedHelper->SetIsNewLedData(true); 
            _DBGPRINTF(PSTR("WebServer: handleSetMultipleColors started, processing took %d ms\n"), (millis() - timeNow)); 
            strResult = PSTR("Successfully set Colors."); 
          } else {
            // String to short
            strResult = PSTR("ERROR, colors invalid or string to short");
          }
      } else {
          strColors = PSTR("Missing color parameter!");
      }
    }
  }

  _DBGPRINTF(PSTR("WebServer: handleSetAction: %s %s\n"), strMode.c_str(), strResult.c_str());
  request->send(200, TEXT_PLAIN, request->url() + ": " + strResult);
}

void handleGetParameter(AsyncWebServerRequest *request) {
  String strResult; 

  if (global::m_pLedHelper != NULL) {
    // Begin JSON String
    strResult = PSTR("{\n\"Value\":\t");
    
    // Check which parameter to set:
    if (request->url().equals("/getdevicename")) {
      strResult += global::GetDeviceName();
    } else if (request->url().equals("/getdeviceid")) {
      strResult += global::GetDeviceID();
    } else if (request->url().equals("/getdeviceidmac")) {
      strResult += global::GetDeviceIDMac();
    } else if (request->url().equals("/getframelength")) {
      strResult += global::m_pLedHelper->GetFrameLength();
    } else if (request->url().equals("/getframedelay")) {
      strResult += global::m_pLedHelper->GetDelayLength();
    } else if (request->url().equals("/getcolorred")) {
      strResult += global::m_pLedHelper->GetColorCorRed();
    } else if (request->url().equals("/getcolorgreen")) {
      strResult += global::m_pLedHelper->GetColorCorGreen();
    } else if (request->url().equals("/getcolorblue")) {
      strResult += global::m_pLedHelper->GetColorCorBlue();
    } else if (request->url().equals("/getminimumluminosity")) {
      strResult += global::m_pLedHelper->GetMinimumLuminosity();
    } else if (request->url().equals("/getbrightness")) {
      strResult += global::m_pLedHelper->GetBrightness();
    } else if (request->url().equals("/getcolorboost")) {
      strResult += global::m_pLedHelper->GetColorBoost();
    } else if (request->url().equals("/getcolorboostmin")) {
      strResult += global::m_pLedHelper->GetColorBoostMin();
    } else if (request->url().equals("/getcolorvaluegain")) {
      strResult += global::m_pLedHelper->GetColorValueGain();
    } else if (request->url().equals("/getvertical")) {
      strResult += global::m_pLedHelper->GetSetupVertical();
    } else if (request->url().equals("/gethorizontal")) {
      strResult += global::m_pLedHelper->GetSetupHorizontal();
    } else if (request->url().equals("/getdirection")) {
      strResult += global::m_pLedHelper->GetSetupDirection();
    } else if (request->url().equals("/getsidetop")) {
      strResult += global::m_pLedHelper->GetSetupSideTop();
    } else if (request->url().equals("/getsideleft")) {
      strResult += global::m_pLedHelper->GetSetupSideLeft();
    } else if (request->url().equals("/getsideright")) {
      strResult += global::m_pLedHelper->GetSetupSideRight();
    } else if (request->url().equals("/getsidebottom")) {
      strResult += global::m_pLedHelper->GetSetupSideBottom();
    } else if (request->url().equals("/getledmode")) {
      strResult += global::m_pLedHelper->GetLedMode();
#if defined(ENABLE_CAM)
    } else if (request->url().equals("/getcamerapoint")) {
      String strType;
      if (request->hasParam("point") &&
          request->hasParam("type")) {
        int iIndex = request->getParam("point")->value().toInt();
        strType = request->getParam("type")->value();
        if (strType.equals("x")) {
          strResult += global::m_pCamProcessor->getCameraPointX((byte)iIndex);
        } else {
          strResult += global::m_pCamProcessor->getCameraPointY((byte)iIndex);
        }
      } else {
        // String to short
        strResult = PSTR("ERROR, invalid point");
      }
    } else if (request->url().equals("/getcammode")) {
      strResult += global::m_pCamProcessor->GetCamMode();
    } else if (request->url().equals("/getcammode")) {
      strResult += global::m_pCamProcessor->GetCamMode();
    } else if (request->url().equals("/getcamdetaillevel")) {
      strResult += global::m_pCamProcessor->GetCamDetailLevel();
#endif      
    } else {
      strResult = PSTR("ERROR, unknown Parameter.");
    }

    // End JSON String
    strResult += "\n}"; 
  } else {
    strResult = PSTR("INTERNAL ERROR, LED Handler not set.");
  }

  _DBGPRINTF(PSTR("WebServer: handleGetParameter: %s\n"), strResult.c_str());
  request->send(200, TEXT_PLAIN, strResult);
}

void handleSetParameterPresets(AsyncWebServerRequest *request) {
  String strResult; 
  String strValue;
  String strParam;

  _DBGPRINTF(PSTR("WebServer: handleSetParameterPresets %s \n"), (request->hasParam("param") ? request->getParam("param")->value().c_str() : "NULL") );

  // Start response
  AsyncResponseStream *response = request->beginResponseStream(TEXT_JAVASCRIPT);

  if (request->hasParam("param")) {
    strParam = request->getParam("param")->value();
    if (request->hasParam("value")) {
      strValue = request->getParam("value")->value();
  
      if (strParam.equals("setcolorpreset")) {
        /* Convert to INT */
        int iNew = strValue.toInt(); 
        global::SetColorPreset(iNew);
        strResult = PSTR("Changed ColorPreset to " + strValue);
  
        // Output changes for control updates
        response->printf_P(PSTR("$(\"#slider_colorBoost\").val(\"%d\").slider(\"refresh\");\n"), global::m_pLedHelper->GetColorBoost() );
        response->printf_P(PSTR("$(\"#slider_colorBoostMin\").val(\"%d\").slider(\"refresh\");\n"), global::m_pLedHelper->GetColorBoostMin() );
        response->printf_P(PSTR("$(\"#slider_colorBoostThreshold\").val(\"%d\").slider(\"refresh\");\n"), global::m_pLedHelper->GetColorBoostThreshold() );
  #if defined(ENABLE_CAM)
        // Output changes for control updates
        response->printf_P(PSTR("$(\"#cam_slider_colorBoost\").val(\"%d\").slider(\"refresh\");\n"), global::m_pLedHelper->GetColorBoost() );
        response->printf_P(PSTR("$(\"#cam_slider_colorBoostMin\").val(\"%d\").slider(\"refresh\");\n"), global::m_pLedHelper->GetColorBoostMin() );
        response->printf_P(PSTR("$(\"#cam_slider_colorBoostThreshold\").val(\"%d\").slider(\"refresh\");\n"), global::m_pLedHelper->GetColorBoostThreshold() );
  #endif      
      } else if (strParam.equals("setbrightnesspreset")) {
        /* Convert to INT */
        int iNew = strValue.toInt(); 
        global::SetBrightnessPreset(iNew);
        strResult = PSTR("Changed BrightnessPreset to " + strValue);
  
        // Output changes for control updates
        response->printf_P(PSTR("$(\"#slider_colorMaxBrightness\").val(\"%d\").slider(\"refresh\");\n"), global::m_pLedHelper->GetBrightness() );
        response->printf_P(PSTR("$(\"#slider_colorValueGain\").val(\"%d\").slider(\"refresh\");\n"), global::m_pLedHelper->GetColorValueGain() );
      } else if (strParam.equals("setmotionpreset")) {
        /* Convert to INT */
        int iNew = strValue.toInt(); 
        global::SetMotionPreset(iNew);
        strResult = PSTR("Changed MotionPreset to " + strValue);
  
        // Output changes for control updates
        response->printf_P(PSTR("$(\"#slider_setupFramesmoothing\").val(\"%d\").slider(\"refresh\");\n"), global::m_pLedHelper->GetFrameSmoothing() );
        response->printf_P(PSTR("$(\"#slider_setupFrameLength\").val(\"%d\").slider(\"refresh\");\n"), global::m_pLedHelper->GetFrameLength() );
        response->printf_P(PSTR("$(\"#slider_setupFramedelay\").val(\"%d\").slider(\"refresh\");\n"), global::m_pLedHelper->GetDelayLength() );
  #if defined(ENABLE_CAM)
      } else if (strParam.equals("setcampreset")) {
        /* Convert to INT */
        int iNew = strValue.toInt(); 
        global::SetCamPreset(iNew, response);
        strResult = PSTR("Changed CamPreset to " + strValue);
  
        // Output changes for control updates
        response->printf_P(PSTR("$(\"#camera_WbMode\").val(\"%d\").selectmenu(\"refresh\");\n"), global::GetCamWbMode() );
        response->printf_P(PSTR("$(\"#slider_camColorRed\").val(\"%d\").slider(\"refresh\");\n"), global::GetCamColorCorRed() );
        response->printf_P(PSTR("$(\"#slider_camColorGreen\").val(\"%d\").slider(\"refresh\");\n"), global::GetCamColorCorGreen() );
        response->printf_P(PSTR("$(\"#slider_camColorBlue\").val(\"%d\").slider(\"refresh\");\n"), global::GetCamColorCorBlue() );
        response->printf_P(PSTR("$(\"#slider_camContrast\").val(\"%d\").slider(\"refresh\");\n"), global::GetCamContrast() );
        response->printf_P(PSTR("$(\"#slider_camBrightness\").val(\"%d\").slider(\"refresh\");\n"), global::GetCamBrightness() );
        response->printf_P(PSTR("$(\"#slider_camSaturation\").val(\"%d\").slider(\"refresh\");\n"), global::GetCamSaturation() );
        response->printf_P(PSTR("$('#camera_Whitebalance').prop('checked', %s).checkboxradio('refresh');\r\n"),  (global::GetCamWhitebalance() == 1 ? "true" : "false") );
        response->printf_P(PSTR("$('#camera_AwbGainControl').prop('checked', %s).checkboxradio('refresh');\r\n"),(global::GetCamAwbGainControl() == 1 ? "true" : "false") );
  
        // Enable/Disable Optional Controls
        response->printf_P(PSTR("$(\"#camera_WbMode\").selectmenu(\"%s\");\r\n"), ( global::GetCamAwbGainControl() == 1 ? "enable" : "disable") );
      } else if (strParam.equals("setcamexposurepreset")) {
        /* Convert to INT */
        int iNew = strValue.toInt(); 
        global::SetExposurePreset(iNew, response);
        strResult = PSTR("Changed CamExposurePreset to " + strValue);
  
        // Output changes for control updates
        response->printf_P(PSTR("$('#camera_aec').prop('checked', %s).checkboxradio('refresh');\r\n"),           (global::GetCamAutoExposurecontrol() == 1 ? "true" : "false") );
        response->printf_P(PSTR("$('#camera_Aec2').prop('checked', %s).checkboxradio('refresh');\r\n"),          (global::GetCamAec2() == 1 ? "true" : "false") );
        response->printf_P(PSTR("$('#slider_camAeLevels').val('%d').slider('refresh');\n"),                       global::GetCamAeLevels() );
        response->printf_P(PSTR("$('#slider_camAecValue').val('%d').slider('refresh');\n"),                       global::GetCamAecValue() );
        response->printf_P(PSTR("$('#camera_Gaincontrol').prop('checked', %s).checkboxradio('refresh');\r\n"),   (global::GetCamGaincontrol() == 1 ? "true" : "false") );
        response->printf_P(PSTR("$('#slider_camAgcGain').val('%d').slider('refresh');\n"),                        global::GetCamAgcGain() );
        response->printf_P(PSTR("$('#camera_rawgma').prop('checked', %s).checkboxradio('refresh');\r\n"),        (global::GetCamRawGMA() == 1 ? "true" : "false") );
        response->printf_P(PSTR("$('#camera_lenc').prop('checked', %s).checkboxradio('refresh');\r\n"),          (global::GetCamLenc() == 1 ? "true" : "false") );
        // Adaptive Options
  
        response->printf_P(PSTR("$('#camera_adptexp').prop('checked', %s).checkboxradio('refresh');\r\n"),       (global::GetCamAdaptiveExposurecontrol() == 1 ? "true" : "false") );
        response->printf_P(PSTR("$('#camera_adptgain').prop('checked', %s).checkboxradio('refresh');\r\n"),      (global::GetCamAdaptiveGaincontrol() == 1 ? "true" : "false") );
        response->printf_P(PSTR("$('#slider_camAecMinValue').val('%d').slider('refresh');\r\n"),                  global::GetCamAecMinValue() );
        response->printf_P(PSTR("$('#slider_camAecMaxValue').val('%d').slider('refresh');\r\n"),                  global::GetCamAecMaxValue() );
        response->printf_P(PSTR("$('#slider_camWhiteActThreshold').val('%d').slider('refresh');\r\n"),            global::GetCamWhiteActivationTreshold() );
        response->printf_P(PSTR("$('#slider_camBeforeWhiteActivationTreshold').val('%d').slider('refresh');\r\n"),global::GetCamBeforeWhiteActivationTreshold() );
        response->printf_P(PSTR("$('#slider_camWhiteMinimumAmount').val('%d').slider('refresh');\r\n"),           global::GetCamWhiteMinimumAmount() );
        response->printf_P(PSTR("$('#slider_camAdaptiveChangePercent').val('%d').slider('refresh');\r\n"),        global::GetCamAdaptiveChangePercent() );
        response->printf_P(PSTR("$('#slider_camAdaptiveChangeDelay').val('%d').slider('refresh');\r\n"),          global::GetCamAdaptiveChangeDelay() );
        response->printf_P(PSTR("$('#slider_camAdaptiveGainDown').val('%d').slider('refresh');\r\n"),             global::GetCamAdaptiveGainDown() );
        response->printf_P(PSTR("$('#slider_camAdaptiveGainUp').val('%d').slider('refresh');\r\n"),               global::GetCamAdaptiveGainUp() );
        response->printf_P(PSTR("$('#slider_camAdaptiveGainMax').val('%d').slider('refresh');\r\n"),              global::GetCamAdaptiveGainMax() );
        
        // Enable/Disable Optional Controls
        response->printf_P(PSTR("$('#slider_camAgcGain').slider('%s');\r\n"), ( global::GetCamGaincontrol() == 0 ? "enable" : "disable") );
        // response->printf_P(PSTR("$('#camera_Aec2').checkboxradio('%s');\r\n"), ( global::GetCamAutoExposurecontrol() == 1 ? "enable" : "disable") );
  
      } else if (strParam.equals("setcaminnerpreset")) {
        /* Convert to INT */
        int iNew = strValue.toInt(); 
        global::SetCamInnerPreset(iNew, response);
        strResult = PSTR("Changed CamInnerPreset to " + strValue);
  
        // Output changes for control updates
        response->printf_P(PSTR("$('#slider_camInnerHori').val('%d').slider('refresh');\r\n"), int(global::m_pCamProcessor->GetCamInnerHori() * 100) );
        response->printf_P(PSTR("$('#slider_camInnerVert').val('%d').slider('refresh');\r\n"), int(global::m_pCamProcessor->GetCamInnerVert() * 100) );
  #endif
  #if defined(ENABLE_HUE)
      } else if (strParam.equals("sethuepreset")) {
        /* Convert to INT */
        int iNew = strValue.toInt(); 
        global::SetHuePreset(iNew);
        strResult = PSTR("Changed HuePreset to " + strValue);
  
        // Output changes for control updates
        response->printf_P(PSTR("$(\"#slider_smart_FrameLength\").val(\"%d\").slider(\"refresh\");\n"), global::GetFrameLengthDtls() );
        response->printf_P(PSTR("$(\"#slider_smart_HandshakeTimeout\").val(\"%d\").slider(\"refresh\");\n"), global::GetHandshakeTimeout() );
  #endif
      } else if (strParam.equals("setambientpreset")) {
        int iNew = strValue.toInt();
        global::SetAmbientPreset(iNew);
        strResult = PSTR("Changed AmbientPreset to " + strValue);
        if (iNew == 0 && global::m_pLedHelper->GetLedMode() == LED_MODE_AMBIENT) {
          global::m_pLedHelper->setFullColor(0, global::m_pLedHelper->GetStoredAmbientRed(), global::m_pLedHelper->GetStoredAmbientGreen(), global::m_pLedHelper->GetStoredAmbientBlue());
        }
      } else if (strParam.equals("setambientspeed")) {
        int iNew = strValue.toInt();
        global::SetAmbientSpeed(iNew);
        strResult = PSTR("Changed AmbientSpeed to " + strValue);
      } else if (strParam.equals("setambientintensity")) {
        int iNew = strValue.toInt();
        global::SetAmbientIntensity(iNew);
        strResult = PSTR("Changed AmbientIntensity to " + strValue);
      } else if (strParam.equals("setambienthealthintensity")) {
        int iNew = strValue.toInt();
        global::SetAmbientHealthIntensity(iNew);
        strResult = PSTR("Changed AmbientHealthIntensity to " + strValue);
      } else {
        // Unknown    
        response->printf_P(PSTR("console.log(\"ERROR, unknown request URL %s\");\n"), strParam.c_str());
      }
      
    } else {
      // String to short
      response->printf_P(PSTR("console.log(\"ERROR, invalid value, empty or to short\");\n"));
    }
  } else {
    // String to short
    response->printf_P(PSTR("console.log(\"ERROR, invalid param, empty or to short\");\n"));
  }

  //send the response last
  request->send(response);
}
        

bool isDeviceClaimedForWeb() {
#if defined(ENABLE_WIFI_AP)
  return global::IsDeviceClaimed();
#else
  return false;
#endif
}

bool isAdminRequest(AsyncWebServerRequest *request) {
#if defined(ENABLE_WIFI_AP)
  if (!global::IsDeviceClaimed()) {
    return true;
  }
  if (request->hasParam("claimkey")) {
    return global::ValidateClaimKey(request->getParam("claimkey")->value().c_str());
  }
  if (request->hasParam("claimkey", true)) {
    return global::ValidateClaimKey(request->getParam("claimkey", true)->value().c_str());
  }
  return false;
#else
  return true;
#endif
}

static void sendForbidden(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *r = request->beginResponse(403, APP_JSON, PSTR("{\"success\":\"0\",\"status\":0,\"error\":\"device_claimed\",\"message\":\"Device key required\"}"));
  r->addHeader(PSTR("Access-Control-Allow-Origin"), PSTR("*"));
  request->send(r);
}

bool requireAdmin(AsyncWebServerRequest *request) {
  if (isAdminRequest(request)) {
    return true;
  }
  sendForbidden(request);
  return false;
}

static bool isProtectedParam(const String& strParam) {
  static const char* const aProtected[] = {
    "setdeviceappass", "setdeviceapssid", "setwifiapenable", "setprovisioningenable", "setprovisioningtimeout",
    "setrelaytarget", "setrelaytodirect", "setrelaywifidirect", "setrelaymode",
    "setdebugsyslogserver", "setdebugsyslogport", "setdebugsyslog", "setdebugserial", "setdebugultra", "setwififail",
    "sethuebridgeip", "sethuebridgeuser", "sethuebridgeclientkey"
  };
  for (size_t i = 0; i < sizeof(aProtected) / sizeof(aProtected[0]); i++) {
    if (strParam.equals(aProtected[i])) {
      return true;
    }
  }
  return false;
}

void handleSetParameter(AsyncWebServerRequest *request) {
  String strResult; 
  String strValue;
  String strParam;

  _DBGPRINTF(PSTR("WebServer: handleSetParameter %s \n"), (request->hasParam("param") ? request->getParam("param")->value().c_str() : "NULL") );
  
  if (request->hasParam("param")) {
    strParam = request->getParam("param")->value();
    if (isProtectedParam(strParam) && !requireAdmin(request)) {
      return;
    }
    if (request->hasParam("value")) {
      strValue = request->getParam("value")->value();
  
      if (global::m_pLedHelper != NULL) {
        // Check which parameter to set:
        if (strParam.equals("setdevicename")) {
          // Name will be truncated in set function
          global::SetDeviceName((char*) strValue.c_str());
          strResult = PSTR("Successfully set new value ") + strValue; 
        } else if (strParam.equals("setdeviceappass")) {
          // Name will be truncated in set function
          global::SetDeviceAPpass((char*) strValue.c_str());
          strResult = PSTR("Successfully set new value ") + strValue; 
          global::HandleWifiAPSetup(); // Trigger SOFTAP Change
        } else if (strParam.equals("setdeviceapssid")) {
          // Name will be truncated in set function
          global::SetDeviceAPssid((char*) strValue.c_str());
          strResult = PSTR("Successfully set new value ") + strValue; 
          global::HandleWifiAPSetup(); // Trigger SOFTAP Change
#if defined(ENABLE_RELAY)
        } else if (strParam.equals("setrelaytarget")) {
          // Name will be truncated in set function
          global::SetAmbibackSelectedDevice((char*) strValue.c_str());
          strResult = PSTR("Successfully set new value ") + strValue; 
        } else if (strParam.equals("setrelaytodirect")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if ( iNew == 0 ) {
            global::SetAmbibackRelayModeDirect(0); 
            strResult = PSTR("SetAmbibackRelayModeDirect DISABLED");
          } else if ( iNew == 1 ){
            global::SetAmbibackRelayModeDirect(1); 
            strResult = PSTR("SetAmbibackRelayModeDirect ENABLED");
          }
        } else if (strParam.equals("setrelaywifidirect")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if ( iNew == 0 ) {
            global::SetAmbibackWifiDirectMode(0); 
            strResult = PSTR("SetAmbibackWifiDirectMode DISABLED");
          } else if ( iNew == 1 ){
            global::SetAmbibackWifiDirectMode(1); 
            strResult = PSTR("SetAmbibackWifiDirectMode ENABLED");
          }
// TODO HANDLE RESTART STUFF
        } else if (strParam.equals("setrelaymode")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          global::SetAmbibackRelayMode(iNew); 
          strResult = PSTR("Successfully set new value ") + strValue; 
#endif
#if defined(ENABLE_WIFI_AP)
        } else if (strParam.equals("setwifiapenable")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if ( iNew == 0 ) {
            global::SetWifiAPEnable(0); 
            strResult = PSTR("SetWifiAPEnable DISABLED");
          } else if ( iNew == 1 ){
            global::SetWifiAPEnable(1); 
            strResult = PSTR("SetWifiAPEnable ENABLED");
          }
        } else if (strParam.equals("setprovisioningenable")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt();
          if (iNew == 0) {
            global::SetWifiProvisioningEnabled(0);
            strResult = PSTR("SetWifiProvisioningEnabled DISABLED");
          } else if (iNew == 1) {
            global::SetWifiProvisioningEnabled(1);
            strResult = PSTR("SetWifiProvisioningEnabled ENABLED");
          }
        } else if (strParam.equals("setprovisioningtimeout")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt();
          if (iNew >= WIFI_PROVISIONING_BOOT_WINDOW_MIN_MINUTES &&
              iNew <= WIFI_PROVISIONING_BOOT_WINDOW_MAX_MINUTES) {
            global::SetWifiProvisioningTimeoutMinutes((byte)iNew);
            strResult = PSTR("Successfully set new value ") + strValue;
          } else {
            strResult = PSTR("ERROR, invalid value ")  + strValue;
          }
#endif
        } else if (strParam.equals("setdebugsyslogserver")) {
          // Name will be truncated in set function
          global::SetDebugSyslogServer((char*) strValue.c_str());
          strResult = PSTR("Successfully set new value ") + strValue; 
        } else if (strParam.equals("setdebugsyslogport")) {
          /* Convert to INT and check range */
          int iNewPort = strValue.toInt(); 
          if (  iNewPort >= 1 && 
                iNewPort <= 65536) {
            global::SetDebugSyslogPort(iNewPort); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = strParam + PSTR(" invalid value");
          }
        } else if (strParam.equals("setbuttonmode")) {
          /* Convert to INT and check range */
          int iNewMode = strValue.toInt(); 
          if (  iNewMode >= BTN_MODE_OFF && 
                iNewMode <= BTN_MODE_LAST) {
            global::SetButtonFunction((byte)iNewMode); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = strParam + PSTR(" invalid value");
          }
        } else if (strParam.equals("setwififail")) {
          /* Convert to INT and check range */
          int iNewMode = strValue.toInt(); 
          global::SetWifiFailHandling((byte)iNewMode); 
          strResult = PSTR("Successfully set new value ") + strValue; 
        } else if (strParam.equals("setrestorelastledmode")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt();
          if (iNew == 0) {
            global::SetRestoreLastLedMode(0);
            strResult = PSTR("SetRestoreLastLedMode DISABLED");
          } else if (iNew == 1) {
            global::SetRestoreLastLedMode(1);
            strResult = PSTR("SetRestoreLastLedMode ENABLED");
          }
        } else if (strParam.equals("setframelength")) {
          /* Convert to INT and check range */
          int iNewLength = strValue.toInt(); 
          if (  iNewLength >= DEFAULT_FRAME_LENGTH && 
                iNewLength <= MAX_FRAME_LENGTH && 
                iNewLength % 5 == 0) {
            global::m_pLedHelper->SetFrameLength((byte)iNewLength); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = strParam + PSTR(" invalid value");
          }
        } else if (strParam.equals("setframedelay")) {
          /* Convert to INT and check range */
          int iNewDelay = strValue.toInt(); 
          if (  iNewDelay >= DEFAULT_FRAME_LENGTH && 
                iNewDelay <= MAX_DELAY_LENGTH && 
                iNewDelay % 5 == 0 && 
                iNewDelay >= global::m_pLedHelper->GetFrameLength() ) {
            global::m_pLedHelper->SetDelayLength(iNewDelay); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value ")  + strValue;
          }
        } else if (strParam.equals("setframesmoothing")) {
          /* Convert to INT and check range */
          int iNewVal = strValue.toInt(); 
          if (  iNewVal >= DEFAULT_FRAME_SMOOTHING && 
                iNewVal <= DEFAULT_FRAME_MAX ) {
            global::m_pLedHelper->SetFrameSmoothing(iNewVal); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcolorred")) {
          /* Convert to INT and check range */
          int iNewMinLum = strValue.toInt(); 
          if (  iNewMinLum >= 0 && 
                iNewMinLum <= 255 ) {
            global::m_pLedHelper->SetColorCorRed(iNewMinLum); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcolorgreen")) {
          /* Convert to INT and check range */
          int iNewMinLum = strValue.toInt(); 
          if (  iNewMinLum >= 0 && 
                iNewMinLum <= 255 ) {
            global::m_pLedHelper->SetColorCorGreen(iNewMinLum); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcolorblue")) {
          /* Convert to INT and check range */
          int iNewMinLum = strValue.toInt(); 
          if (  iNewMinLum >= 0 && 
                iNewMinLum <= 255 ) {
            global::m_pLedHelper->SetColorCorBlue(iNewMinLum); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setminimumluminosity")) {
          /* Convert to INT and check range */
          int iNewMinLum = strValue.toInt(); 
          if (  iNewMinLum >= 0 && 
                iNewMinLum <= 255 ) {
            global::m_pLedHelper->SetMinimumLuminosity(iNewMinLum); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setbrightness")) {
          /* Convert to INT and check range */
          int iNewBright = strValue.toInt(); 
          if (  iNewBright >= 0 && 
                iNewBright <= 255 ) {
            global::m_pLedHelper->SetBrightness(iNewBright); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcolorboost")) {
          /* Convert to INT and check range */
          int iNewColorBoost= strValue.toInt(); 
          if (  iNewColorBoost >= 0 && 
                iNewColorBoost <= 255 ) {
            global::m_pLedHelper->SetColorBoost(iNewColorBoost); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcolorboostbalance")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if (  iNew >= 0 && 
                iNew <= 100 ) {
            global::m_pLedHelper->SetColorBoostBalance(iNew); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcolorboostmin")) {
          /* Convert to INT and check range */
          int iNewColorBoost= strValue.toInt(); 
          if (  iNewColorBoost >= 0 && 
                iNewColorBoost <= 255 ) {
            global::m_pLedHelper->SetColorBoostMin(iNewColorBoost); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcolorboostred")) {
          /* Convert to INT and check range */
          int iNewColorBoost= strValue.toInt(); 
          if (  iNewColorBoost >= 0 && 
                iNewColorBoost <= 255 ) {
            global::m_pLedHelper->SetColorBoostRed(iNewColorBoost); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcolorboostgreen")) {
          /* Convert to INT and check range */
          int iNewColorBoost= strValue.toInt(); 
          if (  iNewColorBoost >= 0 && 
                iNewColorBoost <= 255 ) {
            global::m_pLedHelper->SetColorBoostGreen(iNewColorBoost); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcolorboostblue")) {
          /* Convert to INT and check range */
          int iNewColorBoost= strValue.toInt(); 
          if (  iNewColorBoost >= 0 && 
                iNewColorBoost <= 255 ) {
            global::m_pLedHelper->SetColorBoostBlue(iNewColorBoost); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcolorboostthreshold")) {
          /* Convert to INT and check range */
          int iNewColorBoostThres = strValue.toInt(); 
          if (  iNewColorBoostThres >= 0 && 
                iNewColorBoostThres <= 255 ) {
            global::m_pLedHelper->SetColorBoostThreshold(iNewColorBoostThres); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcolorvaluegain")) {
          /* Convert to INT and check range */
          int iNewValueGain = strValue.toInt(); 
          if (  iNewValueGain >= 0 && 
                iNewValueGain <= 500 ) {
            global::m_pLedHelper->SetColorValueGain(iNewValueGain); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setambientpreset")) {
          int iNew = strValue.toInt();
          if (iNew >= 0 && iNew <= 15) {
            global::SetAmbientPreset(iNew);
            strResult = PSTR("Successfully set new value ") + strValue;
            if (iNew == 0 && global::m_pLedHelper->GetLedMode() == LED_MODE_AMBIENT) {
              global::m_pLedHelper->setFullColor(0, global::m_pLedHelper->GetStoredAmbientRed(), global::m_pLedHelper->GetStoredAmbientGreen(), global::m_pLedHelper->GetStoredAmbientBlue());
            }
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setambientspeed")) {
          int iNew = strValue.toInt();
          if (iNew >= 1 && iNew <= 10) {
            global::SetAmbientSpeed(iNew);
            strResult = PSTR("Successfully set new value ") + strValue;
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setambientintensity")) {
          int iNew = strValue.toInt();
          if (iNew >= 1 && iNew <= 100) {
            global::SetAmbientIntensity(iNew);
            strResult = PSTR("Successfully set new value ") + strValue;
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setambienthealthintensity")) {
          int iNew = strValue.toInt();
          if (iNew >= 1 && iNew <= 100) {
            global::SetAmbientHealthIntensity(iNew);
            strResult = PSTR("Successfully set new value ") + strValue;
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setvertical")) {
          /* Convert to INT and check range */
          int iNewValue = strValue.toInt(); 
          if (  iNewValue >= 8 && 
                iNewValue <= 60 ) {
            global::m_pLedHelper->SetSetupVertical(iNewValue); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("sethorizontal")) {
// TODO CHECK MAX LED COUNT!
          /* Convert to INT and check range */
          int iNewValue = strValue.toInt(); 
          if (  iNewValue >= 8 && 
                iNewValue <= 120 ) {
            global::m_pLedHelper->SetSetupHorizontal(iNewValue); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setdirection")) {
          /* Convert to INT and check range */
          int iNewValue = strValue.toInt(); 
          if (  iNewValue >= 1 && 
                iNewValue <= 9 ) {
            global::m_pLedHelper->SetSetupDirection(iNewValue); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setsidetop")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if ( iNew == 0 ) {
            global::m_pLedHelper->SetSetupSideTop(0); 
            strResult = PSTR("Disabled Side Top");
          } else if ( iNew == 1 ){
            global::m_pLedHelper->SetSetupSideTop(1); 
            strResult = PSTR("Enabled Side Top");
          }
        } else if (strParam.equals("setsideleft")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if ( iNew == 0 ) {
            global::m_pLedHelper->SetSetupSideLeft(0); 
            strResult = PSTR("Disabled Side Left");
          } else if ( iNew == 1 ){
            global::m_pLedHelper->SetSetupSideLeft(1); 
            strResult = PSTR("Enabled Side Left");
          }
        } else if (strParam.equals("setsideright")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if ( iNew == 0 ) {
            global::m_pLedHelper->SetSetupSideRight(0); 
            strResult = PSTR("Disabled Side Right");
          } else if ( iNew == 1 ){
            global::m_pLedHelper->SetSetupSideRight(1); 
            strResult = PSTR("Enabled Side Right");
          }
        } else if (strParam.equals("setsidebottom")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if ( iNew == 0 ) {
            global::m_pLedHelper->SetSetupSideBottom(0); 
            strResult = PSTR("Disabled Side Bottom");
          } else if ( iNew == 1 ){
            global::m_pLedHelper->SetSetupSideBottom(1); 
            strResult = PSTR("Enabled Side Bottom");
          }
  
        } else if (strParam.equals("settopbrightness")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if (  iNew >= 0 && 
                iNew <= 100 ) {
            global::m_pLedHelper->SetSideTopBrightness(iNew); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setleftbrightness")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if (  iNew >= 0 && 
                iNew <= 100 ) {
            global::m_pLedHelper->SetSideLeftBrightness(iNew); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setrightbrightness")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if (  iNew >= 0 && 
                iNew <= 100 ) {
            global::m_pLedHelper->SetSideRightBrightness(iNew); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setbottombrightness")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if (  iNew >= 0 && 
                iNew <= 100 ) {
            global::m_pLedHelper->SetSideBottomBrightness(iNew); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
    // settopbrightness
    // setleftbrightness
    // setrightbrightness
    // setbottombrightness
          
        } else if (strParam.equals("setdebugserial")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if ( iNew == 0 ) {
            global::SetSerialEnabled(false); 
            strResult = PSTR("Stopped Serial debugging");
          } else if ( iNew == 1 ){
            // Start Syslog Debugging
            global::SetSerialEnabled(true); 
            strResult = PSTR("Started Serial debugging");
          }
        } else if (strParam.equals("setdebugsyslog")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if ( iNew == 0 ) {
            global::SetSyslogEnabled(false); 
            strResult = PSTR("Stopped Syslog debugging");
          } else if ( iNew == 1 ){
            // Start Syslog Debugging
            global::SetSyslogEnabled(true); 
            strResult = PSTR("Started Syslog debugging");
            // Trigger extra Debug output
            _DBGPRINTF(PSTR("global: debug: DeviceType: '%s', DeviceVersion: '%s'\n"), DEVICE_NAME, AMBIBACK_VERSION);
          }
        } else if (strParam.equals("setdebugultra")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if ( iNew == 0 ) {
            // Stop Ultra Debugging
            global::m_bUltraDebugEnabled = false; 
            strResult = PSTR("Stopped ultra debugging");
          } else if ( iNew == 1 ){
            // Start Ultra Debugging
            global::m_bUltraDebugEnabled = true; 
            strResult = PSTR("Started ultra debugging");
  #if defined(ESP32)
            // Trigger extra Debug output
            _DBGPRINTF(PSTR("global: debug: DeviceID: '%s/%s', DeviceName: '%s'\n"), global::GetDeviceID(), global::GetDeviceIDMac(), global::GetDeviceName());
            _DBGPRINTF(PSTR("global: debug: ESP32 EfuseMac: %#016x, FlashChipSize: %d, FreeSketchSpace: %d, TCP_MSS: %d\n"), 
              ESP.getEfuseMac(),
              ESP.getFlashChipSize(),
              ESP.getFreeSketchSpace(),
              TCP_MSS
            );
            int8_t max_tx_power = 0;
            esp_wifi_get_max_tx_power(&max_tx_power);
            _DBGPRINTF(PSTR("global: debug: ESP32 Wifi: Signal Strength: %ddbm, MaxPower: %ddbm\n"),
              WiFi.RSSI(),
              max_tx_power
            );
            global::printEspPartitions();
  #endif
          }
  #if defined(ENABLE_CAM)
        } else if (strParam.equals("setcamperformance")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          // Set new Cam Performance
          global::SetCamPerformance(iNew); 
          strResult = PSTR("Changed resolution to " + strValue);
        } else if (strParam.equals("setcamres")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if (  iNew >= RESOLUTION_MINIDX &&
                iNew <= RESOLUTION_MAXIDX 
              ) {
            // Set new Cam resolution
            global::SetCamResolution(iNew); 
            strResult = PSTR("Changed resolution to " + strValue);
          } else {
            strResult = PSTR("ERROR, invalid value " + strValue);
          }
        } else if (strParam.equals("setcamfreq")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if (  iNew >= CAMFREQ_MINIDX &&
                iNew <= CAMFREQ_MAXIDX
              ) {
            // Set new Cam resolution
            global::SetCamFreq(iNew); 
            strResult = PSTR("Changed Cam frequence to " + strValue);
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcambufcount")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if (  iNew > 0 &&
                iNew < (psramFound() ? CAM_BUFFER_COUNT_MAX : CAM_BUFFER_COUNT)
              ) {
            // Set new Cam resolution
            global::SetCamFrameBuffer(iNew); 
            strResult = PSTR("Changed Cam Frame Buffer Count to " + strValue);
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcamflicker")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if (  iNew >= CAMFLICKER_MINIDX &&
                iNew <= CAMFLICKER_MAXIDX
              ) {
            // Set new Cam resolution
            global::SetCamFlicker(iNew); 
            strResult = PSTR("Changed Cam Flicker to " + strValue);
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
  # if CAM_SENSOR == CAM_SENSOR_OV3660
        } else if (strParam.equals("setcamoverclock")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          global::SetCamOverclock(iNew); 
          strResult = PSTR("Changed Cam Overclockcker to " + strValue);
  #endif
        } else if (strParam.equals("setcamcolorred")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if (  iNew >= 0 && 
                iNew <= 255 ) {
            global::SetCamColorCorRed(iNew); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcamcolorgreen")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if (  iNew >= 0 && 
                iNew <= 255 ) {
            global::SetCamColorCorGreen(iNew); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcamcolorblue")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if (  iNew >= 0 && 
                iNew <= 255 ) {
            global::SetCamColorCorBlue(iNew); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcamactivationthreshold")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if (  iNew >= 0 && 
                iNew <= 128 ) {
            global::SetCamActivationThreshold(iNew); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcamblackthresholdsmooth")) {
          /* Convert to INT, check is done LEDHelper */
          int iNew = strValue.toInt(); 
          global::m_pCamProcessor->SetCamBlackThresholdSmoothing((byte)iNew); 
          strResult = PSTR("Changed CamBlackThresholdSmoothing to " + strValue);
        } else if (strParam.equals("setcamblackremovethreshold")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if (  iNew >= 0 && 
                iNew <= 128 ) {
            global::SetCamBlackRemoveThreshold(iNew); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcamminpixelamount")) {
          /* Convert to INT and check range */
          int iNew = strValue.toInt(); 
          if (  iNew >= 0 && 
                iNew <= 100 ) {
            global::SetCamMinPixelAmount(iNew); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("setcamautoonoff")) {
          /* Convert to INT, check is done global */
          int iNew = strValue.toInt(); 
          global::SetCamAutoOnOff((byte)iNew); 
          strResult = PSTR("Changed CamAutoOnOff to " + strValue);
        } else if (strParam.equals("setcamautoonoffdelay")) {
          /* Convert to INT, check is done global */
          int iNew = strValue.toInt(); 
          global::SetCamAutoOnOffDelay((byte)iNew); 
          strResult = PSTR("Changed CamAutoOnOffDelay to " + strValue);
        } else if (strParam.equals("setcamcontrast")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamContrast(iNew); 
          strResult = PSTR("Changed contrast to " + strValue);
        } else if (strParam.equals("setcambrightness")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamBrightness(iNew); 
          strResult = PSTR("Changed Brightness to " + strValue);
        } else if (strParam.equals("setcamsaturation")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamSaturation(iNew); 
          strResult = PSTR("Changed Saturation to " + strValue);
        } else if (strParam.equals("setcamspecialeffect")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamSpecialEffect(iNew); 
          strResult = PSTR("Changed SpecialEffect to " + strValue);
        } else if (strParam.equals("setcamwbmode")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamWbMode(iNew); 
          strResult = PSTR("Changed WbMode to " + strValue);
        } else if (strParam.equals("setcamaelevels")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamAeLevels(iNew); 
          strResult = PSTR("Changed AeLevels to " + strValue);
        } else if (strParam.equals("setcamagcgain")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamAgcGain(iNew); 
          strResult = PSTR("Changed AgcGain to " + strValue);
        } else if (strParam.equals("setcamawbgain")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamAwbGainControl(iNew); 
          strResult = PSTR("Changed AwbGainControl to " + strValue);
        } else if (strParam.equals("setcamagainceilingsensor")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamGainceilingSensor(iNew); 
          strResult = PSTR("Changed GainceilingSensor to " + strValue);
        } else if (strParam.equals("setcamagaincontrol")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamGaincontrol(iNew); 
          strResult = PSTR("Changed Gaincontrol to " + strValue);
        } else if (strParam.equals("setcamawhitebalance")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamWhitebalance(iNew); 
          strResult = PSTR("Changed Whitebalance to " + strValue);
        } else if (strParam.equals("setcamcolorbar")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamColorbar(iNew); 
          strResult = PSTR("Changed Colorbar to " + strValue);
        } else if (strParam.equals("setcamquality")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamQuality(iNew); 
          strResult = PSTR("Changed CamQuality to " + strValue);
        } else if (strParam.equals("setcamaec2")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamAec2(iNew); 
          strResult = PSTR("Changed CamAec2 to " + strValue);
        } else if (strParam.equals("setcamadptexp")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamAdaptiveExposurecontrol(iNew); 
          strResult = PSTR("Changed CamAdaptiveExposurecontrol to " + strValue);
        } else if (strParam.equals("setcamaecminvalue")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamAecMinValue(iNew); 
          strResult = PSTR("Changed CamAecMinValue to " + strValue);
        } else if (strParam.equals("setcamaecmaxvalue")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamAecMaxValue(iNew); 
          strResult = PSTR("Changed CamAecMaxValue to " + strValue);
        } else if (strParam.equals("setcamwhiteactthreshold")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamWhiteActivationTreshold((byte) iNew); 
          strResult = PSTR("Changed CamWhiteActivationTreshold to " + strValue);
        } else if (strParam.equals("setcambeforewhiteactthreshold")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamBeforeWhiteActivationTreshold((byte) iNew); 
          strResult = PSTR("Changed CamBeforeWhiteActivationTreshold to " + strValue);
        } else if (strParam.equals("setcamwhiteminimumamount")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamWhiteMinimumAmount((byte) iNew); 
          strResult = PSTR("Changed CamWhiteMinimumAmount to " + strValue);
        } else if (strParam.equals("setcamadaptivechangepercent")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt();
          global::SetCamAdaptiveChangePercent(iNew); 
          strResult = PSTR("Changed CamAdaptiveChangePercent to " + strValue);
        } else if (strParam.equals("setcamadaptivechangedelay")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamAdaptiveChangeDelay(iNew); 
          strResult = PSTR("Changed CamAdaptiveChangeDelay to " + strValue);
        } else if (strParam.equals("setcamadptgain")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamAdaptiveGaincontrol(iNew); 
          strResult = PSTR("Changed CamAdaptiveGaincontrol to " + strValue);
        } else if (strParam.equals("setcamadaptivegaindown")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamAdaptiveGainDown(iNew); 
          strResult = PSTR("Changed CamAdaptiveGainDown to " + strValue);
        } else if (strParam.equals("setcamadaptivegainup")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamAdaptiveGainUp(iNew); 
          strResult = PSTR("Changed CamAdaptiveGainUp to " + strValue);
        } else if (strParam.equals("setcamadaptivegainmax")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamAdaptiveGainMax(iNew); 
          strResult = PSTR("Changed CamAdaptiveGainMax to " + strValue);
        } else if (strParam.equals("setcamaecvalue")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamAecValue(iNew); 
          strResult = PSTR("Changed CamAecValue to " + strValue);
        } else if (strParam.equals("setcamhmirror")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamHMirror(iNew); 
          strResult = PSTR("Changed CamHMirror to " + strValue);
        } else if (strParam.equals("setcamvflip")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamVFlip(iNew); 
          strResult = PSTR("Changed CamVFlip to " + strValue);
        } else if (strParam.equals("setcambpc")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamBPC(iNew); 
          strResult = PSTR("Changed CamBPC to " + strValue);
        } else if (strParam.equals("setcamwpc")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamWPC(iNew); 
          strResult = PSTR("Changed CamWPC to " + strValue);
        } else if (strParam.equals("setcamdenoise")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamDenoise(iNew); 
          strResult = PSTR("Changed CamDenoise to " + strValue);
        } else if (strParam.equals("setcamlenc")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamLenc(iNew); 
          strResult = PSTR("Changed CamLenc to " + strValue);
        } else if (strParam.equals("setcamrawgma")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamRawGMA(iNew); 
          strResult = PSTR("Changed CamRawGMA to " + strValue);
        } else if (strParam.equals("setcamaec")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamAutoExposurecontrol(iNew); 
          strResult = PSTR("Changed CamAutoExposurecontrol to " + strValue);
        } else if (strParam.equals("setcamdcw")) {
          /* Convert to INT, check is done in esp32cam*/
          int iNew = strValue.toInt(); 
          global::SetCamDcw(iNew); 
          strResult = PSTR("Changed CamDcw to " + strValue);
        } else if (strParam.equals("setcamcolorboostmax")) {
          /* Convert to INT, check is done LEDHelper  */
          int iNew = strValue.toInt(); 
          global::m_pCamProcessor->SetColorBoostMaxPercent(iNew); 
          strResult = PSTR("Changed ColorBoostMaxPercent to " + strValue);
        } else if (strParam.equals("setcammode")) {
          /* Convert to INT, check is done LEDHelper */
          int iNew = strValue.toInt(); 
          global::m_pCamProcessor->SetCamMode((byte)iNew); 
          strResult = PSTR("Changed CamMode to " + strValue);
        } else if (strParam.equals("setcamdetaillevel")) {
          /* Convert to INT, check is done LEDHelper */
          int iNew = strValue.toInt(); 
          global::m_pCamProcessor->SetCamDetailLevel((byte)iNew); 
          strResult = PSTR("Changed CamDetailLevel to " + strValue);
        } else if (strParam.equals("setcamdetailsmooth")) {
          /* Convert to INT, check is done LEDHelper */
          int iNew = strValue.toInt(); 
          global::m_pCamProcessor->SetCamDetailSmooth((byte)iNew); 
          strResult = PSTR("Changed CamDetailSmoothing to " + strValue);
        } else if (strParam.equals("setcamsmoothenable")) {
          /* Convert to INT, check is done LEDHelper */
          int iNew = strValue.toInt();
          global::m_pCamProcessor->SetCamSmoothEnable((byte)iNew);
          strResult = PSTR("Changed CamSmoothEnable to " + strValue);
        } else if (strParam.equals("setcamsmoothmaxframes")) {
          /* Convert to INT, check is done LEDHelper */
          int iNew = strValue.toInt();
          global::m_pCamProcessor->SetCamSmoothMaxFrames((byte)iNew);
          strResult = PSTR("Changed CamSmoothMaxFrames to " + strValue);
        } else if (strParam.equals("setcamsmoothmaxdiff")) {
          /* Convert to INT, check is done LEDHelper */
          int iNew = strValue.toInt();
          global::m_pCamProcessor->SetCamSmoothMaxDiff((byte)iNew);
          strResult = PSTR("Changed CamSmoothMaxDiff to " + strValue);
        } else if (strParam.equals("setcamsmooth")) {
          /* Convert to INT, check is done LEDHelper */
          int iNew = strValue.toInt();
          global::m_pCamProcessor->SetCamSmooth((byte)iNew);
          strResult = PSTR("Changed CamSmooth Mode to " + strValue);
        } else if (strParam.equals("setcaminnerhori")) {
          /* Convert to FLOAT, check is done LEDHelper */
          float flNew = (float) strValue.toFloat() / 100;
          global::m_pCamProcessor->SetCamInnerHori(flNew);
          strResult = PSTR("Changed CamInnerHori to " + strValue);
        } else if (strParam.equals("setcaminnervert")) {
          /* Convert to FLOAT, check is done LEDHelper */
          float flNew = (float) strValue.toFloat() / 100;
          global::m_pCamProcessor->SetCamInnerVert(flNew);
          strResult = PSTR("Changed CamInnerVert to " + strValue);
        } else if (strParam.equals("setcamhighquality")) {
          /* Convert to INT, check is done LEDHelper */
          int iNew = strValue.toInt(); 
           global::SetCamHighQuality((byte)iNew);
          strResult = PSTR("Changed CamHighQuality to " + strValue);
  #endif
  #if defined(ENABLE_HUE)
        } else if (strParam.equals("sethuebridgeip")) {
          global::SetHueBridgeIP((char*) strValue.c_str());
          strResult = PSTR("Successfully set new value ") + strValue; 
        } else if (strParam.equals("sethuebridgeuser")) {
          global::SetHueBridgeUser((char*) strValue.c_str());
          strResult = PSTR("Successfully set new value ") + strValue;
        } else if (strParam.equals("sethuebridgeclientkey")) {
          global::SetHueBridgeClientkey((char*) strValue.c_str());
          strResult = PSTR("Successfully set new value ") + strValue;
        } else if (strParam.equals("sethuesupport")) {
          /* Convert to INT */
          int iNew = strValue.toInt(); 
          global::SetHueSupport(iNew); 
          strResult = PSTR("Changed HueSupport to " + strValue);
        } else if (strParam.equals("sethueframelength")) {
          /* Convert to INT */
          int iNew = strValue.toInt(); 
          if (  iNew >= 20 && 
                iNew <= 100 ) {
            global::SetFrameLengthDtls(iNew); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("sethuehandshaketimeout")) {
          /* Convert to INT */
          int iNew = strValue.toInt(); 
          if (  iNew >= 5 && 
                iNew <= 60 ) {
            global::SetHandshakeTimeout(iNew); 
            strResult = PSTR("Successfully set new value ") + strValue; 
          } else {
            strResult = PSTR("ERROR, invalid value");
          }
        } else if (strParam.equals("sethuelightenable")) {
          String strHueID;
          if (request->hasParam("id")) {
            strHueID = request->getParam("id")->value();
            /* Convert to INT */
            int iNew = strValue.toInt(); 
            global::SetHueLightState((char*) strHueID.c_str(), iNew); 
            strResult = PSTR("Changed Hue Light State for " + strHueID + " to " + strValue);
          } else {
            strResult = PSTR("ERROR, ID missing.");
          }
        } else if (strParam.equals("sethuelightzone")) {
          String strHueID;
          String strHueZone;
          if (request->hasParam("id") && request->hasParam("zone")) {
            strHueID = request->getParam("id")->value();
            strHueZone = request->getParam("zone")->value();
            /* Convert to INT */
            int iNew = strValue.toInt(); 
            int iZone = strHueZone.toInt(); 
            global::SetHueLightZoneState((char*) strHueID.c_str(), iZone, iNew); 
            strResult = PSTR("Changed Hue Light Zone " + strHueZone + " State for " + strHueID + " to " + strValue);
          } else {
            strResult = PSTR("ERROR, ID/Zone missing.");
          }
        } else if (strParam.equals("sethuelightcolcorred")) {
          String strHueID;
          if (request->hasParam("id")) {
            strHueID = request->getParam("id")->value();
            /* Convert to INT */
            int iNew = strValue.toInt(); 
            global::SetHueLightColCorRed((char*) strHueID.c_str(), iNew); 
            strResult = PSTR("Changed Hue Light colcorred for " + strHueID + " to " + strValue);
          } else {
            strResult = PSTR("ERROR, ID missing.");
          }
        } else if (strParam.equals("sethuelightcolcorgreen")) {
          String strHueID;
          if (request->hasParam("id")) {
            strHueID = request->getParam("id")->value();
            /* Convert to INT */
            int iNew = strValue.toInt(); 
            global::SetHueLightColCorGreen((char*) strHueID.c_str(), iNew); 
            strResult = PSTR("Changed Hue Light colcorgreen for " + strHueID + " to " + strValue);
          } else {
            strResult = PSTR("ERROR, ID missing.");
          }
        } else if (strParam.equals("sethuelightcolcorblue")) {
          String strHueID;
          if (request->hasParam("id")) {
            strHueID = request->getParam("id")->value();
            /* Convert to INT */
            int iNew = strValue.toInt(); 
            global::SetHueLightColCorBlue((char*) strHueID.c_str(), iNew); 
            strResult = PSTR("Changed Hue Light colcorblue for " + strHueID + " to " + strValue);
          } else {
            strResult = PSTR("ERROR, ID missing.");
          }
        } else if (strParam.equals("sethuelightcolbrightness")) {
          String strHueID;
          if (request->hasParam("id")) {
            strHueID = request->getParam("id")->value();
            /* Convert to INT */
            int iNew = strValue.toInt(); 
            global::SetHueLightColBrightness((char*) strHueID.c_str(), iNew); 
            strResult = PSTR("Changed Hue Light Brightness for " + strHueID + " to " + strValue);
          } else {
            strResult = PSTR("ERROR, ID missing.");
          }
  #endif
  #if defined(ENABLE_DREAMSCREEN)
        } else if (strParam.equals("setdreamscreensupport")) {
          /* Convert to INT */
          int iNew = strValue.toInt(); 
          global::SetDreamScreenSupport(iNew); 
          strResult = PSTR("Changed DreamScreenSupport to " + strValue);
        } else if (strParam.equals("setdreamscreenemulation")) {
          /* Convert to INT */
          int iNew = strValue.toInt(); 
          global::SetDreamScreenEmulation(iNew); 
          strResult = PSTR("Changed DreamScreenEmulation to " + strValue);
        } else if (strParam.equals("setdreamscreengroup")) {
          /* Convert to INT */
          int iNew = strValue.toInt(); 
          global::SetDreamScreenGroup(iNew); 
          strResult = PSTR("Changed DreamScreenGroup to " + strValue);
        } else if (strParam.equals("setdreamscreencolorsmoothing")) {
          /* Convert to INT */
          int iNew = strValue.toInt(); 
          global::SetDreamScreenLowColorSmoothing(iNew); 
          strResult = PSTR("Changed DreamScreenLowColorSmoothing to " + strValue);
  #endif
        } else {
          strResult = PSTR("ERROR, unknown Parameter.");
        }
      } else {
        strResult = PSTR("INTERNAL ERROR, LED Handler not set.");
      }
    } else {
      // String to short
      strResult = PSTR("ERROR, invalid value, empty or to short");
    }
  } else {
    // String to short
    strResult = PSTR("ERROR, invalid param, empty or to short");
  }
  
  _DBGPRINTF(PSTR("WebServer: handleSetParameter: %s\n"), strResult.c_str());
  request->send(200, TEXT_PLAIN, request->url() + ": " + strResult);
}
#define ADD_CORS_HEADER(r) do { (r)->addHeader(PSTR("Access-Control-Allow-Origin"), PSTR("*")); } while(0)

void handleGetParametersAsJson(AsyncWebServerRequest *request){
  _DBGPRINTF(PSTR("WebServer: requested %s \n"), request->url().c_str());
  // Start response
  AsyncResponseStream *response = request->beginResponseStream(APP_JSON);
  ADD_CORS_HEADER(response);
  response->printf_P(PSTR("{\n")); 
  response->printf_P(PSTR("\t\"devicetype\": \"%s\",\n"), AMBIBACK_NAME); 
  response->printf_P(PSTR("\t\"deviceversion\": \"%s\",\n"), AMBIBACK_VERSION);
  response->printf_P(PSTR("\t\"coreversion\": \"%s\",\n"), AMBIBACK_CORE_VERSION);
  response->printf_P(PSTR("\t\"idfversion\": \"%s\",\n"), esp_get_idf_version());
  response->printf_P(PSTR("\t\"buildid\": \"%s\",\n"), AMBIBACK_BUILD_ID);
  response->printf_P(PSTR("\t\"firmwarebaseurl\": \"%s\",\n"), HTTP_BASE_URL); 
  response->printf_P(PSTR("\t\"devicename\": \"%s\",\n"), global::GetDeviceName()); 
  response->printf_P(PSTR("\t\"deviceid\": \"%s\",\n"), global::GetDeviceID());
  response->printf_P(PSTR("\t\"devicemac\": \"%s\",\n"), global::GetDeviceIDMac());
  response->printf_P(PSTR("\t\"deviceip\": \"%s\",\n"), WiFi.localIP().toString().c_str()); 
#if defined(ENABLE_WIFI_AP)
  response->printf_P(PSTR("\t\"provisioningactive\": \"%d\",\n"), global::IsWifiProvisioningActive());
  response->printf_P(PSTR("\t\"provisioningpending\": \"%d\",\n"), global::IsWifiProvisioningPending());
  response->printf_P(PSTR("\t\"provisioningenabled\": \"%d\",\n"), global::GetWifiProvisioningEnabled());
  response->printf_P(PSTR("\t\"provisioningtimeoutminutes\": \"%d\",\n"), global::GetWifiProvisioningTimeoutMinutes());
  response->printf_P(PSTR("\t\"provisioningstarted\": \"%lu\",\n"), global::GetWifiProvisioningStarted());
  response->printf_P(PSTR("\t\"provisioningexpiresin\": \"%lu\",\n"), global::GetWifiProvisioningExpiresIn());
  response->printf_P(PSTR("\t\"provisioningapsecurity\": \"%s\",\n"), "wpa2");
  response->printf_P(PSTR("\t\"deviceapenabled\": \"%d\",\n"), global::GetWifiAPEnable());
  response->printf_P(PSTR("\t\"deviceapssid\": \"%s\",\n"), global::GetDeviceAPssid());
  response->printf_P(PSTR("\t\"deviceappass\": \"%s\",\n"), (isAdminRequest(request) ? global::GetDeviceAPpass() : ""));
#endif
  response->printf_P(PSTR("\t\"buttonmode\": \"%d\",\n"), global::GetButtonFunction());
  response->printf_P(PSTR("\t\"wififailhandling\": \"%d\",\n"), global::GetWifiFailHandling());
  response->printf_P(PSTR("\t\"debugsyslogserver\": \"%s\",\n"), global::GetDebugSyslogServer());
  response->printf_P(PSTR("\t\"debugsyslogport\": \"%d\",\n"), global::GetDebugSyslogPort());
  response->printf_P(PSTR("\t\"debugserial\": \"%d\",\n"), global::GetSerialEnabled());
  response->printf_P(PSTR("\t\"debugsyslog\": \"%d\",\n"), global::GetSyslogEnabled());
  response->printf_P(PSTR("\t\"debugultra\": \"%d\",\n"), global::GetUltraDebugEnabled());
  response->printf_P(PSTR("\t\"vertical\": \"%d\",\n"), global::m_pLedHelper->GetSetupVertical()); 
  response->printf_P(PSTR("\t\"horizontal\": \"%d\",\n"), global::m_pLedHelper->GetSetupHorizontal()); 
  response->printf_P(PSTR("\t\"direction\": \"%d\",\n"), global::m_pLedHelper->GetSetupDirection()); 
  response->printf_P(PSTR("\t\"sidetop\": \"%d\",\n"), global::m_pLedHelper->GetSetupSideTop()); 
  response->printf_P(PSTR("\t\"sideleft\": \"%d\",\n"), global::m_pLedHelper->GetSetupSideLeft()); 
  response->printf_P(PSTR("\t\"sideright\": \"%d\",\n"), global::m_pLedHelper->GetSetupSideRight()); 
  response->printf_P(PSTR("\t\"sidebottom\": \"%d\",\n"), global::m_pLedHelper->GetSetupSideBottom()); 
  response->printf_P(PSTR("\t\"sidetopbrightness\": \"%d\",\n"), global::m_pLedHelper->GetSideTopBrightness()); 
  response->printf_P(PSTR("\t\"sideleftbrightness\": \"%d\",\n"), global::m_pLedHelper->GetSideLeftBrightness()); 
  response->printf_P(PSTR("\t\"siderightbrightness\": \"%d\",\n"), global::m_pLedHelper->GetSideRightBrightness()); 
  response->printf_P(PSTR("\t\"sidebottombrightness\": \"%d\",\n"), global::m_pLedHelper->GetSideBottomBrightness()); 
  response->printf_P(PSTR("\t\"framelength\": \"%d\",\n"), global::m_pLedHelper->GetFrameLength()); 
  response->printf_P(PSTR("\t\"framedelay\": \"%d\",\n"), global::m_pLedHelper->GetDelayLength());
  response->printf_P(PSTR("\t\"framesmoothing\": \"%d\",\n"), global::m_pLedHelper->GetFrameSmoothing());
  response->printf_P(PSTR("\t\"motionpreset\": \"%d\",\n"), global::GetMotionPreset()); 
  response->printf_P(PSTR("\t\"colorred\": \"%d\",\n"), global::m_pLedHelper->GetColorCorRed()); 
  response->printf_P(PSTR("\t\"colorgreen\": \"%d\",\n"), global::m_pLedHelper->GetColorCorGreen()); 
  response->printf_P(PSTR("\t\"colorblue\": \"%d\",\n"), global::m_pLedHelper->GetColorCorBlue()); 
  response->printf_P(PSTR("\t\"colorpreset\": \"%d\",\n"), global::GetColorPreset()); 
  response->printf_P(PSTR("\t\"colorboost\": \"%d\",\n"), global::m_pLedHelper->GetColorBoost());
  response->printf_P(PSTR("\t\"colorboostbalance\": \"%d\",\n"), global::m_pLedHelper->GetColorBoostBalance());
  response->printf_P(PSTR("\t\"colorboostmin\": \"%d\",\n"), global::m_pLedHelper->GetColorBoostMin());
  response->printf_P(PSTR("\t\"colorboostred\": \"%d\",\n"), global::m_pLedHelper->GetColorBoostRed());
  response->printf_P(PSTR("\t\"colorboostgreen\": \"%d\",\n"), global::m_pLedHelper->GetColorBoostGreen());
  response->printf_P(PSTR("\t\"colorboostblue\": \"%d\",\n"), global::m_pLedHelper->GetColorBoostBlue());
  response->printf_P(PSTR("\t\"colorboostthreshold\": \"%d\",\n"), global::m_pLedHelper->GetColorBoostThreshold());
  response->printf_P(PSTR("\t\"brightnesspreset\": \"%d\",\n"), global::GetBrightnessPreset());
  response->printf_P(PSTR("\t\"colorvaluegain\": \"%d\",\n"), global::m_pLedHelper->GetColorValueGain());
  response->printf_P(PSTR("\t\"brightness\": \"%d\",\n"), global::m_pLedHelper->GetBrightness());
  response->printf_P(PSTR("\t\"minimumluminosity\": \"%d\",\n"), global::m_pLedHelper->GetMinimumLuminosity()); 
  response->printf_P(PSTR("\t\"restorelastledmode\": \"%d\",\n"), global::GetRestoreLastLedMode());
  response->printf_P(PSTR("\t\"ledmode\": \"%d\",\n"), global::m_pLedHelper->GetLedMode()); 
  response->printf_P(PSTR("\t\"ambientpreset\": \"%d\",\n"), global::GetAmbientPreset());
  response->printf_P(PSTR("\t\"ambientspeed\": \"%d\",\n"), global::GetAmbientSpeed());
  response->printf_P(PSTR("\t\"ambientintensity\": \"%d\",\n"), global::GetAmbientIntensity());
  response->printf_P(PSTR("\t\"ambienthealthintensity\": \"%d\",\n"), global::GetAmbientHealthIntensity());
  
#if defined(ENABLE_CAM)
  response->printf_P(PSTR("\t\"camerapoint\":{\n"));
  for(int iIndex=0; iIndex < 4; iIndex++) {
    byte iPoint = (byte)(iIndex + 1); /* Firmware points 1-4 (TL,TR,BL,BR) */
    response->printf_P(PSTR("\t\t\"%d\":{\n"), iIndex);
    response->printf_P(PSTR("\t\t\t\"x\":\"%d\",\n"), global::m_pCamProcessor->getCameraPointX(iPoint));
    response->printf_P(PSTR("\t\t\t\"y\":\"%d\"\n"), global::m_pCamProcessor->getCameraPointY(iPoint));
    response->printf_P(PSTR("\t\t}"));
    if (iIndex < 3) /* only add , if other element follows */
      response->printf_P(PSTR(",\n"));
  }
  response->printf_P(PSTR("\t},\n"));
  response->printf_P(PSTR("\t \"cammode\": \"%d\",\n"), global::m_pCamProcessor->GetCamMode()); 
  response->printf_P(PSTR("\t \"camdetaillevel\": \"%d\",\n"), global::m_pCamProcessor->GetCamDetailLevel());
  response->printf_P(PSTR("\t \"camsmoothmode\": \"%d\",\n"), global::m_pCamProcessor->GetCamSmooth()); 
  response->printf_P(PSTR("\t \"camsmoothenable\": \"%d\",\n"), global::m_pCamProcessor->GetCamSmoothEnable()); 
  response->printf_P(PSTR("\t \"camsmoothmaxframes\": \"%d\",\n"), global::m_pCamProcessor->GetCamSmoothMaxFrames()); 
  response->printf_P(PSTR("\t \"camsmoothmaxdiff\": \"%d\",\n"), global::m_pCamProcessor->GetCamSmoothMaxDiff()); 
  response->printf_P(PSTR("\t \"caminnerhori\": \"%f\",\n"), global::m_pCamProcessor->GetCamInnerHori()); 
  response->printf_P(PSTR("\t \"caminnervert\": \"%f\",\n"), global::m_pCamProcessor->GetCamInnerVert()); 
  response->printf_P(PSTR("\t \"camperformance\": \"%d\",\n"), global::GetCamPerformance()); 
  response->printf_P(PSTR("\t \"camresolution\": \"%d\",\n"), global::GetCamResolution()); 
  response->printf_P(PSTR("\t \"camfreq\": \"%d\",\n"), global::GetCamFreq());
  response->printf_P(PSTR("\t \"camfbcount\": \"%d\",\n"), global::GetCamFrameBuffer());
  response->printf_P(PSTR("\t \"camflicker\": \"%d\",\n"), global::GetCamFlicker());
# if CAM_SENSOR == CAM_SENSOR_OV3660
  response->printf_P(PSTR("\t \"camoverclock\": \"%d\",\n"), global::GetCamOverclock());
#endif  
  response->printf_P(PSTR("\t \"campreset\": \"%d\",\n"), global::GetCamPreset());
  response->printf_P(PSTR("\t \"camexposurepreset\": \"%d\",\n"), global::GetExposurePreset());
  response->printf_P(PSTR("\t \"caminnerpreset\": \"%d\",\n"), global::GetCamInnerPreset());
  response->printf_P(PSTR("\t \"camcolorcorred\": \"%d\",\n"), global::GetCamColorCorRed()); 
  response->printf_P(PSTR("\t \"camcolorcorgreen\": \"%d\",\n"), global::GetCamColorCorGreen()); 
  response->printf_P(PSTR("\t \"camcolorcorblue\": \"%d\",\n"), global::GetCamColorCorBlue()); 
  response->printf_P(PSTR("\t \"camcoloractivationthreshold\": \"%d\",\n"), global::GetCamActivationThreshold()); 
  response->printf_P(PSTR("\t \"camcolorblackthresholdsmoothing\": \"%d\",\n"), global::m_pCamProcessor->GetCamBlackThresholdSmoothing()); 
  response->printf_P(PSTR("\t \"camcolorblackremovethreshold\": \"%d\",\n"), global::GetCamBlackRemoveThreshold());
  response->printf_P(PSTR("\t \"camminpixelamount\": \"%d\",\n"), global::GetCamMinPixelAmount());
  response->printf_P(PSTR("\t \"camcontrast\": \"%d\",\n"), global::GetCamContrast()); 
  response->printf_P(PSTR("\t \"cambrightness\": \"%d\",\n"), global::GetCamBrightness()); 
  response->printf_P(PSTR("\t \"camsaturation\": \"%d\",\n"), global::GetCamSaturation()); 
  response->printf_P(PSTR("\t \"camspecialeffect\": \"%d\",\n"), global::GetCamSpecialEffect()); 
  response->printf_P(PSTR("\t \"camwbmode\": \"%d\",\n"), global::GetCamWbMode()); 
  response->printf_P(PSTR("\t \"camquality\": \"%d\",\n"), global::GetCamQuality()); 
  response->printf_P(PSTR("\t \"camawbgaincontrol\": \"%d\",\n"), global::GetCamAwbGainControl()); 
  response->printf_P(PSTR("\t \"camawhitebalance\": \"%d\",\n"), global::GetCamWhitebalance()); 
  response->printf_P(PSTR("\t \"camhmirror\": \"%d\",\n"), global::GetCamHMirror()); 
  response->printf_P(PSTR("\t \"camvflip\": \"%d\",\n"), global::GetCamVFlip());
  response->printf_P(PSTR("\t \"camhighquality\": \"%d\",\n"), global::GetCamHighQuality());
  response->printf_P(PSTR("\t \"cambpc\": \"%d\",\n"), global::GetCamBPC());
  response->printf_P(PSTR("\t \"camwpc\": \"%d\",\n"), global::GetCamWPC());
  response->printf_P(PSTR("\t \"camdenoise\": \"%d\",\n"), global::GetCamDenoise());
  response->printf_P(PSTR("\t \"camlenc\": \"%d\",\n"), global::GetCamLenc());
  response->printf_P(PSTR("\t \"camrawgma\": \"%d\",\n"), global::GetCamRawGMA());
  response->printf_P(PSTR("\t \"camaec\": \"%d\",\n"), global::GetCamAutoExposurecontrol());
  response->printf_P(PSTR("\t \"camaec2\": \"%d\",\n"), global::GetCamAec2());
  response->printf_P(PSTR("\t \"camaelevels\": \"%d\",\n"), global::GetCamAeLevels());
  response->printf_P(PSTR("\t \"camaecvalue\": \"%d\",\n"), global::GetCamAecValue());
  response->printf_P(PSTR("\t \"camadptexp\": \"%d\",\n"), global::GetCamAdaptiveExposurecontrol());
  response->printf_P(PSTR("\t \"camaecminvalue\": \"%d\",\n"), global::GetCamAecMinValue());
  response->printf_P(PSTR("\t \"camaecmaxvalue\": \"%d\",\n"), global::GetCamAecMaxValue());
  response->printf_P(PSTR("\t \"camwhiteactthreshold\": \"%d\",\n"), global::GetCamWhiteActivationTreshold());
  response->printf_P(PSTR("\t \"camwhitebeforeactthreshold\": \"%d\",\n"), global::GetCamBeforeWhiteActivationTreshold());
  response->printf_P(PSTR("\t \"camwhiteminamount\": \"%d\",\n"), global::GetCamWhiteMinimumAmount());
  response->printf_P(PSTR("\t \"camadaptivechangepercent\": \"%d\",\n"), global::GetCamAdaptiveChangePercent());
  response->printf_P(PSTR("\t \"camadaptivechangedelay\": \"%d\",\n"), global::GetCamAdaptiveChangeDelay());
  response->printf_P(PSTR("\t \"camadptgain\": \"%d\",\n"), global::GetCamAdaptiveGaincontrol());
  response->printf_P(PSTR("\t \"camadptgaindown\": \"%d\",\n"), global::GetCamAdaptiveGainDown());
  response->printf_P(PSTR("\t \"camadptgainup\": \"%d\",\n"), global::GetCamAdaptiveGainUp());
  response->printf_P(PSTR("\t \"camadptgainmax\": \"%d\",\n"), global::GetCamAdaptiveGainMax());
  response->printf_P(PSTR("\t \"camdcw\": \"%d\",\n"), global::GetCamDcw());
  response->printf_P(PSTR("\t \"camautoonoff\": \"%d\",\n"), global::GetCamAutoOnOff()); 
  response->printf_P(PSTR("\t \"camautoonoffdelay\": \"%d\",\n"), global::GetCamAutoOnOffDelay()); 
#endif     
#if defined(ENABLE_HUE)
  response->printf_P(PSTR("\t \"huesupportenabled\": \"%d\",\n"), global::GetHueSupport()); 
  response->printf_P(PSTR("\t \"huebridgeip\": \"%s\",\n"), global::GetHueBridgeIP()); 
  response->printf_P(PSTR("\t \"hueubridgeuser\": \"%s\",\n"), (isAdminRequest(request) ? global::GetHueBridgeUser() : "")); 
  response->printf_P(PSTR("\t \"hueframelength\": \"%d\",\n"), global::GetFrameLengthDtls()); 
  response->printf_P(PSTR("\t \"huehandshaketimeout\": \"%d\",\n"), global::GetHandshakeTimeout()); 
  response->printf_P(PSTR("\t \"huepreset\": \"%d\",\n"), global::GetHuePreset()); 
#endif     
#if defined(ENABLE_DREAMSCREEN)
  response->printf_P(PSTR("\t \"dreamscreensupport\": \"%d\",\n"), global::GetDreamScreenSupport()); 
  response->printf_P(PSTR("\t \"dreamscreenemulation\": \"%d\",\n"), global::GetDreamScreenEmulation()); 
  response->printf_P(PSTR("\t \"dreamscreengroupid\": \"%d\",\n"), global::GetDreamScreenGroup());
  response->printf_P(PSTR("\t \"dreamscreenlowcolorsmoothing\": \"%d\",\n"), global::GetDreamScreenLowColorSmoothing());
#endif
#if defined(ENABLE_RELAY)
  response->printf_P(PSTR("\t \"ambibackrelayto\": \"%s\",\n"), global::GetAmbibackSelectedDevice() );
  response->printf_P(PSTR("\t \"ambibackrelaydirect\": \"%d\",\n"), global::GetAmbibackRelayModeDirect());
  response->printf_P(PSTR("\t \"ambibackrelaywifidirect\": \"%d\",\n"), global::GetAmbibackWifiDirectMode());
  response->printf_P(PSTR("\t \"ambibackrelaymode\": \"%d\",\n"), global::GetAmbibackRelayMode());
#endif

  response->printf_P(PSTR("\t\"finished\": \"0\"\n")); 
  response->printf_P(PSTR("}\n")); 

  //send the response last
  request->send(response);
}

void handleGetStatusAsJson(AsyncWebServerRequest *request){
  _DBGPRINTF(PSTR("WebServer: requested %s \n"), request->url().c_str());
  // Start response
  AsyncResponseStream *response = request->beginResponseStream(APP_JSON);
  ADD_CORS_HEADER(response);
  response->printf_P(PSTR("{\n"));
  response->printf_P(PSTR("\t\"devicename\": \"%s\",\n"), global::GetDeviceName());
  response->printf_P(PSTR("\t\"deviceip\": \"%s\",\n"), WiFi.localIP().toString().c_str());
#if defined(ENABLE_WIFI_AP)
  response->printf_P(PSTR("\t\"provisioningactive\": \"%d\",\n"), global::IsWifiProvisioningActive());
  response->printf_P(PSTR("\t\"provisioningpending\": \"%d\",\n"), global::IsWifiProvisioningPending());
  response->printf_P(PSTR("\t\"provisioningenabled\": \"%d\",\n"), global::GetWifiProvisioningEnabled());
  response->printf_P(PSTR("\t\"provisioningtimeoutminutes\": \"%d\",\n"), global::GetWifiProvisioningTimeoutMinutes());
  response->printf_P(PSTR("\t\"provisioningstarted\": \"%lu\",\n"), global::GetWifiProvisioningStarted());
  response->printf_P(PSTR("\t\"provisioningexpiresin\": \"%lu\",\n"), global::GetWifiProvisioningExpiresIn());
  response->printf_P(PSTR("\t\"provisioningapsecurity\": \"%s\",\n"), "wpa2");
#endif
  response->printf_P(PSTR("\t\"videostatus\": \"%s\",\n"), (global::m_pLedHelper->GetLedMode() == LED_MODE_VIDEO ? "connected" : "disconnected")); 
  response->printf_P(PSTR("\t\"modestatus\": \"%s\",\n"), global::m_pLedHelper->GetLedModeStr()); 
  response->printf_P(PSTR("\t\"wifinetwork\": \"%s\",\n"), WiFi.SSID().c_str()); 
  response->printf_P(PSTR("\t\"wifirssi\": \"%d\",\n"), WiFi.RSSI()); 
  response->printf_P(PSTR("\t\"ledfps\": \"%d\",\n"), global::m_pLedHelper->GetCurrentFps()); 
  response->printf_P(PSTR("\t\"netfps\": \"%d\",\n"), global::GetCurrentPkts());
  response->printf_P(PSTR("\t\"netdrpfps\": \"%d\",\n"), global::GetCurrentPktsDropped());
  response->printf_P(PSTR("\t\"freemem\": \"%d\",\n"), heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
  response->printf_P(PSTR("\t\"freeheap\": \"%d\",\n"), ESP.getFreeHeap());
  response->printf_P(PSTR("\t\"freepsram\": \"%d\",\n"), (psramFound() ? heap_caps_get_free_size(MALLOC_CAP_8BIT) : 0));
#if defined(ESP32_RUNTIMESTATS)
  response->printf_P(PSTR("\t\"cpufreq\": \"%d\",\n"), getCpuFrequencyMhz());
  response->printf_P(PSTR("\t\"cpuxtalfreq\": \"%d\",\n"), getXtalFrequencyMhz());
  response->printf_P(PSTR("\t\"cpunumtasks\": \"%d\",\n"), uxTaskGetNumberOfTasks());
#endif
#if defined(ENABLE_CAM)
  response->printf_P(PSTR("\t\"camfps\": \"%d\",\n"), global::m_pCamProcessor->GetCurrentCamFps());
  response->printf_P(PSTR("\t\"blkscnds\": \"%d\",\n"), global::m_pCamProcessor->GetBlackSeconds());
  response->printf_P(PSTR("\t\"camavrout\": \"%d\",\n"), global::m_pCamProcessor->GetAvrOutside());
  response->printf_P(PSTR("\t\"camavrin\": \"%d\",\n"), global::m_pCamProcessor->GetAvrInside());
  response->printf_P(PSTR("\t\"camaecvalue\": \"%d\",\n"), global::GetCamAecValue());
  response->printf_P(PSTR("\t\"camagcgain\": \"%d\",\n"), global::GetCamAgcGain());
#endif
#if defined(ENABLE_HUE)
  response->printf_P(PSTR("\t\"hueenabled\": \"%d\",\n"), global::GetHueSupport()); 
  response->printf_P(PSTR("\t\"huefps\": \"%d\",\n"), global::m_pHueHelper->GetCurrentFps()); 
  response->printf_P(PSTR("\t\"huestatus\": \"%s\",\n"), (global::m_pHueHelper->GetHueDtlsPrepared() == true ? (global::m_pHueHelper->GetHueDtlsConnected() ? "connected" : "prepared") : "disconnected")); 
  response->printf_P(PSTR("\t\"hueerror\": \"%s\",\n"), global::m_pHueHelper->GetHueLastError());
#endif
#if defined(ENABLE_DREAMSCREEN)
  response->printf_P(PSTR("\t\"dreamscreenstatus\": \"%s\",\n"), (global::m_pDreamScreen->GetConnected() == true ? "connected" : "disconnected") ); 
  response->printf_P(PSTR("\t\"dreamscreenfps\": \"%d\",\n"), global::m_pDreamScreen->GetCurrentFps());
#endif
  
  response->printf_P(PSTR("\t\"finished\": \"0\"\n")); 
  response->printf_P(PSTR("}\n")); 

  //send the response last
  request->send(response);
}

void handleDiscover(AsyncWebServerRequest *request) {
  _DBGPRINTF(PSTR("WebServer: handleDiscover: requested %s\n"), request->url().c_str());
  if (UdpServer::m_pUdpServer == NULL) {
    AsyncWebServerResponse *r = request->beginResponse(500, APP_JSON, PSTR("[]"));
    r->addHeader(PSTR("Access-Control-Allow-Origin"), PSTR("*"));
    request->send(r);
    return;
  }
  char szBuf[2048];
  UdpServer::RunHttpDiscover(szBuf, sizeof(szBuf));
  AsyncWebServerResponse *r = request->beginResponse(200, APP_JSON, szBuf);
  r->addHeader(PSTR("Access-Control-Allow-Origin"), PSTR("*"));
  request->send(r);
}

// --- 

void handleGetScanNetworksAsJson(AsyncWebServerRequest *request){
  _DBGPRINTF(PSTR("WebServer::handleGetScanNetworksAsJson: requested %s, start WIFI SCAN\n"), request->url().c_str());

  // Start response
  AsyncResponseStream *response = request->beginResponseStream(APP_JSON);
  ADD_CORS_HEADER(response);
  response->printf_P(PSTR("{\n")); 
  
  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  if (n == 0) {
    _DBGPRINTF(PSTR("WebServer::handleGetScanNetworksAsJson: no networks found\n"));
  } else {
    _DBGPRINTF(PSTR("WebServer::handleGetScanNetworksAsJson: %d networks found\n"), n);
    response->printf_P(PSTR("\t\"networks\":{\n"));
    for(int iIndex=0; iIndex < n; iIndex++) {
      _DBGPRINTF_ULTRA(PSTR("WebServer::handleGetScanNetworksAsJson: %d SSID %s RSSI %d Encryption: %s \n"),
        iIndex,
        (WiFi.SSID(iIndex) != NULL ? WiFi.SSID(iIndex).c_str() : "%INVALID%"),
        WiFi.RSSI(iIndex),
        global::GetWifiAuthTxt(WiFi.encryptionType(iIndex)));
      
      response->printf_P(PSTR("\t\t\"%d\":{\n"), iIndex);
      response->printf_P(PSTR("\t\t\t\"SSID\":\"%s\",\n"), (WiFi.SSID(iIndex) != NULL ? WiFi.SSID(iIndex).c_str() : "%INVALID%") );
      response->printf_P(PSTR("\t\t\t\"RSSI\":\"%d\",\n"), WiFi.RSSI(iIndex) );
      response->printf_P(PSTR("\t\t\t\"ENC\":\"%d\"\n"), WiFi.encryptionType(iIndex) );
      response->printf_P(PSTR("\t\t}"));
      if (iIndex < n-1) /* only add , if other element follows */
        response->printf_P(PSTR(",\n"));
    }
    response->printf_P(PSTR("\t},\n"));
  }
  response->printf_P(PSTR("\t\"finished\": \"0\"\n")); 
  response->printf_P(PSTR("}\n")); 

  //send the response last
  request->send(response);
}

#if defined(ENABLE_WIFI_AP)

static bool isProvisioningAllowed(AsyncWebServerRequest *request) {
  return !global::IsDeviceClaimed() || global::IsProvisioningPhysical() || isAdminRequest(request);
}

static void writeProvisioningStateJson(AsyncResponseStream *response, bool bShowToken) {
  response->printf_P(PSTR("{\n"));
  response->printf_P(PSTR("\t\"active\": \"%d\",\n"), global::IsWifiProvisioningActive());
  response->printf_P(PSTR("\t\"pending\": \"%d\",\n"), global::IsWifiProvisioningPending());
  response->printf_P(PSTR("\t\"state\": \"%s\",\n"),
    (global::IsWifiProvisioningActive() ? (global::IsWifiProvisioningPending() ? "pending" : "active") : "idle"));
  response->printf_P(PSTR("\t\"enabled\": \"%d\",\n"), global::GetWifiProvisioningEnabled());
  response->printf_P(PSTR("\t\"timeoutminutes\": \"%d\",\n"), global::GetWifiProvisioningTimeoutMinutes());
  response->printf_P(PSTR("\t\"started\": \"%lu\",\n"), global::GetWifiProvisioningStarted());
  response->printf_P(PSTR("\t\"expiresin\": \"%lu\",\n"), global::GetWifiProvisioningExpiresIn());
  response->printf_P(PSTR("\t\"token\": \"%s\",\n"), (bShowToken ? global::GetWifiProvisioningToken() : ""));
  response->printf_P(PSTR("\t\"claimed\": \"%d\",\n"), global::IsDeviceClaimed());
  response->printf_P(PSTR("\t\"claimable\": \"%d\",\n"), global::CanClaimDevice());
  response->printf_P(PSTR("\t\"apsecurity\": \"%s\",\n"), "wpa2");
  response->printf_P(PSTR("\t\"reason\": \"%s\",\n"),
    (!global::GetWifiProvisioningEnabled() ? "disabled" : (global::IsWifiProvisioningActive() ? "boot_window" : "idle")));
  response->printf_P(PSTR("\t\"apenabled\": \"%d\",\n"), global::GetWifiAPEnable());
  response->printf_P(PSTR("\t\"apssid\": \"%s\",\n"), global::GetDeviceAPssid());
  response->printf_P(PSTR("\t\"apip\": \"%s\"\n"), WiFi.softAPIP().toString().c_str());
  response->printf_P(PSTR("}\n"));
}

void handleGetProvisioningAsJson(AsyncWebServerRequest *request) {
  _DBGPRINTF(PSTR("WebServer::handleGetProvisioningAsJson: requested %s\n"), request->url().c_str());

  AsyncResponseStream *response = request->beginResponseStream(APP_JSON);
  ADD_CORS_HEADER(response);
  writeProvisioningStateJson(response, isProvisioningAllowed(request));

  request->send(response);
}

void handleProvisioningStart(AsyncWebServerRequest *request) {
  if (!isProvisioningAllowed(request)) {
    sendForbidden(request);
    return;
  }
  _DBGPRINTF(PSTR("WebServer::handleProvisioningStart: requested %s\n"), request->url().c_str());

  AsyncResponseStream *response = request->beginResponseStream(APP_JSON);
  ADD_CORS_HEADER(response);
  response->printf_P(PSTR("{\n"));
  response->printf_P(PSTR("\t\"success\": \"%d\",\n"), (global::IsWifiProvisioningActive() ? 1 : 0));
  response->printf_P(PSTR("\t\"state\": \"%s\",\n"),
    (global::IsWifiProvisioningActive() ? (global::IsWifiProvisioningPending() ? "pending" : "active") : "idle"));
  response->printf_P(PSTR("\t\"enabled\": \"%d\",\n"), global::GetWifiProvisioningEnabled());
  response->printf_P(PSTR("\t\"timeoutminutes\": \"%d\",\n"), global::GetWifiProvisioningTimeoutMinutes());
  response->printf_P(PSTR("\t\"started\": \"%lu\",\n"), global::GetWifiProvisioningStarted());
  response->printf_P(PSTR("\t\"expiresin\": \"%lu\",\n"), global::GetWifiProvisioningExpiresIn());
  response->printf_P(PSTR("\t\"token\": \"%s\",\n"), global::GetWifiProvisioningToken());
  response->printf_P(PSTR("\t\"claimed\": \"%d\",\n"), global::IsDeviceClaimed());
  response->printf_P(PSTR("\t\"claimable\": \"%d\",\n"), global::CanClaimDevice());
  response->printf_P(PSTR("\t\"apsecurity\": \"%s\",\n"), "wpa2");
  response->printf_P(PSTR("\t\"reason\": \"%s\",\n"),
    (!global::GetWifiProvisioningEnabled() ? "disabled" : (global::IsWifiProvisioningActive() ? "boot_window" : "idle")));
  response->printf_P(PSTR("\t\"apenabled\": \"%d\",\n"), global::GetWifiAPEnable());
  response->printf_P(PSTR("\t\"apssid\": \"%s\",\n"), global::GetDeviceAPssid());
  response->printf_P(PSTR("\t\"apip\": \"%s\"\n"), WiFi.softAPIP().toString().c_str());
  response->printf_P(PSTR("}\n"));

  request->send(response);
}

void handleProvisioningSubmit(AsyncWebServerRequest *request) {
  if (!isProvisioningAllowed(request)) {
    sendForbidden(request);
    return;
  }
  _DBGPRINTF(PSTR("WebServer::handleProvisioningSubmit: requested %s\n"), request->url().c_str());

  bool bTokenValid = false;
  String strToken;
  String strSSID;
  String strPassword;
  String strResult;

  if (request->hasParam("token", true)) {
    strToken = request->getParam("token", true)->value();
    bTokenValid = global::ValidateWifiProvisioningToken((char*)strToken.c_str());
  }

  _DBGPRINTF(PSTR("WebServer::handleProvisioningSubmit: active=%d pending=%d tokenPresent=%d tokenValid=%d ssidPresent=%d passwordPresent=%d keyPresent=%d\n"),
    global::IsWifiProvisioningActive(),
    global::IsWifiProvisioningPending(),
    request->hasParam("token", true),
    bTokenValid,
    request->hasParam("ssid", true),
    request->hasParam("password", true),
    request->hasParam("key", true));

  if (!global::IsWifiProvisioningActive() || !bTokenValid) {
    strResult = PSTR("ERROR, invalid provisioning token or inactive session");
  } else if (!request->hasParam("ssid", true)) {
    strResult = PSTR("ERROR, missing ssid");
  } else {
    strSSID = request->getParam("ssid", true)->value();
    if (request->hasParam("password", true)) {
      strPassword = request->getParam("password", true)->value();
    } else if (request->hasParam("key", true)) {
      strPassword = request->getParam("key", true)->value();
    } else {
      strPassword = "";
    }

    global::SetWifiProvisioningCredentials((char*)strSSID.c_str(), (char*)strPassword.c_str());
    global::SetWifiProvisioningPending(true);
    _DBGPRINTF(PSTR("WebServer::handleProvisioningSubmit: staged SSID=%s passwordLen=%d expiresIn=%lu\n"),
      global::GetWifiProvisioningSSID(),
      strlen(global::GetWifiProvisioningPassword()),
      global::GetWifiProvisioningExpiresIn());
    strResult = PSTR("Successfully staged provisioning credentials");
  }

  AsyncResponseStream *response = request->beginResponseStream(APP_JSON);
  ADD_CORS_HEADER(response);
  response->printf_P(PSTR("{\n"));
  response->printf_P(PSTR("\t\"success\": \"%d\",\n"), (strResult.startsWith("Successfully") ? 1 : 0));
  response->printf_P(PSTR("\t\"message\": \"%s\",\n"), strResult.c_str());
  response->printf_P(PSTR("\t\"state\": \"%s\",\n"), (global::IsWifiProvisioningPending() ? "pending" : (global::IsWifiProvisioningActive() ? "active" : "idle")));
  response->printf_P(PSTR("\t\"started\": \"%lu\",\n"), global::GetWifiProvisioningStarted());
  response->printf_P(PSTR("\t\"expiresin\": \"%lu\",\n"), global::GetWifiProvisioningExpiresIn());
  response->printf_P(PSTR("\t\"token\": \"%s\"\n"), global::GetWifiProvisioningToken());
  response->printf_P(PSTR("}\n"));

  request->send(response);
}

// Stores the key chosen by the app. Needs the session token and a claimable device.
void handleProvisioningClaim(AsyncWebServerRequest *request) {
  bool bTokenValid = request->hasParam("token", true) &&
    global::ValidateWifiProvisioningToken((char*)request->getParam("token", true)->value().c_str());
  bool bAllowed = global::CanClaimDevice() || (global::IsWifiProvisioningActive() && global::IsDeviceClaimed() && isAdminRequest(request));
  bool bStored = false;

  if (!bTokenValid || !bAllowed) {
    sendForbidden(request);
    return;
  }
  if (request->hasParam("newkey", true)) {
    bStored = global::SetClaimKey(request->getParam("newkey", true)->value().c_str(), true);
  }

  AsyncResponseStream *response = request->beginResponseStream(APP_JSON);
  ADD_CORS_HEADER(response);
  response->printf_P(PSTR("{\n\t\"success\": \"%d\",\n\t\"claimed\": \"%d\",\n\t\"message\": \"%s\"\n}\n"),
    bStored, global::IsDeviceClaimed(), (bStored ? "Device claimed" : "ERROR, invalid key"));
  request->send(response);
}

void handleProvisioningFinish(AsyncWebServerRequest *request) {
  if (!isProvisioningAllowed(request)) {
    sendForbidden(request);
    return;
  }
  _DBGPRINTF(PSTR("WebServer::handleProvisioningFinish: requested %s\n"), request->url().c_str());

  bool bTokenValid = false;
  String strToken;
  String strResult;
  bool bConnected = false;
  bool bRebootScheduled = false;

  if (request->hasParam("token", true)) {
    strToken = request->getParam("token", true)->value();
    bTokenValid = global::ValidateWifiProvisioningToken((char*)strToken.c_str());
  }

  _DBGPRINTF(PSTR("WebServer::handleProvisioningFinish: active=%d pending=%d tokenPresent=%d tokenValid=%d stagedSSID=%s passwordLen=%d expiresIn=%lu\n"),
    global::IsWifiProvisioningActive(),
    global::IsWifiProvisioningPending(),
    request->hasParam("token", true),
    bTokenValid,
    global::GetWifiProvisioningSSID(),
    strlen(global::GetWifiProvisioningPassword()),
    global::GetWifiProvisioningExpiresIn());

  if (!global::IsWifiProvisioningActive() || !bTokenValid) {
    strResult = PSTR("ERROR, invalid provisioning token or inactive session");
  } else if (!global::IsWifiProvisioningPending()) {
    strResult = PSTR("ERROR, no staged credentials available");
  } else {
    char* pszSSID = global::GetWifiProvisioningSSID();
    char* pszPassword = global::GetWifiProvisioningPassword();

    if (pszSSID == NULL || strlen(pszSSID) == 0) {
      strResult = PSTR("ERROR, staged ssid is empty");
    } else {
      _DBGPRINTF(PSTR("WebServer::handleProvisioningFinish: storing staged SSID=%s passwordLen=%d and scheduling reboot\n"),
        pszSSID,
        strlen(pszPassword));
      global::SetWifiSSID(pszSSID, true);
      global::SetWifiPassword(pszPassword, true);
      global::SetWifiProvisioningPending(false);
      global::m_pEepromHelper->CommitChanges();
      global::ScheduleReboot(1000);
      bConnected = true;
      bRebootScheduled = true;
      strResult = PSTR("Successfully stored provisioning credentials, reboot scheduled");
    }
  }

  _DBGPRINTF(PSTR("WebServer::handleProvisioningFinish: result=%s finalState=%s active=%d pending=%d rebootAt=%lu\n"),
    strResult.c_str(),
    (bRebootScheduled ? "rebooting" : (global::IsWifiProvisioningPending() ? "pending" : (global::IsWifiProvisioningActive() ? "active" : "idle"))),
    global::IsWifiProvisioningActive(),
    global::IsWifiProvisioningPending(),
    global::GetRebootAt());

  AsyncResponseStream *response = request->beginResponseStream(APP_JSON);
  ADD_CORS_HEADER(response);
  response->printf_P(PSTR("{\n"));
  response->printf_P(PSTR("\t\"success\": \"%d\",\n"), (bConnected ? 1 : 0));
  response->printf_P(PSTR("\t\"message\": \"%s\",\n"), strResult.c_str());
  response->printf_P(PSTR("\t\"state\": \"%s\",\n"), (bRebootScheduled ? "rebooting" : (global::IsWifiProvisioningPending() ? "pending" : (global::IsWifiProvisioningActive() ? "active" : "idle"))));
  response->printf_P(PSTR("\t\"expiresin\": \"%lu\",\n"), global::GetWifiProvisioningExpiresIn());
  response->printf_P(PSTR("\t\"connected\": \"%d\",\n"), bConnected);
  response->printf_P(PSTR("\t\"rebootin\": \"%d\"\n"), (bRebootScheduled ? 1000 : 0));
  response->printf_P(PSTR("}\n"));

  request->send(response);
}

#endif

void handleGetLastStatusMessageAsJson(AsyncWebServerRequest *request){
  _DBGPRINTF(PSTR("WebServer::handleGetLastStatusMessageAsJson: requested %s, return LAST STATUS MESSAGE\n"), request->url().c_str());

  // Start response
  AsyncResponseStream *response = request->beginResponseStream(APP_JSON);
  ADD_CORS_HEADER(response);
  response->printf_P(PSTR("{\n")); 
  if (global::m_pWebServer->GetLastStatusMessage() != NULL) {
    _DBGPRINTF(PSTR("WebServer::handleGetLastStatusMessageAsJson: return status msg %s\n"), global::m_pWebServer->GetLastStatusMessage());

    response->printf_P(PSTR("\t%s,\n"), global::m_pWebServer->GetLastStatusMessage());
    global::m_pWebServer->SetLastStatusMessage(NULL, 0); // erase message
  } else {
    response->printf_P(PSTR("\t\"status\":\"\",\n") );
  }
  
  response->printf_P(PSTR("\t\"finished\": \"0\"\n")); 
  response->printf_P(PSTR("}\n")); 

  //send the response last
  request->send(response);
}
