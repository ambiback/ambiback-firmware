#include "../esp32cam.h"

#include <esp_camera.h>
#include <sensor.h>

namespace esp32cam {

#define CAM_SENSOR_CALL(f, ...) do { \
  sensor_t* s = esp_camera_sensor_get(); \
  if (!s || !s->f) return false; \
  return s->f(s, __VA_ARGS__) == 0; \
} while(0)

bool CameraClass::changePll(int mul, int pre, int pclk) {
  sensor_t* s = esp_camera_sensor_get();
  if (!s || !s->set_pll) return false;
  return s->set_pll(s, 0, mul, 1, 0, pre, 0, 1, pclk) == 0;
}

bool CameraClass::changeFrequence(int mhz) {
  sensor_t* s = esp_camera_sensor_get();
  if (!s || !s->set_xclk) return false;
  return s->set_xclk(s, LEDC_TIMER_0, mhz * 1000000) == 0;
}

int CameraClass::getBandMode() {
  sensor_t* s = esp_camera_sensor_get();
  return s ? s->status.denoise : 0;
}

bool CameraClass::changeBandMode(int mode) { CAM_SENSOR_CALL(set_denoise, mode); }
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
