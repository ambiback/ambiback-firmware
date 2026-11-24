/**
 * @mainpage esp32cam
 *
 * https://github.com/yoursunny/esp32cam
 */

#ifndef ESP32CAM_H
#define ESP32CAM_H

#include "internal/config.hpp"
#include "internal/mjpeg.hpp"

namespace esp32cam {

class CameraClass {
public:
  /**
   * @brief Enable camera.
   * @return whether success.
   */
  bool begin(const Config& config);

  /**
   * @brief Disable camera.
   * @return whether success.
   */
  bool end();

  /**
   * @brief Retrieve list of resolutions (likely) supported by hardware.
   * @pre Camera is enabled.
   */
  ResolutionList listResolutions() const;

  /**
   * @brief Change camera resolution.
   * @pre Camera is enabled.
   * @param resolution new resolution; must be no higher than initial resolution.
   * @param sleepFor how long to wait for stabilization (millis).
   */
  bool changeResolution(const Resolution& resolution, int sleepFor = 500);

  /**
   * @brief Capture a frame of picture.
   * @pre Camera is enabled.
   * @return the picture frame, or nullptr on error.
   */
  std::unique_ptr<Frame> capture();

  struct [[deprecated("use esp32cam::MjpegConfig")]] StreamMjpegConfig : MjpegConfig{};

  /**
   * @brief Stream Motion JPEG.
   * @pre The camera has been initialized to JPEG mode.
   * @return number of frames streamed.
   */
  int streamMjpeg(Client& client, const MjpegConfig& cfg = MjpegConfig());

  /** @brief AmbiBack: last ESP error from init/ops (esp_err_t). */
  long lastEspErr = 0;

  /** @brief AmbiBack: sensor control wrappers */
  bool changePll(int mul, int pre, int pclk);
  bool changeFrequence(int mhz);
  int getBandMode();
  bool changeBandMode(int mode);
  bool changeContrast(int level);
  bool changeBrightness(int level);
  bool changeSaturation(int level);
  bool changeSpecialEffect(int effect);
  bool changeWbMode(int mode);
  bool changeAeLevels(int level);
  bool changeAwbGainControl(int enable);
  bool changAgcGain(int gain);
  bool changGainceilingSensor(int gainceiling);
  bool changeGaincontrol(int enable);
  bool changeWhitebalance(int enable);
  bool changeColorbar(int enable);
  bool changeQuality(int quality);
  bool changAec2(int enable);
  bool changeAecValue(int value);
  bool changHMirror(int enable);
  bool changVFlip(int enable);
  bool changBPC(int enable);
  bool changWPC(int enable);
  bool changDenoise(int enable);
  bool changLenc(int enable);
  bool changRawGMA(int enable);
  bool changAutoExposurecontrol(int enable);
  bool changDcw(int enable);
  bool changeZoom(int enable);
};

/** @brief ESP32 camera API. */
extern CameraClass Camera;

/** @brief Capture a frame with the camera. */
inline std::unique_ptr<Frame>
capture() {
  return Camera.capture();
}

} // namespace esp32cam

#endif // ESP32CAM_H
