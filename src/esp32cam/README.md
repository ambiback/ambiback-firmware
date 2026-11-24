# esp32cam for AmbiBack

This folder contains a modified copy of [yoursunny/esp32cam](https://github.com/yoursunny/esp32cam) v0.0.20240626. It is part of the firmware sketch and is compiled from `src/`, so no separately installed esp32cam library is needed. An esp32cam library in your Arduino libraries folder is ignored.

The firmware includes it with:

```cpp
#include "src/esp32cam/esp32cam.h"
```

The original ISC license is kept in `LICENSE`.

## Differences from upstream

- **Config::setFreqHz(uint32_t)**: set the XCLK frequency.
- **Camera::lastEspErr**: last ESP error from init and sensor calls.
- **Camera** sensor wrappers: changePll, changeFrequence, getBandMode, changeBandMode, changeZoom, changeContrast, changeBrightness, changeSaturation, changeSpecialEffect, changeWbMode, changeAeLevels, changeAwbGainControl, changAgcGain, changGainceilingSensor, changeGaincontrol, changeWhitebalance, changeColorbar, changeQuality, changAec2, changeAecValue, changHMirror, changVFlip, changBPC, changWPC, changDenoise, changLenc, changRawGMA, changAutoExposurecontrol, changDcw.
- **Frame::getPixFormat()**: get the pixel format.
- **Frame::setData()**: replace the buffer, for example after RGB conversion.
- **Frame::writeTo(AsyncClient&)**: write to an AsyncTCP client.
- **MjpegConfig::bAbort**: stream abort flag.
- **StreamMjpegConfig**: deprecated alias for MjpegConfig.

## Taken from the earlier AmbiBack esp32cam fork

These parts follow the fork that earlier AmbiBack firmware was built with:

- **changeBandMode / getBandMode**: OV3660 anti-flicker through sensor registers. Mode 0 is automatic, 1 is 50 Hz, 2 is 60 Hz. `getBandMode` returns 1 for 50 Hz and 0 for 60 Hz.
- **changePll**: the same `set_pll` argument mapping as the fork.
- **changeZoom**: switches COM7_ZOOM_EN (register 0x12).
- Frame writes yield with `taskYIELD()`.
