# esp32cam v0.0.20240626 (AmbiBack patched)

Vendored [yoursunny/esp32cam](https://github.com/yoursunny/esp32cam) v0.0.20240626 with AmbiBack compatibility extensions.

## Differences from upstream

- **Config::setFreqHz(uint32_t)** – set XCLK frequency
- **Camera::lastEspErr** – last ESP error from init/ops
- **Camera** sensor wrappers: changePll, changeFrequence, getBandMode, changeBandMode, changeContrast, changeBrightness, changeSaturation, changeSpecialEffect, changeWbMode, changeAeLevels, changeAwbGainControl, changAgcGain, changGainceilingSensor, changeGaincontrol, changeWhitebalance, changeColorbar, changeQuality, changAec2, changeAecValue, changHMirror, changVFlip, changBPC, changWPC, changDenoise, changLenc, changRawGMA, changAutoExposurecontrol, changDcw
- **Frame::getPixFormat()** – get pixel format
- **Frame::setData()** – replace buffer (e.g. after RGB conversion)
- **Frame::writeTo(AsyncClient&)** – write to AsyncTCP client
- **MjpegConfig::bAbort** – stream abort flag
- **StreamMjpegConfig** – deprecated alias for MjpegConfig (inherits bAbort)

## Installation (Arduino)

Replace the Arduino library with this vendored copy:

```powershell
# Backup existing library
$arduinoLibs = "D:\!cvsroot\Arduino\libraries"
$backup = "$arduinoLibs\esp32cam.bak"
if (Test-Path "$arduinoLibs\esp32cam") {
  if (Test-Path $backup) { Remove-Item $backup -Recurse -Force }
  Rename-Item "$arduinoLibs\esp32cam" "esp32cam.bak"
}
# Copy vendored library
Copy-Item -Path "D:\!cvsroot\_Misc\AmbiBack.Controller\lib\esp32cam" -Destination "$arduinoLibs\esp32cam" -Recurse -Force
```

Or manually replace `D:\!cvsroot\Arduino\libraries\esp32cam` with the contents of this folder.
