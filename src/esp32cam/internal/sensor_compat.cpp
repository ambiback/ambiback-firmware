#include "../esp32cam.h"

#include <esp_camera.h>
#include <sensor.h>

namespace esp32cam {

#define CAM_SENSOR_CALL(f, ...) do { \
  sensor_t* s = esp_camera_sensor_get(); \
  if (!s || !s->f) return false; \
  return s->f(s, __VA_ARGS__) == 0; \
} while(0)

// Same argument mapping as the AmbiBack esp32cam fork: pre is passed as root.
bool CameraClass::changePll(int mul, int pre, int pclk) {
  sensor_t* s = esp_camera_sensor_get();
  if (!s || !s->set_pll) return false;
  return s->set_pll(s, false, mul, 1, pre, false, 0, true, pclk) == 0;
}

// Enables or disables COM7_ZOOM_EN (register 0x12, bit 0x04).
bool CameraClass::changeZoom(int enable) {
  sensor_t* s = esp_camera_sensor_get();
  if (!s || !s->set_reg) return false;
  return s->set_reg(s, 0x12, 0xFF, enable ? 0x04 : 0) == 0;
}

bool CameraClass::changeFrequence(int mhz) {
  sensor_t* s = esp_camera_sensor_get();
  if (!s || !s->set_xclk) return false;
  return s->set_xclk(s, LEDC_TIMER_0, mhz * 1000000) == 0;
}

// Light frequency (anti-flicker) on OV3660, from the AmbiBack esp32cam fork.
// Returns 0 for 60 Hz and 1 for 50 Hz.
int CameraClass::getBandMode() {
  sensor_t* s = esp_camera_sensor_get();
  if (!s || !s->get_reg) return 0;
  return s->get_reg(s, 0x3C0C, 0x01);
}

// Mode 0 = automatic, 1 = manual 50 Hz, 2 = manual 60 Hz.
bool CameraClass::changeBandMode(int mode) {
  sensor_t* s = esp_camera_sensor_get();
  if (!s || !s->set_reg) return false;
  if (mode == 0) {
    // Automatic detection: banding filter on, 0x3004[2] = 1.
    return s->set_reg(s, 0x3a00, 0x32, 0x32) == 0 &&
           s->set_reg(s, 0x3004, 0x04, 0x04) == 0 &&
           s->set_reg(s, 0x3c00, 0x04, 0x00) == 0;
  }
  // Manual mode: switch automatic detection off first.
  if (s->set_reg(s, 0x3a00, 0x32, 0x00) != 0 || s->set_reg(s, 0x3004, 0x04, 0x00) != 0) {
    return false;
  }
  if (mode == 1) {
    // 50 Hz
    if (s->set_reg(s, 0x3c00, 0x04, 0x04) != 0) return false;
    s->set_reg(s, 0x3a08, 0x00, 0x00); // band width
    s->set_reg(s, 0x3a09, 0xeb, 0xeb); // band width
    s->set_reg(s, 0x3a0e, 0x06, 0x06); // max band
    s->set_reg(s, 0x3C0C, 0x01, 0x01);
    return true;
  }
  // 60 Hz
  if (s->set_reg(s, 0x3c00, 0x04, 0x00) != 0) return false;
  s->set_reg(s, 0x3a0a, 0x00, 0x00); // band width
  s->set_reg(s, 0x3a0b, 0xc4, 0xc4); // band width
  s->set_reg(s, 0x3a0d, 0x08, 0x08); // max band
  s->set_reg(s, 0x3C0C, 0x01, 0x00);
  return true;
}
bool CameraClass::changeContrast(int level) { CAM_SENSOR_CALL(set_contrast, level); }
bool CameraClass::changeBrightness(int level) { CAM_SENSOR_CALL(set_brightness, level); }
bool CameraClass::changeSaturation(int level) { CAM_SENSOR_CALL(set_saturation, level); }
bool CameraClass::changeSpecialEffect(int effect) { CAM_SENSOR_CALL(set_special_effect, effect); }
bool CameraClass::changeWbMode(int mode) { CAM_SENSOR_CALL(set_wb_mode, mode); }
bool CameraClass::changeAeLevels(int level) { CAM_SENSOR_CALL(set_ae_level, level); }
bool CameraClass::changeAwbGainControl(int enable) { CAM_SENSOR_CALL(set_awb_gain, enable); }
bool CameraClass::changAgcGain(int gain) { CAM_SENSOR_CALL(set_agc_gain, gain); }

bool CameraClass::changGainceilingSensor(int gainceiling) {
  sensor_t* s = esp_camera_sensor_get();
  if (!s || !s->set_gainceiling) return false;
  return s->set_gainceiling(s, static_cast<gainceiling_t>(gainceiling)) == 0;
}

bool CameraClass::changeGaincontrol(int enable) { CAM_SENSOR_CALL(set_gain_ctrl, enable); }
bool CameraClass::changeWhitebalance(int enable) { CAM_SENSOR_CALL(set_whitebal, enable); }
bool CameraClass::changeColorbar(int enable) { CAM_SENSOR_CALL(set_colorbar, enable); }
bool CameraClass::changeQuality(int quality) { CAM_SENSOR_CALL(set_quality, quality); }
bool CameraClass::changAec2(int enable) { CAM_SENSOR_CALL(set_aec2, enable); }
bool CameraClass::changeAecValue(int value) { CAM_SENSOR_CALL(set_aec_value, value); }
bool CameraClass::changHMirror(int enable) { CAM_SENSOR_CALL(set_hmirror, enable); }
bool CameraClass::changVFlip(int enable) { CAM_SENSOR_CALL(set_vflip, enable); }
bool CameraClass::changBPC(int enable) { CAM_SENSOR_CALL(set_bpc, enable); }
bool CameraClass::changWPC(int enable) { CAM_SENSOR_CALL(set_wpc, enable); }
bool CameraClass::changDenoise(int enable) { CAM_SENSOR_CALL(set_denoise, enable); }
bool CameraClass::changLenc(int enable) { CAM_SENSOR_CALL(set_lenc, enable); }
bool CameraClass::changRawGMA(int enable) { CAM_SENSOR_CALL(set_raw_gma, enable); }
bool CameraClass::changAutoExposurecontrol(int enable) { CAM_SENSOR_CALL(set_exposure_ctrl, enable); }
bool CameraClass::changDcw(int enable) { CAM_SENSOR_CALL(set_dcw, enable); }

} // namespace esp32cam
