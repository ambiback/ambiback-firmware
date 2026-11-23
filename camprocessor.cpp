/* CamProcessor: camera image -> sector CRGB buffer. JPEG decode, geometry, color extraction, smoothing.
   Include config first so ENABLE_CAM is defined before the guard. */
#include "globalincludes.h"
#if defined(ENABLE_CAM)

#include "camprocessor.h"
#include "global.h"
#include "FastLED.h"
#include "eepromhelper.h"
#include "img_converters.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#if !(defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3)
#include "esp_jpg_decode.h"
#else
typedef esp_jpeg_image_scale_t jpg_scale_t;
#endif
#include "rom/tjpgd.h"

typedef struct {
  uint16_t width;
  uint16_t height;
  uint16_t data_offset;
  const uint8_t* input;
  uint8_t* output;
} rgb_jpg_decoder;

typedef struct {
  rgb_jpg_decoder jpeg;
  CamProcessor* pProc;
} jpeg_direct_arg;

static unsigned int _jpg_read(void* arg, size_t index, uint8_t* buf, size_t len) {
  jpeg_direct_arg* a = (jpeg_direct_arg*)arg;
  if (buf && a->jpeg.input)
    memcpy(buf, a->jpeg.input + index, len);
  return (unsigned int)len;
}

static bool _rgb_write(void* arg, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* data) {
  rgb_jpg_decoder* jpeg = (rgb_jpg_decoder*)arg;
  if (!data) {
    if (x == 0 && y == 0) {
      jpeg->width = w;
      jpeg->height = h;
      if (!jpeg->output)
        jpeg->output = (uint8_t*)_malloc_psram((w * h * 3) + jpeg->data_offset);
      if (!jpeg->output) return false;
    }
    return true;
  }
  size_t jw = jpeg->width * 3;
  size_t t = y * jw;
  size_t b = t + (h * jw);
  size_t l = x * 3;
  uint8_t* out = jpeg->output + jpeg->data_offset;
  uint8_t* o = out;
  size_t iy, ix;
  w = w * 3;
  for (iy = t; iy < b; iy += jw) {
    o = out + iy + l;
    for (ix = 0; ix < w; ix += 3) {
      o[ix] = data[ix + 2];
      o[ix + 1] = data[ix + 1];
      o[ix + 2] = data[ix];
    }
    data += w;
  }
  return true;
}

static bool _rgb_direct(void* arg, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* data) {
  jpeg_direct_arg* a = (jpeg_direct_arg*)arg;
  bool isStart = (!data && x == 0 && y == 0);
  bool isEnd = (!data && !isStart);
  if (isStart) {
    a->jpeg.width = w;
    a->jpeg.height = h;
  }
  return a->pProc->ProcessRgbDirectBlock(x, y, w, h, data, isStart, isEnd);
}

bool CamProcessor::ProcessRgbDirectBlock(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* data, bool isStart, bool isEnd) {
  if (isStart) {
    camCalcColorsInitFrameVariables();
    return true;
  }
  if (isEnd) {
    camCalcColorsFinishFrameVariables();
    return true;
  }
  int iBlue, iRed, iGreen, iAvarage;
  size_t outputWidthBytes = w * 3;
  size_t yEnd = y + h;
  size_t xInLine = 0;
  size_t xBytePos = 0;
  int* piCamPoints = m_piCamPoints;
  if (m_bCamCalcJpegBlocks && m_pCamPixelsInJpegBlocks != NULL) {
    for (size_t y1 = y; y1 < yEnd; y1++) {
      for (size_t x1 = 0; x1 < w; x1++) {
        xInLine = x + x1;
        if (TestBit(piCamPoints, BitIndexEx(LEDSINGLECOLOR, y1, xInLine, Getlev1size(), Getlev2size()))) {
          m_pCamPixelsInJpegBlocks[m_iCamCurrentJpegBlocks] = 1;
          break;
        }
      }
      if (m_pCamPixelsInJpegBlocks[m_iCamCurrentJpegBlocks] == 1) break;
    }
  }
  if (m_pCamPixelsInJpegBlocks == NULL || m_pCamPixelsInJpegBlocks[m_iCamCurrentJpegBlocks] == 1) {
    for (size_t y1 = y; y1 < yEnd; y1++) {
      for (size_t x1 = 0; x1 < w; x1++) {
        xInLine = x + x1;
        xBytePos = x1 * 3;
        iBlue = data[xBytePos + 2];
        iGreen = data[xBytePos + 1];
        iRed = data[xBytePos];
        iAvarage = iRed + iGreen + iBlue;
        if (TestBit(piCamPoints, BitIndexEx(LEDSINGLECOLOR, y1, xInLine, Getlev1size(), Getlev2size())))
          camCalcColorsProcessPixel(xInLine, y1, &iRed, &iGreen, &iBlue, iAvarage);
        else {
          IncrementSkipPixels();
          AddAvrOutside(iAvarage);
        }
      }
      data += outputWidthBytes;
      _taskYIELD();
    }
  } else {
    for (size_t y1 = y; y1 < yEnd; y1++) {
      for (size_t x1 = 0; x1 < w; x1++) {
        xBytePos = x1 * 3;
        IncrementSkipPixels();
        AddAvrOutside(data[xBytePos + 2] + data[xBytePos + 1] + data[xBytePos]);
      }
      data += outputWidthBytes;
    }
  }
  m_iCamCurrentJpegBlocks++;
  return true;
}

CamProcessor::CamProcessor() {
  // All pointers that setters may free() must be NULL before any setter is called
  m_pRectAreas = NULL;
  m_pLedSmoothFrames = NULL;
  m_pBlackDetected = NULL;
  m_pCamPixelsInJpegBlocks = NULL;
  m_piCamPoints = NULL;
  m_pVoidCamleds = NULL;
  m_pshPixels = NULL;
  m_pshBlackPixels = NULL;
  m_frameWidth = 0;
  m_frameHeight = 0;
  m_realframeWidth = 0;
  m_realframeHeight = 0;
  m_bCameraPointsUpdated = true;
  m_aCameraPoint[0].x = 25;
  m_aCameraPoint[0].y = 25;
  m_aCameraPoint[1].x = 75;
  m_aCameraPoint[1].y = 25;
  m_aCameraPoint[2].x = 25;
  m_aCameraPoint[2].y = 75;
  m_aCameraPoint[3].x = 75;
  m_aCameraPoint[3].y = 75;
  m_iColorBoostMaxPercent = 0;
  m_bCamMode = LED_MODE_CAM_DETAIL;
  m_bCamDetailLevel = (psramFound() ? CAM_DEFAULT_CALC_LEDS_SIDE : CAM_MIN_CALC_LEDS_SIDE);
  SetCamDetailLevel(m_bCamDetailLevel, false);
  m_bCamDetailSmooth = 0;
  m_flCamInnerHori = CAM_MAX_CALC_INNER_IMAGE_HORI;
  m_flCamInnerVert = CAM_MAX_CALC_INNER_IMAGE_VERT;
  m_bCamSmoothEnable = 1;
  m_bCamSmoothMaxFrames = CAM_SMOOTH_FRAMES_DEFAULT;
  m_bCamSmoothMaxDiff = CAM_SMOOTH_FRAMES_DIFF_DEFAULT;
  memset(m_bCamSmoothCurrentMaxStep, m_bCamSmoothMaxFrames, MAX_LEDS * sizeof(byte));
  memset(m_bCamSmoothFrameStep, m_bCamSmoothMaxDiff / m_bCamSmoothMaxFrames, MAX_LEDS * sizeof(byte));
  m_bCurrentCamFps = 0;
  m_bCurrentCamFramesCtr = 0;
  m_iBlackFrames = 0;
  m_iBlackSeconds = 0;
  m_lAdptLastUpdate = 0;
  m_lGainLastUpdateUp = 0;
  m_lGainLastUpdateDown = 0;
  m_lev1size = 0;
  m_lev2size = 0;
  m_iTopFirstLine = 0;
  m_iBottomLastLine = 0;
  m_iLeftFirstPixel = 0;
  m_iRightLastPixel = 0;
  m_bInnerRectangle = false;
  m_iInnerTopLastLine = -1;
  m_iInnerBottomFirstLine = -1;
  m_iInnerLeftLastPixel = -1;
  m_iInnerRightFirstPixel = -1;
  m_iBlackDetectedSize = 0;
  m_iCamCurrentJpegBlocks = 0;
  m_bCamCalcJpegBlocks = true;
  m_bSectorsMatchCount = 0;
  memset(m_bSectorsMatch, 0, CAM_MAX_CALC_LEDS_ALL * sizeof(byte));
  m_iSkipPixels = 0;
  m_iAvrOutsideCtr = 0;
  m_iAvrInsideCtr = 0;
  m_iAvrOutside = 0;
  m_iAvrInside = 0;
  m_bWhiteActivationTreshold = WHITEACTTHRESHOLD_DEFAULT;
  m_bBeforeWhiteActivationTreshold = WHITEACTTHRESHOLD_DEFAULT - CAM_ADPT_BEFORE_ACT;
  m_bWhiteAdaptivePercent = CAM_WHITE_AEC_CHANGEPERC;
  m_iWhiteAdaptiveChangeDelay = CAM_ADPT_CHANGE_DELAY;
  m_bCamAdaptiveGainDown = CAM_ADPT_GAIN_DOWN;
  m_bCamAdaptiveGainUp = CAM_ADPT_GAIN_UP;
  m_bCamAdaptiveGainMax = CAM_ADPT_GAIN_MAX;
  memset(&m_MainRectArea, 0, sizeof(m_MainRectArea));
  m_MainInnerRectArea.topleft.x = -1;
  m_MainInnerRectArea.topleft.y = -1;
  m_MainInnerRectArea.topright.x = -1;
  m_MainInnerRectArea.topright.y = -1;
  m_MainInnerRectArea.bottomleft.x = -1;
  m_MainInnerRectArea.bottomleft.y = -1;
  m_MainInnerRectArea.bottomright.x = -1;
  m_MainInnerRectArea.bottomright.y = -1;
  m_cam_mutex = xSemaphoreCreateMutex();
}

CamProcessor::~CamProcessor() {
  if (m_pLedSmoothFrames) {
    free(m_pLedSmoothFrames);
    m_pLedSmoothFrames = NULL;
  }
  if (m_pBlackDetected) {
    free(m_pBlackDetected);
    m_pBlackDetected = NULL;
  }
  if (m_pCamPixelsInJpegBlocks) {
    free(m_pCamPixelsInJpegBlocks);
    m_pCamPixelsInJpegBlocks = NULL;
  }
  if (m_piCamPoints) {
    free(m_piCamPoints);
    m_piCamPoints = NULL;
  }
  if (m_pRectAreas) {
    free(m_pRectAreas);
    m_pRectAreas = NULL;
  }
  if (m_cam_mutex) {
    vSemaphoreDelete(m_cam_mutex);
    m_cam_mutex = NULL;
  }
}

short CamProcessor::getCameraPointX(byte index) {
  if (index >= 1 && index <= 4) return m_aCameraPoint[index].x;
  return 0;
}
short CamProcessor::getCameraPointY(byte index) {
  if (index >= 1 && index <= 4) return m_aCameraPoint[index].y;
  return 0;
}

void CamProcessor::SetColorBoostMaxPercent(int iNew, bool bEpprom) {
  (void)bEpprom;
  if (iNew >= 0 && iNew <= 50) {
    m_iColorBoostMaxPercent = iNew;
    _DBGPRINTF(PSTR("CamProcessor: SetColorBoostMaxPercent to %d\n"), iNew);
  }
}

void CamProcessor::SetCamBlackThresholdSmoothing(byte bNew, bool bEpprom) {
  if (bNew >= 0 && bNew <= BLACKDETECT_MAX) {
    m_bCamBlackThresholdSmoothing = bNew;
    if (bEpprom)
      global::m_pEepromHelper->SetSettingByte("CamBlackSmooth", bNew);
    if (m_pBlackDetected != NULL) {
      free(m_pBlackDetected);
      m_pBlackDetected = NULL;
    }
    if (m_bCamBlackThresholdSmoothing > 0) {
      m_iBlackDetectedSize = ((CAM_MAX_CALC_LEDS_ALL * (m_bCamBlackThresholdSmoothing + 1)) / 8);
      m_pBlackDetected = (int*)_malloc_psram(m_iBlackDetectedSize);
      if (m_pBlackDetected) memset(m_pBlackDetected, 0, m_iBlackDetectedSize);
    }
  }
}

void CamProcessor::SetCamMode(byte bNew, bool bEpprom) {
  m_bCamMode = bNew;
  m_bCameraPointsUpdated = true;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte(CAM_MODE_START, bNew);
}

void CamProcessor::SetCamDetailLevel(byte bNew, bool bEpprom) {
  if (!psramFound() && bNew > CAM_DEFAULT_CALC_LEDS_SIDE) {
    global::errprintf_P(PSTR("CamProcessor: SetCamDetailLevel PSRAM disabled, MAX limit to %d\n"), CAM_DEFAULT_CALC_LEDS_SIDE);
    bNew = CAM_MIN_CALC_LEDS_SIDE;
  }
  m_bCamDetailLevel = bNew;
  m_bCamDetailTop = m_bCamDetailLevel * 0 + 1;
  m_bCamDetailRight = m_bCamDetailLevel * 1 + 1;
  m_bCamDetailBottom = m_bCamDetailLevel * 2 + 1;
  m_bCamDetailLeft = m_bCamDetailLevel * 3 + 1;
  m_bCameraPointsUpdated = true;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte(CAM_DETAILLEV_START, bNew);
  int iDetailLevel = CAM_SECTOR_COUNT(m_bCamDetailLevel);
  rectanglepoints* pNew = (rectanglepoints*)_malloc_psram(iDetailLevel * sizeof(rectanglepoints));
  if (pNew) {
    if (m_pRectAreas) free(m_pRectAreas);
    m_pRectAreas = pNew;
  }
}

void CamProcessor::SetCamInnerHori(float flNew, bool bEpprom) {
  if (flNew > 0.50f)
    m_flCamInnerHori = 0.50f;
  else
    m_flCamInnerHori = flNew;
  m_bCameraPointsUpdated = true;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingFloat("CamInnerHori", flNew);
}

void CamProcessor::SetCamInnerVert(float flNew, bool bEpprom) {
  if (flNew > 0.50f)
    m_flCamInnerVert = 0.50f;
  else
    m_flCamInnerVert = flNew;
  m_bCameraPointsUpdated = true;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingFloat("CamInnerVert", flNew);
}

void CamProcessor::SetCamDetailSmooth(byte bNew, bool bEpprom) {
  m_bCamDetailSmooth = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("CamDetailSmooth", bNew);
}

void CamProcessor::SetCamSmooth(byte bNew, bool bEpprom) {
  m_bCamSmooth = bNew;
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("CamSmoothMode", bNew);
  if (m_bCamSmooth == 0) {
    SetCamSmoothEnable(0, false);
  } else if (m_bCamSmooth == 1) {
    SetCamSmoothEnable(1, false);
    SetCamSmoothMaxFrames(2, false);
    SetCamSmoothMaxDiff(CAM_SMOOTH_FRAMES_DIFF_DEFAULT, false);
  } else if (m_bCamSmooth == 2) {
    SetCamSmoothEnable(1, false);
    SetCamSmoothMaxFrames(4, false);
    SetCamSmoothMaxDiff(CAM_SMOOTH_FRAMES_DIFF_DEFAULT, false);
  } else if (m_bCamSmooth == 3) {
    SetCamSmoothEnable(1, false);
    SetCamSmoothMaxFrames(8, false);
    SetCamSmoothMaxDiff(CAM_SMOOTH_FRAMES_DIFF_DEFAULT, false);
  } else {
    SetCamSmoothEnable(global::m_pEepromHelper->GetSettingByte("CamSmoothEnable", 1), false);
    SetCamSmoothMaxFrames(global::m_pEepromHelper->GetSettingByte("CamSmoothMaxF", CAM_SMOOTH_FRAMES_DEFAULT), false);
    SetCamSmoothMaxDiff(global::m_pEepromHelper->GetSettingByte("CamSmoothMaxD", CAM_SMOOTH_FRAMES_DIFF_DEFAULT), false);
  }
}

void CamProcessor::SetCamSmoothEnable(byte bNew, bool bEpprom) {
  m_bCamSmoothEnable = bNew;
  if (m_pLedSmoothFrames != NULL) {
    free(m_pLedSmoothFrames);
    m_pLedSmoothFrames = NULL;
  }
  if (bEpprom)
    global::m_pEepromHelper->SetSettingByte("CamSmoothEnable", bNew);
  if (m_bCamSmoothEnable > 0) {
    int iArraySize = (MAX_LEDS * sizeof(struct CRGB) * (CAM_SMOOTH_FRAMES_MAX + 1));
    m_pLedSmoothFrames = _malloc_psram(iArraySize);
    if (m_pLedSmoothFrames) memset(m_pLedSmoothFrames, 0, iArraySize);
  }
}

void CamProcessor::SetCamSmoothMaxFrames(byte bNew, bool bEpprom) {
  if (bNew >= CAM_SMOOTH_FRAMES_MIN && bNew <= CAM_SMOOTH_FRAMES_MAX) {
    m_bCamSmoothMaxFrames = bNew;
    if (bEpprom)
      global::m_pEepromHelper->SetSettingByte("CamSmoothMaxF", bNew);
  }
}

void CamProcessor::SetCamSmoothMaxDiff(byte bNew, bool bEpprom) {
  if (bNew >= CAM_SMOOTH_FRAMES_MIN_DIFF && bNew <= CAM_SMOOTH_FRAMES_MAX_DIFF) {
    m_bCamSmoothMaxDiff = bNew;
    if (bEpprom)
      global::m_pEepromHelper->SetSettingByte("CamSmoothMaxD", bNew);
  }
}

void CamProcessor::setCameraPoint(byte index, short x, short y, bool bEpprom) {
  if (index >= 1 && index <= 4) {
    m_aCameraPoint[index].x = x;
    m_aCameraPoint[index].y = y;
    m_bCameraPointsUpdated = true;
    if (bEpprom) {
      int iCombined = (m_aCameraPoint[index].x << 16) | ((m_aCameraPoint[index].y) & 0xffff);
      int iEppromStart = CAM_POINT_1_START;
      if (index == 2) iEppromStart = CAM_POINT_2_START;
      else if (index == 3) iEppromStart = CAM_POINT_3_START;
      else if (index == 4) iEppromStart = CAM_POINT_4_START;
      global::m_pEepromHelper->SetSettingInt(iEppromStart, iCombined);
    }
  }
}

bool CamProcessor::camMutexLock() {
  return xSemaphoreTake(m_cam_mutex, (TickType_t)1000);
}

void CamProcessor::camMutexRelease() {
  xSemaphoreGive(m_cam_mutex);
}

void CamProcessor::camSetJpegProperties(int iFrameWidth, int* piJpgScale, int* piMulti, int* piFinalWidth, int* piFinalHeight) {
  int iQuality = (global::GetCamHighQuality() == 0 ? 1 : 2);
  if (iFrameWidth > 800) {
    *piJpgScale = (global::GetCamHighQuality() == 0 ? JPG_SCALE_8X : JPG_SCALE_4X);
    *piMulti = 8 / iQuality;
    *piFinalWidth = *piFinalWidth / (8 / iQuality);
    *piFinalHeight = *piFinalHeight / (8 / iQuality);
  } else if (iFrameWidth >= 640) {
    *piJpgScale = (global::GetCamHighQuality() == 0 ? JPG_SCALE_4X : JPG_SCALE_2X);
    *piMulti = 4 / iQuality;
    *piFinalWidth = *piFinalWidth / (4 / iQuality);
    *piFinalHeight = *piFinalHeight / (4 / iQuality);
  } else if (iFrameWidth >= 160) {
    *piJpgScale = (global::GetCamHighQuality() == 0 ? JPG_SCALE_2X : JPG_SCALE_NONE);
    *piMulti = 2 / iQuality;
    *piFinalWidth = *piFinalWidth / (2 / iQuality);
    *piFinalHeight = *piFinalHeight / (2 / iQuality);
  } else {
    *piJpgScale = JPG_SCALE_NONE;
  }
}

bool CamProcessor::camJpg2rgb888(const uint8_t* src, size_t src_len, uint8_t* out, int scale) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)scale;
  return fmt2rgb888(src, src_len, PIXFORMAT_JPEG, out);
#else
  rgb_jpg_decoder jpeg;
  jpeg.width = 0;
  jpeg.height = 0;
  jpeg.input = src;
  jpeg.output = out;
  jpeg.data_offset = 0;
  if (esp_jpg_decode(src_len, (jpg_scale_t)scale, _jpg_read, _rgb_write, (void*)&jpeg) != ESP_OK)
    return false;
  return true;
#endif
}

bool CamProcessor::camJpg2rgbDirectProcess(const uint8_t* src, size_t src_len, int scale) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)src;
  (void)src_len;
  (void)scale;
  return false;
#else
  jpeg_direct_arg a;
  a.jpeg.width = 0;
  a.jpeg.height = 0;
  a.jpeg.input = src;
  a.jpeg.output = NULL;
  a.jpeg.data_offset = 0;
  a.pProc = this;
  if (esp_jpg_decode(src_len, (jpg_scale_t)scale, _jpg_read, _rgb_direct, (void*)&a) != ESP_OK)
    return false;
  return true;
#endif
}

void CamProcessor::PreProcessCameraSmoothing(byte bIndex, int* piRed, int* piGreen, int* piBlue, void* pVoidLedFrames) {
  CRGB* pLedFrames = (CRGB*)pVoidLedFrames;
  if (pLedFrames == NULL) return;
  int iFirstIdx = bIndex;
  byte bSmoothingSteps = 0;
  int iRed = *piRed;
  int iGreen = *piGreen;
  int iBlue = *piBlue;
  m_bCamSmoothFrameStep[bIndex] = (m_bCamSmoothCurrentMaxStep[bIndex] > 0 ? m_bCamSmoothMaxDiff / m_bCamSmoothCurrentMaxStep[bIndex] : m_bCamSmoothMaxDiff);
  int iDiff = (iRed > (pLedFrames + iFirstIdx)->r ? iRed - (pLedFrames + iFirstIdx)->r : (pLedFrames + iFirstIdx)->r - iRed);
  iDiff += (iGreen > (pLedFrames + iFirstIdx)->g ? iGreen - (pLedFrames + iFirstIdx)->g : (pLedFrames + iFirstIdx)->g - iGreen);
  iDiff += (iBlue > (pLedFrames + iFirstIdx)->b ? iBlue - (pLedFrames + iFirstIdx)->b : (pLedFrames + iFirstIdx)->b - iBlue);
  if (global::GetCamAdaptiveGaincontrol() == 1 &&
      ((GetGainLastUpdateUp() + m_iWhiteAdaptiveChangeDelay) > millis() ||
       (GetGainLastUpdateDown() + m_iWhiteAdaptiveChangeDelay) > millis())) {
    m_bCamSmoothCurrentMaxStep[bIndex] = m_bCamSmoothMaxFrames;
    bSmoothingSteps = m_bCamSmoothMaxFrames;
  } else {
    bSmoothingSteps = (iDiff > 0 ? (iDiff < m_bCamSmoothMaxDiff ? m_bCamSmoothCurrentMaxStep[bIndex] - (iDiff / m_bCamSmoothFrameStep[bIndex]) : 0) : m_bCamSmoothCurrentMaxStep[bIndex]);
  }
  if (bSmoothingSteps > 0) {
    for (byte z = 0; z < bSmoothingSteps; z++) {
      int iAddIdx = z * MAX_LEDS;
      *piRed += (pLedFrames + iAddIdx + bIndex)->r;
      *piGreen += (pLedFrames + iAddIdx + bIndex)->g;
      *piBlue += (pLedFrames + iAddIdx + bIndex)->b;
    }
    *piRed = *piRed / (bSmoothingSteps + 1);
    *piGreen = *piGreen / (bSmoothingSteps + 1);
    *piBlue = *piBlue / (bSmoothingSteps + 1);
  }
  if (bSmoothingSteps == 0)
    m_bCamSmoothCurrentMaxStep[bIndex] = 1;
  else if (m_bCamSmoothCurrentMaxStep[bIndex] < m_bCamSmoothMaxFrames)
    m_bCamSmoothCurrentMaxStep[bIndex]++;
  else if (bSmoothingSteps < m_bCamSmoothCurrentMaxStep[bIndex])
    m_bCamSmoothCurrentMaxStep[bIndex] = bSmoothingSteps;
}

void CamProcessor::PostProcessCameraSmoothing(void* pVoidLedFrames, void* pVoidLedsNext, int numLeds) {
  CRGB* pLedFrames = (CRGB*)pVoidLedFrames;
  CRGB* pLedsNext = (CRGB*)pVoidLedsNext;
  if (pLedFrames == NULL) return;
  int iMaxLedsCrgbSize = MAX_LEDS * sizeof(struct CRGB);
  memmove((byte*)pLedFrames + iMaxLedsCrgbSize, pLedFrames, (CAM_SMOOTH_FRAMES_MAX - 1) * iMaxLedsCrgbSize);
  memset(pLedFrames, 0, iMaxLedsCrgbSize);
  for (int l = 0; l < numLeds; l++) {
    (pLedFrames + l)->r = (pLedsNext + l)->r;
    (pLedFrames + l)->g = (pLedsNext + l)->g;
    (pLedFrames + l)->b = (pLedsNext + l)->b;
  }
}

float CamProcessor::camCalcColorsArea(int x1, int y1, int x2, int y2, int x3, int y3) {
  return abs((x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2)) / 2.0);
}

bool CamProcessor::camCalcColorsIsPixelInside(float A, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, int x, int y) {
  if (((y >= y4 || y >= y1) && (y <= y3 || y <= y2)) &&
      ((x >= x4 || x >= x3) && (x <= x1 || x <= x2))) {
    float A1 = camCalcColorsArea(x, y, x1, y1, x2, y2);
    float A2 = camCalcColorsArea(x, y, x2, y2, x3, y3);
    float A3 = camCalcColorsArea(x, y, x3, y3, x4, y4);
    float A4 = camCalcColorsArea(x, y, x1, y1, x4, y4);
    return (A == A1 + A2 + A3 + A4);
  }
  return false;
}

void CamProcessor::camCalcColorsInitGlobalVariables() {
  m_iTopFirstLine = (m_MainRectArea.topleft.y > m_MainRectArea.topright.y ? m_MainRectArea.topright.y : m_MainRectArea.topleft.y);
  m_iBottomLastLine = (m_MainRectArea.bottomleft.y > m_MainRectArea.bottomright.y ? m_MainRectArea.bottomleft.y : m_MainRectArea.bottomright.y);
  m_iLeftFirstPixel = (m_MainRectArea.topleft.x > m_MainRectArea.bottomleft.x ? m_MainRectArea.bottomleft.x : m_MainRectArea.topleft.x);
  m_iRightLastPixel = (m_MainRectArea.topright.x < m_MainRectArea.bottomright.x ? m_MainRectArea.bottomright.x : m_MainRectArea.topright.x);
  m_iInnerTopLastLine = (m_MainInnerRectArea.topleft.y != -1 ? (m_MainInnerRectArea.topleft.y > m_MainInnerRectArea.topright.y ? m_MainInnerRectArea.topleft.y : m_MainInnerRectArea.topright.y) : -1);
  m_iInnerBottomFirstLine = (m_MainInnerRectArea.bottomleft.y != -1 ? (m_MainInnerRectArea.bottomleft.y > m_MainInnerRectArea.bottomright.y ? m_MainInnerRectArea.bottomright.y : m_MainInnerRectArea.bottomleft.y) : -1);
  m_iInnerLeftLastPixel = (m_MainInnerRectArea.topleft.x != -1 ? (m_MainInnerRectArea.topleft.x > m_MainInnerRectArea.bottomleft.x ? m_MainInnerRectArea.topleft.x : m_MainInnerRectArea.bottomleft.x) : -1);
  m_iInnerRightFirstPixel = (m_MainInnerRectArea.topright.x != -1 ? (m_MainInnerRectArea.topright.x < m_MainInnerRectArea.bottomright.x ? m_MainInnerRectArea.topright.x : m_MainInnerRectArea.bottomright.x) : -1);
  m_bInnerRectangle = (m_iInnerTopLastLine != -1 && m_iInnerBottomFirstLine != -1 && m_iInnerLeftLastPixel != -1 && m_iInnerRightFirstPixel != -1);
}

void CamProcessor::camCalcColorsInitFrameVariables() {
  m_iWhiteSectors = 0;
  m_iBelowWhiteSectors = 0;
  m_iBlackSectors = 0;
  m_iBlackPixels = 0;
  m_iWhitePixels = 0;
  m_iTotalPixels = 0;
  m_iSkipPixels = 0;
  m_iAvrOutsideCtr = 0;
  m_iAvrInsideCtr = 0;
  m_bSectors = 1;
  m_bSectorSteps = 1;
  if (GetCamMode() != LED_MODE_CAM_SINGLE) {
    m_bSectorStart = 1;
    m_bSectors = CAM_SECTOR_COUNT(GetCamDetailLevel());
    if (GetCamMode() == LED_MODE_CAM_BAR)
      m_bSectorSteps = GetCamDetailLevel();
  } else {
    m_bSectorStart = 0;
  }
  m_bWhiteMinimumPercent = global::GetCamWhiteMinimumAmount();
  m_bSectorsMatchCount = 0;
  memset(m_bSectorsMatch, 0, CAM_MAX_CALC_LEDS_ALL * sizeof(byte));
  m_bColCorRed = global::GetCamColorCorRed();
  m_bColCorGreen = global::GetCamColorCorGreen();
  m_bColCorBlue = global::GetCamColorCorBlue();
  m_bColActivationTreshold = global::GetCamActivationThreshold();
  m_bColBlackRemoveTreshold = global::GetCamBlackRemoveThreshold();
  m_bWhiteActivationTreshold = global::GetCamWhiteActivationTreshold();
  m_bWhiteAdaptivePercent = (byte)global::GetCamAdaptiveChangePercent();
  m_iWhiteAdaptiveChangeDelay = (byte)global::GetCamAdaptiveChangeDelay();
  m_bBeforeWhiteActivationTreshold = m_bWhiteActivationTreshold - global::GetCamBeforeWhiteActivationTreshold();
  m_bColTresholdHit = false;
  m_bCamAdaptiveGainDown = global::GetCamAdaptiveGainDown();
  m_bCamAdaptiveGainUp = global::GetCamAdaptiveGainUp();
  m_bCamAdaptiveGainMax = global::GetCamAdaptiveGainMax();
  for (int i = 0; i < MAX_LEDS; i++) {
    m_iaBlue[i] = 0;
    m_iaRed[i] = 0;
    m_iaGreen[i] = 0;
  }
  if (m_bColActivationTreshold > 0 && m_pBlackDetected != NULL) {
    int iBlackDetectedSingleSize = CAM_MAX_CALC_LEDS_ALL / 8;
    memmove((byte*)m_pBlackDetected + iBlackDetectedSingleSize, m_pBlackDetected, m_iBlackDetectedSize - iBlackDetectedSingleSize);
  }
  m_iCamCurrentJpegBlocks = 0;
}

void CamProcessor::camCalcColorsProcessPixel(int x, int y, int* piRed, int* piGreen, int* piBlue, int iAvarage) {
  int* piCamPoints = m_piCamPoints;
  if (m_bColCorRed != 100) {
    *piRed = (*piRed * m_bColCorRed) / 100;
    *piRed = (*piRed < 255 ? (*piRed > 0 ? *piRed : 0) : 255);
  }
  if (m_bColCorGreen != 100) {
    *piGreen = (*piGreen * m_bColCorGreen) / 100;
    *piGreen = (*piGreen < 255 ? (*piGreen > 0 ? *piGreen : 0) : 255);
  }
  if (m_bColCorBlue != 100) {
    *piBlue = (*piBlue * m_bColCorBlue) / 100;
    *piBlue = (*piBlue < 255 ? (*piBlue > 0 ? *piBlue : 0) : 255);
  }
  if (m_bColBlackRemoveTreshold == 0 ||
      (*piRed >= m_bColBlackRemoveTreshold || *piGreen >= m_bColBlackRemoveTreshold || *piBlue >= m_bColBlackRemoveTreshold)) {
    bool bSectorHit = false;
    int iBitIndex = BitIndexEx(m_bSectorStart, y, x, Getlev1size(), Getlev2size());
    for (byte n = m_bSectorStart; n < m_bSectors; n += m_bSectorSteps) {
      if (TestBit(piCamPoints, iBitIndex)) {
        m_iaRed[n] += *piRed;
        m_iaGreen[n] += *piGreen;
        m_iaBlue[n] += *piBlue;
        m_pshPixels[n]++;
        if (bSectorHit) break;
        bSectorHit = true;
        if (global::GetCamAdaptiveExposurecontrol() == 1 && m_bWhiteActivationTreshold > 0 &&
            (*piRed >= m_bWhiteActivationTreshold || *piGreen >= m_bWhiteActivationTreshold || *piBlue >= m_bWhiteActivationTreshold))
          m_iWhitePixels++;
      }
      iBitIndex += Getlev1size();
    }
    m_iTotalPixels++;
    m_iAvrInsideCtr += iAvarage;
  } else {
    m_iBlackPixels++;
    if (GetCamMode() != LED_MODE_CAM_SINGLE) {
      int iBitIndex = BitIndexEx(0, y, x, Getlev1size(), Getlev2size());
      for (int n = m_bSectorStart; n < m_bSectors; n += m_bSectorSteps) {
        if (TestBit(piCamPoints, iBitIndex)) {
          m_pshBlackPixels[n]++;
          break;
        }
        iBitIndex += Getlev1size();
      }
    }
  }
}

void CamProcessor::camCalcColorsFromCamPoints(uint8_t* uiData, int iFormat) {
  uint8_t hb, lb;
  int iBlue, iRed, iGreen, iAvarage;
  camCalcColorsInitFrameVariables();
  for (int v = 0; v < m_frameHeight; v++) {
    for (int h = 0; h < m_frameWidth; h++) {
      if (iFormat == PIXFORMAT_RGB565) {
        hb = *uiData++;
        lb = *uiData++;
        iRed = (hb & 0xF8);
        iGreen = ((hb & 0x07) << 5 | (lb & 0xE0) >> 3);
        iBlue = (lb & 0x1F) << 3;
      } else {
        iBlue = *uiData++;
        iGreen = *uiData++;
        iRed = *uiData++;
      }
      iAvarage = iRed + iGreen + iBlue;
      if ((v >= GetTopFirstLine() && v <= GetBottomLastLine() && h >= GetLeftFirstPixel() && h <= GetRightLastPixel()) &&
          (!GetInnerRectangle() || !(v >= GetInnerTopLastLine() && v <= GetInnerBottomFirstLine() && h >= GetInnerLeftLastPixel() && h <= GetInnerRightFirstPixel())))
        camCalcColorsProcessPixel(h, v, &iRed, &iGreen, &iBlue, iAvarage);
      else {
        m_iSkipPixels++;
        m_iAvrOutsideCtr += iAvarage;
      }
    }
  }
  camCalcColorsFinishFrameVariables();
}

void CamProcessor::camCalcColorsFinishFrameVariables() {
  CRGB* pCamleds = (CRGB*)m_pVoidCamleds;
  rectanglepoints* rectAreas = m_pRectAreas;
  if (!pCamleds || !rectAreas) return;
  if (m_bCamCalcJpegBlocks) {
    if (m_pCamPixelsInJpegBlocks == NULL) {
      m_pCamPixelsInJpegBlocks = (byte*)_malloc_psram(m_iCamCurrentJpegBlocks);
      if (m_pCamPixelsInJpegBlocks) memset(m_pCamPixelsInJpegBlocks, 0, m_iCamCurrentJpegBlocks);
    } else
      m_bCamCalcJpegBlocks = false;
  }
  if (GetCamMode() == LED_MODE_CAM_SINGLE) {
    if (m_pshPixels[0] > 0 && m_pshPixels[0] >= rectAreas[0].shPixelMinCount) {
      pCamleds[0].r = (uint8_t)(m_iaRed[0] / m_pshPixels[0]);
      pCamleds[0].g = (uint8_t)(m_iaGreen[0] / m_pshPixels[0]);
      pCamleds[0].b = (uint8_t)(m_iaBlue[0] / m_pshPixels[0]);
    } else {
      pCamleds[0].r = 0;
      pCamleds[0].g = 0;
      pCamleds[0].b = 0;
    }
  } else {
    for (byte i = m_bSectorStart; i < m_bSectors; i += m_bSectorSteps) {
      if (m_pshPixels[i] > 0 && m_pshPixels[i] >= rectAreas[i].shPixelMinCount) {
        pCamleds[i].r = (uint8_t)(m_iaRed[i] / m_pshPixels[i]);
        pCamleds[i].g = (uint8_t)(m_iaGreen[i] / m_pshPixels[i]);
        pCamleds[i].b = (uint8_t)(m_iaBlue[i] / m_pshPixels[i]);
      } else {
        pCamleds[i].r = 0;
        pCamleds[i].g = 0;
        pCamleds[i].b = 0;
      }
    }
  }
  m_iAvrOutside = (m_iSkipPixels > 0 ? m_iAvrOutsideCtr / (m_iSkipPixels * 3) : 0);
  m_iAvrInside = (m_iTotalPixels > 0 ? m_iAvrInsideCtr / (m_iTotalPixels * 3) : 0);
  if (global::GetCamAdaptiveExposurecontrol() == 1) {
    byte bDetailSectors = (GetCamDetailLevel() * 4);
    for (byte i = m_bSectorStart; i < m_bSectors; i += m_bSectorSteps) {
      if (pCamleds[i].r >= m_bWhiteActivationTreshold || pCamleds[i].g >= m_bWhiteActivationTreshold || pCamleds[i].b >= m_bWhiteActivationTreshold)
        m_iWhiteSectors++;
      else if (pCamleds[i].r < m_bBeforeWhiteActivationTreshold && pCamleds[i].g < m_bBeforeWhiteActivationTreshold && pCamleds[i].b < m_bBeforeWhiteActivationTreshold)
        m_iBelowWhiteSectors++;
    }
    if (global::GetCamAecValue() > global::m_iCamAecMaxValue)
      global::SetCamAecValue(global::m_iCamAecMaxValue, false);
    else if (global::GetCamAecValue() < global::m_iCamAecMinValue)
      global::SetCamAecValue(global::m_iCamAecMinValue, false);
    else {
      int iWhitePixelsPercent = (m_iTotalPixels > 0 ? (m_iWhitePixels * 100) / m_iTotalPixels : 0);
      int iNewAecDiffValue = (global::m_iCamAecValue * m_bWhiteAdaptivePercent) / 100;
      if (m_iTotalPixels > 0 && m_bWhiteMinimumPercent > 0 && iWhitePixelsPercent >= m_bWhiteMinimumPercent) {
        int iNewAecValue = global::m_iCamAecValue - (iNewAecDiffValue * iWhitePixelsPercent / m_bWhiteMinimumPercent);
        iNewAecValue = (iNewAecValue > global::m_iCamAecMinValue ? iNewAecValue : global::m_iCamAecMinValue);
        if (global::GetCamAdaptiveGaincontrol() == 1 && global::m_iCamAecValue <= global::m_iCamAecMinValue &&
            (GetGainLastUpdateUp() + m_iWhiteAdaptiveChangeDelay) < millis() && global::m_iCamAgcGain > CAM_ADPT_GAIN_MIN) {
          global::SetCamAgcGain(--global::m_iCamAgcGain, false);
          global::SetCamAecValue(global::m_iCamAecMaxValue, false);
          SetAdptLastUpdate();
          SetGainLastUpdateDown();
        } else if (iNewAecValue < global::m_iCamAecValue && iNewAecValue >= global::m_iCamAecMinValue) {
          global::SetCamAecValue(iNewAecValue, false);
          SetAdptLastUpdate();
        }
      } else if (m_iBelowWhiteSectors >= bDetailSectors && (GetAdptLastUpdate() + m_iWhiteAdaptiveChangeDelay) < millis()) {
        int iNewAecValue = global::m_iCamAecValue + iNewAecDiffValue;
        iNewAecValue = (iNewAecValue < global::m_iCamAecMaxValue ? iNewAecValue : global::m_iCamAecMaxValue);
        if (global::GetCamAdaptiveGaincontrol() == 1 && global::m_iCamAecValue >= global::m_iCamAecMaxValue &&
            (GetGainLastUpdateDown() + m_iWhiteAdaptiveChangeDelay) < millis() && global::m_iCamAgcGain < m_bCamAdaptiveGainMax) {
          global::SetCamAgcGain(++global::m_iCamAgcGain, false);
          global::SetCamAecValue(global::m_iCamAecMaxValue / 2, false);
          SetGainLastUpdateUp();
        } else if (iNewAecValue > global::m_iCamAecValue && iNewAecValue <= global::m_iCamAecMaxValue) {
          global::SetCamAecValue(iNewAecValue, false);
        }
      }
    }
  }
  if (m_bColActivationTreshold > 0) {
    for (byte i = m_bSectorStart; i < m_bSectors; i += m_bSectorSteps) {
      m_bColTresholdHit = false;
      if (pCamleds[i].r <= m_bColActivationTreshold && pCamleds[i].g <= m_bColActivationTreshold && pCamleds[i].b <= m_bColActivationTreshold) {
        m_bColTresholdHit = true;
        m_iBlackSectors++;
      }
      if (m_pBlackDetected != NULL) {
        if (m_bColTresholdHit) {
          SetBit(m_pBlackDetected, BitIndex(1, i, CAM_MAX_CALC_LEDS_ALL));
          if (!TestBit(m_pBlackDetected, i)) {
            for (int z = 1; z <= m_bCamBlackThresholdSmoothing; z++) {
              if (!TestBit(m_pBlackDetected, BitIndex(z, i, CAM_MAX_CALC_LEDS_ALL))) {
                m_bColTresholdHit = false;
                break;
              }
            }
          }
          if (m_bColTresholdHit) SetBit(m_pBlackDetected, i);
        } else {
          ClearBit(m_pBlackDetected, BitIndex(1, i, CAM_MAX_CALC_LEDS_ALL));
          if (TestBit(m_pBlackDetected, i)) {
            for (int z = 1; z <= m_bCamBlackThresholdSmoothing; z++) {
              if (TestBit(m_pBlackDetected, BitIndex(z, i, CAM_MAX_CALC_LEDS_ALL))) {
                m_bColTresholdHit = true;
                break;
              }
            }
          }
          if (!m_bColTresholdHit) ClearBit(m_pBlackDetected, i);
        }
      }
      if (m_bColTresholdHit && pCamleds[i].r <= m_bColBlackRemoveTreshold && pCamleds[i].g <= m_bColBlackRemoveTreshold && pCamleds[i].b <= m_bColBlackRemoveTreshold) {
        pCamleds[i].r = 0;
        pCamleds[i].g = 0;
        pCamleds[i].b = 0;
      }
    }
  }
  if (m_iAvrInside <= m_bColActivationTreshold)
    IncrementBlackFrames();
}

int* CamProcessor::camCalcCamPointAreas(int frameWidth, int frameHeight) {
  int iDetailLevel = CAM_SECTOR_COUNT(m_bCamDetailLevel);
  int iArraySize = (iDetailLevel * frameHeight * frameWidth) / 8;
  int* piCamPoints = (int*)_malloc_psram(iArraySize);
  m_lev1size = frameWidth * frameHeight;
  m_lev2size = frameWidth;
  if (piCamPoints == NULL) return NULL;
  for (int n = 0; n < iDetailLevel; n++) {
    for (int v = 0; v < frameHeight; v++) {
      for (int h = 0; h < frameWidth; h++)
        ClearBit(piCamPoints, BitIndexEx(n, v, h, m_lev1size, m_lev2size));
    }
  }
  m_MainRectArea.topleft.x = (short)((frameWidth * m_aCameraPoint[POINT_TOP_LEFT].x) / 100);
  m_MainRectArea.topleft.y = (short)((frameHeight * m_aCameraPoint[POINT_TOP_LEFT].y) / 100);
  m_MainRectArea.topright.x = (short)((frameWidth * m_aCameraPoint[POINT_TOP_RIGHT].x) / 100);
  m_MainRectArea.topright.y = (short)((frameHeight * m_aCameraPoint[POINT_TOP_RIGHT].y) / 100);
  m_MainRectArea.bottomleft.x = (short)((frameWidth * m_aCameraPoint[POINT_BOTTOM_LEFT].x) / 100);
  m_MainRectArea.bottomleft.y = (short)((frameHeight * m_aCameraPoint[POINT_BOTTOM_LEFT].y) / 100);
  m_MainRectArea.bottomright.x = (short)((frameWidth * m_aCameraPoint[POINT_BOTTOM_RIGHT].x) / 100);
  m_MainRectArea.bottomright.y = (short)((frameHeight * m_aCameraPoint[POINT_BOTTOM_RIGHT].y) / 100);
  int topWidth = m_MainRectArea.topright.x - m_MainRectArea.topleft.x;
  int bottomWidth = m_MainRectArea.bottomright.x - m_MainRectArea.bottomleft.x;
  int leftHeight = m_MainRectArea.bottomleft.y - m_MainRectArea.topleft.y;
  int rightHeight = m_MainRectArea.bottomright.y - m_MainRectArea.topright.y;
  m_MainInnerRectArea.topleft.x = (short)(m_MainRectArea.topleft.x + (float(topWidth * (m_flCamInnerHori * 2))));
  m_MainInnerRectArea.topleft.y = (short)(m_MainRectArea.topleft.y + (float(leftHeight * (m_flCamInnerVert * 2))));
  m_MainInnerRectArea.topright.x = (short)(m_MainRectArea.topright.x - (float(topWidth * (m_flCamInnerHori * 2))));
  m_MainInnerRectArea.topright.y = (short)(m_MainRectArea.topright.y + (float(rightHeight * (m_flCamInnerVert * 2))));
  m_MainInnerRectArea.bottomleft.x = (short)(m_MainRectArea.bottomleft.x + (float(bottomWidth * (m_flCamInnerHori * 2))));
  m_MainInnerRectArea.bottomleft.y = (short)(m_MainRectArea.bottomleft.y - (float(leftHeight * (m_flCamInnerVert * 2))));
  m_MainInnerRectArea.bottomright.x = (short)(m_MainRectArea.bottomright.x - (float(bottomWidth * (m_flCamInnerHori * 2))));
  m_MainInnerRectArea.bottomright.y = (short)(m_MainRectArea.bottomright.y - (float(rightHeight * (m_flCamInnerVert * 2))));
  rectanglepoints* pNewRect = (rectanglepoints*)_malloc_psram(iDetailLevel * sizeof(rectanglepoints));
  if (!pNewRect) return piCamPoints;
  if (m_pRectAreas) free(m_pRectAreas);
  m_pRectAreas = pNewRect;
  rectanglepoints* rectAreas = m_pRectAreas;
  rectAreas[LEDSINGLECOLOR].topleft.x = m_MainRectArea.topleft.x;
  rectAreas[LEDSINGLECOLOR].topleft.y = m_MainRectArea.topleft.y;
  rectAreas[LEDSINGLECOLOR].topright.x = m_MainRectArea.topright.x;
  rectAreas[LEDSINGLECOLOR].topright.y = m_MainRectArea.topright.y;
  rectAreas[LEDSINGLECOLOR].bottomleft.x = m_MainRectArea.bottomleft.x;
  rectAreas[LEDSINGLECOLOR].bottomleft.y = m_MainRectArea.bottomleft.y;
  rectAreas[LEDSINGLECOLOR].bottomright.x = m_MainRectArea.bottomright.x;
  rectAreas[LEDSINGLECOLOR].bottomright.y = m_MainRectArea.bottomright.y;
  if (GetCamMode() != LED_MODE_CAM_SINGLE) {
    float flCalcHoriTop = 0, flCalcHoriBottom = 0, flCalcVertLeft = 0, flCalcVertRight = 0;
    float topWidthDetail = 0, bottomWidthDetail = 0, leftHeightDetail = 0, rightHeightDetail = 0;
    int detailAddBegin = 0, detailAddEnd = 0;
    int iVertDiffLeft = 0, iVertDiffRight = 0, iVertDiffTop = 0, iVertDiffBottom = 0;
    int calcsteps = (GetCamMode() == LED_MODE_CAM_DETAIL ? m_bCamDetailLevel : 1);
    for (int iStep = 0; iStep < calcsteps; iStep++) {
      int iCalcMulti = 100 / calcsteps;
      int iCalcStep = (iStep * iCalcMulti);
      int iCalcStepRev = ((calcsteps - iStep) * iCalcMulti);
      int iLedIndex = m_bCamDetailLeft + iStep;
      flCalcVertLeft = (leftHeight + (leftHeight / calcsteps) > rightHeight ? m_flCamInnerVert : ((float)(1.0f - (float)(rightHeight - leftHeight) / (float)(rightHeight)) * m_flCamInnerVert));
      rectAreas[iLedIndex].topleft.x = m_MainRectArea.topleft.x;
      rectAreas[iLedIndex].topleft.y = m_MainRectArea.topleft.y;
      rectAreas[iLedIndex].bottomleft.x = m_MainRectArea.bottomleft.x;
      rectAreas[iLedIndex].bottomleft.y = m_MainRectArea.bottomleft.y;
      rectAreas[iLedIndex].topright.x = m_MainRectArea.topleft.x + ((topWidth * 100 * flCalcVertLeft) / 100);
      rectAreas[iLedIndex].topright.y = (short)(m_MainRectArea.topleft.y - ((m_MainRectArea.topleft.y - m_MainRectArea.topright.y) * flCalcVertLeft));
      rectAreas[iLedIndex].bottomright.x = m_MainRectArea.bottomleft.x + ((bottomWidth * 100 * flCalcVertLeft) / 100);
      rectAreas[iLedIndex].bottomright.y = (short)(m_MainRectArea.bottomleft.y + ((m_MainRectArea.bottomright.y - m_MainRectArea.bottomleft.y) * flCalcVertLeft));
      if (calcsteps > 1) {
        topWidthDetail = rectAreas[iLedIndex].topright.x - rectAreas[iLedIndex].topleft.x;
        bottomWidthDetail = rectAreas[iLedIndex].bottomright.x - rectAreas[iLedIndex].bottomleft.x;
        leftHeightDetail = float(rectAreas[iLedIndex].bottomleft.y - rectAreas[iLedIndex].topleft.y) / float(calcsteps);
        rightHeightDetail = float(rectAreas[iLedIndex].bottomright.y - rectAreas[iLedIndex].topright.y) / float(calcsteps);
        if (bottomWidthDetail > topWidthDetail) {
          iVertDiffLeft = rectAreas[iLedIndex].topleft.x - rectAreas[iLedIndex].bottomleft.x;
          iVertDiffRight = rectAreas[iLedIndex].topright.x - rectAreas[iLedIndex].bottomright.x;
          rectAreas[iLedIndex].topleft.x -= (iVertDiffLeft * iCalcStep) / 100;
          rectAreas[iLedIndex].topright.x -= (iVertDiffRight * iCalcStep) / 100;
          rectAreas[iLedIndex].bottomleft.x += (iVertDiffLeft * (iCalcStepRev)) / 100;
          rectAreas[iLedIndex].bottomright.x += (iVertDiffRight * (iCalcStepRev)) / 100;
        } else {
          iVertDiffLeft = rectAreas[iLedIndex].bottomleft.x - rectAreas[iLedIndex].topleft.x;
          iVertDiffRight = rectAreas[iLedIndex].bottomright.x - rectAreas[iLedIndex].topright.x;
          rectAreas[iLedIndex].topleft.x += (iVertDiffLeft * iCalcStep) / 100;
          rectAreas[iLedIndex].topright.x += (iVertDiffRight * iCalcStep) / 100;
          rectAreas[iLedIndex].bottomleft.x -= (iVertDiffLeft * (iCalcStepRev)) / 100;
          rectAreas[iLedIndex].bottomright.x -= (iVertDiffRight * (iCalcStepRev)) / 100;
        }
        rectAreas[iLedIndex].topleft.y += (short)(iStep * leftHeightDetail);
        rectAreas[iLedIndex].topright.y += (short)(iStep * rightHeightDetail);
        rectAreas[iLedIndex].bottomleft.y = rectAreas[iLedIndex].topleft.y + (short)leftHeightDetail;
        rectAreas[iLedIndex].bottomright.y = rectAreas[iLedIndex].topright.y + (short)rightHeightDetail;
      }
      iLedIndex = m_bCamDetailRight + iStep;
      flCalcVertRight = (rightHeight + (rightHeight / calcsteps) > leftHeight ? m_flCamInnerVert : ((float)(1.0f - (float)(leftHeight - rightHeight) / (float)(leftHeight)) * m_flCamInnerVert));
      rectAreas[iLedIndex].topright.x = m_MainRectArea.topright.x;
      rectAreas[iLedIndex].topright.y = m_MainRectArea.topright.y;
      rectAreas[iLedIndex].bottomright.x = m_MainRectArea.bottomright.x;
      rectAreas[iLedIndex].bottomright.y = m_MainRectArea.bottomright.y;
      rectAreas[iLedIndex].topleft.x = m_MainRectArea.topright.x - ((topWidth * 100 * flCalcVertRight) / 100);
      rectAreas[iLedIndex].topleft.y = (short)(m_MainRectArea.topright.y - ((m_MainRectArea.topright.y - m_MainRectArea.topleft.y) * flCalcVertRight));
      rectAreas[iLedIndex].bottomleft.x = m_MainRectArea.bottomright.x - ((bottomWidth * 100 * flCalcVertRight) / 100);
      rectAreas[iLedIndex].bottomleft.y = (short)(m_MainRectArea.bottomright.y - ((m_MainRectArea.bottomright.y - m_MainRectArea.bottomleft.y) * flCalcVertRight));
      if (calcsteps > 1) {
        topWidthDetail = rectAreas[iLedIndex].topright.x - rectAreas[iLedIndex].topleft.x;
        bottomWidthDetail = rectAreas[iLedIndex].bottomright.x - rectAreas[iLedIndex].bottomleft.x;
        leftHeightDetail = float(rectAreas[iLedIndex].bottomleft.y - rectAreas[iLedIndex].topleft.y) / float(calcsteps);
        rightHeightDetail = float(rectAreas[iLedIndex].bottomright.y - rectAreas[iLedIndex].topright.y) / float(calcsteps);
        if (bottomWidthDetail > topWidthDetail) {
          iVertDiffLeft = rectAreas[iLedIndex].bottomleft.x - rectAreas[iLedIndex].topleft.x;
          iVertDiffRight = rectAreas[iLedIndex].bottomright.x - rectAreas[iLedIndex].topright.x;
          rectAreas[iLedIndex].topleft.x += (iVertDiffLeft * iCalcStep) / 100;
          rectAreas[iLedIndex].topright.x += (iVertDiffRight * iCalcStep) / 100;
          rectAreas[iLedIndex].bottomleft.x -= (iVertDiffLeft * (iCalcStepRev)) / 100;
          rectAreas[iLedIndex].bottomright.x -= (iVertDiffRight * (iCalcStepRev)) / 100;
        } else {
          iVertDiffLeft = rectAreas[iLedIndex].topleft.x - rectAreas[iLedIndex].bottomleft.x;
          iVertDiffRight = rectAreas[iLedIndex].topright.x - rectAreas[iLedIndex].bottomright.x;
          rectAreas[iLedIndex].topleft.x -= (iVertDiffLeft * iCalcStep) / 100;
          rectAreas[iLedIndex].topright.x -= (iVertDiffRight * iCalcStep) / 100;
          rectAreas[iLedIndex].bottomleft.x += (iVertDiffLeft * (iCalcStepRev)) / 100;
          rectAreas[iLedIndex].bottomright.x += (iVertDiffRight * (iCalcStepRev)) / 100;
        }
        rectAreas[iLedIndex].topleft.y += (short)(iStep * leftHeightDetail);
        rectAreas[iLedIndex].topright.y += (short)(iStep * rightHeightDetail);
        rectAreas[iLedIndex].bottomleft.y = rectAreas[iLedIndex].topleft.y + (short)leftHeightDetail;
        rectAreas[iLedIndex].bottomright.y = rectAreas[iLedIndex].topright.y + (short)rightHeightDetail;
      }
      iLedIndex = m_bCamDetailTop + iStep;
      flCalcHoriTop = (topWidth + (topWidth / calcsteps) > bottomWidth ? m_flCamInnerHori : ((float)(1.0f - (float)(bottomWidth - topWidth) / (float)(bottomWidth)) * m_flCamInnerHori));
      rectAreas[iLedIndex].topleft.x = m_MainRectArea.topleft.x;
      rectAreas[iLedIndex].topleft.y = m_MainRectArea.topleft.y;
      rectAreas[iLedIndex].topright.x = m_MainRectArea.topright.x;
      rectAreas[iLedIndex].topright.y = m_MainRectArea.topright.y;
      rectAreas[iLedIndex].bottomleft.y = m_MainRectArea.topleft.y + ((leftHeight * 100 * flCalcHoriTop) / 100);
      rectAreas[iLedIndex].bottomleft.x = (short)(m_MainRectArea.topleft.x + ((m_MainRectArea.bottomleft.x - m_MainRectArea.topleft.x) * flCalcHoriTop));
      rectAreas[iLedIndex].bottomright.y = m_MainRectArea.topright.y + ((rightHeight * 100 * flCalcHoriTop) / 100);
      rectAreas[iLedIndex].bottomright.x = (short)(m_MainRectArea.topright.x + ((m_MainRectArea.bottomright.x - m_MainRectArea.topright.x) * flCalcHoriTop));
      if (calcsteps > 1) {
        topWidthDetail = float(rectAreas[iLedIndex].topright.x - rectAreas[iLedIndex].topleft.x) / float(calcsteps);
        detailAddBegin = (iStep == 0 ? ((rectAreas[iLedIndex].topright.x - rectAreas[iLedIndex].topleft.x) % calcsteps) / 2 : 0);
        detailAddEnd = (iStep == calcsteps / 2 ? detailAddBegin : 0);
        rectAreas[iLedIndex].topleft.x += (short)((iStep * topWidthDetail) + detailAddBegin);
        rectAreas[iLedIndex].topright.x = rectAreas[iLedIndex].topleft.x + (short)topWidthDetail + detailAddEnd;
        bottomWidthDetail = float(rectAreas[iLedIndex].bottomright.x - rectAreas[iLedIndex].bottomleft.x) / float(calcsteps);
        detailAddBegin = (iStep == 0 ? ((rectAreas[iLedIndex].bottomright.x - rectAreas[iLedIndex].bottomleft.x) % calcsteps) / 2 : 0);
        detailAddEnd = (iStep == calcsteps / 2 ? detailAddBegin : 0);
        rectAreas[iLedIndex].bottomleft.x += (short)((iStep * bottomWidthDetail) + detailAddBegin);
        rectAreas[iLedIndex].bottomright.x = rectAreas[iLedIndex].bottomleft.x + (short)bottomWidthDetail + detailAddEnd;
        iVertDiffTop = rectAreas[iLedIndex].topright.y - rectAreas[iLedIndex].topleft.y;
        iVertDiffBottom = rectAreas[iLedIndex].bottomright.y - rectAreas[iLedIndex].bottomleft.y;
        rectAreas[iLedIndex].topleft.y += (iVertDiffTop * iCalcStep) / 100;
        rectAreas[iLedIndex].topright.y -= (iVertDiffTop * iCalcStepRev) / 100;
        rectAreas[iLedIndex].bottomleft.y += (iVertDiffBottom * iCalcStep) / 100;
        rectAreas[iLedIndex].bottomright.y -= (iVertDiffBottom * iCalcStepRev) / 100;
      }
      iLedIndex = m_bCamDetailBottom + iStep;
      flCalcHoriBottom = (bottomWidth + (bottomWidth / calcsteps) > topWidth ? m_flCamInnerHori : ((float)(1.0f - (float)(topWidth - bottomWidth) / (float)(topWidth)) * m_flCamInnerHori));
      rectAreas[iLedIndex].bottomleft.x = m_MainRectArea.bottomleft.x;
      rectAreas[iLedIndex].bottomleft.y = m_MainRectArea.bottomleft.y;
      rectAreas[iLedIndex].bottomright.x = m_MainRectArea.bottomright.x;
      rectAreas[iLedIndex].bottomright.y = m_MainRectArea.bottomright.y;
      rectAreas[iLedIndex].topleft.y = m_MainRectArea.bottomleft.y - ((leftHeight * 100 * flCalcHoriBottom) / 100);
      rectAreas[iLedIndex].topleft.x = (short)(m_MainRectArea.bottomleft.x + ((m_MainRectArea.topleft.x - m_MainRectArea.bottomleft.x) * flCalcHoriBottom));
      rectAreas[iLedIndex].topright.y = m_MainRectArea.bottomright.y - ((rightHeight * 100 * flCalcHoriBottom) / 100);
      rectAreas[iLedIndex].topright.x = (short)(m_MainRectArea.bottomright.x + ((m_MainRectArea.topright.x - m_MainRectArea.bottomright.x) * flCalcHoriBottom));
      if (calcsteps > 1) {
        topWidthDetail = float(rectAreas[iLedIndex].topright.x - rectAreas[iLedIndex].topleft.x) / float(calcsteps);
        detailAddBegin = (iStep == 0 ? ((rectAreas[iLedIndex].topright.x - rectAreas[iLedIndex].topleft.x) % calcsteps) / 2 : 0);
        detailAddEnd = (iStep == calcsteps / 2 ? detailAddBegin : 0);
        rectAreas[iLedIndex].topleft.x += (short)((iStep * topWidthDetail) + detailAddBegin);
        rectAreas[iLedIndex].topright.x = rectAreas[iLedIndex].topleft.x + (short)topWidthDetail + detailAddEnd;
        bottomWidthDetail = float(rectAreas[iLedIndex].bottomright.x - rectAreas[iLedIndex].bottomleft.x) / float(calcsteps);
        detailAddBegin = (iStep == 0 ? ((rectAreas[iLedIndex].bottomright.x - rectAreas[iLedIndex].bottomleft.x) % calcsteps) / 2 : 0);
        detailAddEnd = (iStep == calcsteps / 2 ? detailAddBegin : 0);
        rectAreas[iLedIndex].bottomleft.x += (short)((iStep * bottomWidthDetail) + detailAddBegin);
        rectAreas[iLedIndex].bottomright.x = rectAreas[iLedIndex].bottomleft.x + (short)bottomWidthDetail + detailAddEnd;
        iVertDiffTop = rectAreas[iLedIndex].topright.y - rectAreas[iLedIndex].topleft.y;
        iVertDiffBottom = rectAreas[iLedIndex].bottomright.y - rectAreas[iLedIndex].bottomleft.y;
        rectAreas[iLedIndex].topleft.y += (iVertDiffTop * iCalcStep) / 100;
        rectAreas[iLedIndex].topright.y -= (iVertDiffTop * iCalcStepRev) / 100;
        rectAreas[iLedIndex].bottomleft.y += (iVertDiffBottom * iCalcStep) / 100;
        rectAreas[iLedIndex].bottomright.y -= (iVertDiffBottom * iCalcStepRev) / 100;
      }
    }
  }
  int iLoop = 1, iStep = 1;
  if (GetCamMode() == LED_MODE_CAM_BAR) {
    iLoop = CAM_SECTOR_COUNT(m_bCamDetailLevel);
    iStep = GetCamDetailLevel();
  } else if (GetCamMode() == LED_MODE_CAM_DETAIL) {
    iLoop = CAM_SECTOR_COUNT(m_bCamDetailLevel);
  }
  for (int i = 0; i < iLoop; i += (GetCamMode() != LED_MODE_CAM_SINGLE && i == LEDSINGLECOLOR ? 1 : iStep)) {
    int iPixelsTrue = 0;
    float A = camCalcColorsArea(rectAreas[i].topright.x, rectAreas[i].topright.y, rectAreas[i].bottomright.x, rectAreas[i].bottomright.y, rectAreas[i].bottomleft.x, rectAreas[i].bottomleft.y) +
              camCalcColorsArea(rectAreas[i].topright.x, rectAreas[i].topright.y, rectAreas[i].topleft.x, rectAreas[i].topleft.y, rectAreas[i].bottomleft.x, rectAreas[i].bottomleft.y);
    for (int v = 0; v < frameHeight; v++) {
      for (int h = 0; h < frameWidth; h++) {
        if (camCalcColorsIsPixelInside(A, rectAreas[i].topright.x, rectAreas[i].topright.y, rectAreas[i].bottomright.x, rectAreas[i].bottomright.y, rectAreas[i].bottomleft.x, rectAreas[i].bottomleft.y, rectAreas[i].topleft.x, rectAreas[i].topleft.y, h, v)) {
          SetBit(piCamPoints, BitIndexEx(i, v, h, m_lev1size, m_lev2size));
          iPixelsTrue++;
        }
      }
    }
    if (i == LEDSINGLECOLOR && m_MainInnerRectArea.topleft.x != -1) {
      float Ainner = camCalcColorsArea(m_MainInnerRectArea.topright.x, m_MainInnerRectArea.topright.y, m_MainInnerRectArea.bottomright.x, m_MainInnerRectArea.bottomright.y, m_MainInnerRectArea.bottomleft.x, m_MainInnerRectArea.bottomleft.y) +
                     camCalcColorsArea(m_MainInnerRectArea.topright.x, m_MainInnerRectArea.topright.y, m_MainInnerRectArea.topleft.x, m_MainInnerRectArea.topleft.y, m_MainInnerRectArea.bottomleft.x, m_MainInnerRectArea.bottomleft.y);
      for (int v = 0; v < frameHeight; v++) {
        for (int h = 0; h < frameWidth; h++) {
          if (camCalcColorsIsPixelInside(Ainner, m_MainInnerRectArea.topright.x, m_MainInnerRectArea.topright.y, m_MainInnerRectArea.bottomright.x, m_MainInnerRectArea.bottomright.y, m_MainInnerRectArea.bottomleft.x, m_MainInnerRectArea.bottomleft.y, m_MainInnerRectArea.topleft.x, m_MainInnerRectArea.topleft.y, h, v)) {
            ClearBit(piCamPoints, BitIndexEx(i, v, h, m_lev1size, m_lev2size));
            iPixelsTrue--;
          }
        }
      }
    }
    rectAreas[i].shPixelCount = (short)(iPixelsTrue & 0xffff);
    rectAreas[i].shPixelMinCount = (short)((rectAreas[i].shPixelCount * global::GetCamMinPixelAmount()) / 100) & 0xffff;
  }
  camCalcColorsInitGlobalVariables();
  if (m_piCamPoints) free(m_piCamPoints);
  m_piCamPoints = piCamPoints;
  return m_piCamPoints;
}

#if defined(ESP32)
/* Cam task: capture loop and task lifecycle (CamTask_Start/Delete).
   Implemented here so builds that do not compile camtask.cpp (e.g. USBFlasher sketch copy) still link. */
#include "camtask.h"
#include "ledhelper.h"
#include <esp32cam.h>
#include "sensor.h"
#include "freertos/task.h"
#include <WiFi.h>

static TaskHandle_t handleTaskCam = NULL;
static void TaskCamCode(void* pvParameters);

static unsigned long s_nTimeStart = 0;
static unsigned long s_nTimeCapture = 0;
static unsigned long s_nTimeJpg2Rgb = 0;
static unsigned long s_nTimeAfterColorCalc = 0;
static unsigned long s_nTimeFinish = 0;
static unsigned long s_nTimeFPSCheck = 0;

/* Static buffers for TaskCamCode to avoid ~1.8KB stack usage (stack canary overflow) */
static CRGB s_camleds[MAX_LEDS];
static short s_shoutPixels[MAX_LEDS];
static short s_shblackPixels[MAX_LEDS];

void CamTask_Start(void) {
  if (handleTaskCam == NULL) {
    xTaskCreatePinnedToCore(&TaskCamCode,
                            "TaskCam",
                            16384,  /* 16KB stack: JPEG decode + color calc need more than 8KB (stack canary overflow) */
                            NULL,
                            DEFAULT_TASK_PRIORITY,
                            &handleTaskCam,
                            ARDUINO_RUNNING_CORE);
    _DBGPRINTF(PSTR(PSTR_LED "TaskCam created HANDLE %d\n"), handleTaskCam);
  } else {
    int iStatus = eTaskGetState(handleTaskCam);
    if (iStatus == eSuspended) {
      _DBGPRINTF(PSTR(PSTR_LED "TaskCam suspended, resuming\n"));
      vTaskResume(handleTaskCam);
    }
  }
}

void CamTask_Delete(void) {
  if (handleTaskCam != NULL) {
    vTaskDelete(handleTaskCam);
    handleTaskCam = NULL;
  }
}

static void TaskCamCode(void* pvParameters) {
  _DBGPRINTF(PSTR("TaskCamCode: ENTER\n"));

  int iFinalWidth = 0;
  int iFinalHeight = 0;
  int iAllPixels = 0;
  byte bFailedFrames = 0;
  byte bFailedFramesCtr = 0;
  bool bFrameFailed = false;
  bool bCompareFrame = false;

  LEDHelper* pLedHelper = global::m_pLedHelper;

  do {
    memset(s_camleds, 0, MAX_LEDS * sizeof(struct CRGB));

    global::m_pCamProcessor->SetSectorOutputBuffer((void*)s_camleds);
    global::m_pCamProcessor->SetPshPixels(s_shoutPixels);
    global::m_pCamProcessor->SetPshBlackPixels(s_shblackPixels);

    while (pLedHelper != NULL && pLedHelper->GetLedMode() == LED_MODE_CAM) {
      while (WiFi.status() != WL_CONNECTED) {
        _DBGPRINTF_ULTRA(PSTR("TaskCamCode: Error, WiFi DISCONNECTED - wait 1000ms!\n"));
        _taskDelay(1000);
        continue;
      }

      while (global::GetFirmwareRunning()) {
        _DBGPRINTF_ULTRA(PSTR("TaskCamCode: Error, Firmware Running - wait 1000ms!\n"));
        _taskDelay(1000);
        continue;
      }

      s_nTimeStart = millis();

      int iCurrenFrameLength = (global::GetFirmwareRunning()
#if defined(ENABLE_RELAY)
                                || global::GetAmbibackRelayFailed() > 0
#endif
                                ? NET_SLOW_FPS_FRAME : global::GetCamPerformanceFrameTime() );

      if (!global::m_pCamProcessor->camMutexLock()) {
        global::errprintf_P(PSTR("TaskCamCode: Error, Camera capture failed - MUTEX STILL LOCKED!\n"));
        continue;
      }

      _taskYIELD();
      auto frame = esp32cam::capture();

      bCompareFrame = bCompareFrame ? false : true;

      global::m_pCamProcessor->camMutexRelease();

      s_nTimeCapture = (millis() - s_nTimeStart);

      if (frame == nullptr) {
        if (bFrameFailed) {
          global::errprintf_P(PSTR("TaskCamCode: Error, Camera capture failed - try to restart CAMERA Module!\n"));
          global::DeInitCam();
          _taskYIELD();
          global::InitCam();
          _taskYIELD();
          continue;
        } else {
          global::errprintf_P(PSTR("TaskCamCode: Error, Camera capture failed - retry!\n"));
          bFrameFailed = true;
          _taskYIELD();
          continue;
        }
      } else if ( FRAMESIZE_WIDTH[global::m_iCamIndex] != frame->getWidth() ||
                  FRAMESIZE_HEIGHT[global::m_iCamIndex] != frame->getHeight() ) {
        _DBGPRINTF(PSTR("TaskCamCode: CAPTURE OK %dx%d %db FORMAT=%d, but INVALID Format - SKIPPING Frame\n"),
                   frame->getWidth(),
                   frame->getHeight(),
                   static_cast<int>(frame->size()),
                   frame->getPixFormat());
        _taskDelay(100);
        bFrameFailed = false;
        continue;
      } else {
        _DBGPRINTF_ULTRA(PSTR("TaskCamCode: CAPTURE OK %dx%d %db FORMAT=%d\n"),
                         frame->getWidth(),
                         frame->getHeight(),
                         static_cast<int>(frame->size()),
                         frame->getPixFormat());
        _taskYIELD();
        bFrameFailed = false;
      }

      uint8_t* uiData = frame->data();
      size_t tzDataLen = frame->size();

      for (int i = 0; i < MAX_LEDS; i++) {
        s_shoutPixels[i] = 0;
        s_shblackPixels[i] = 0;
      }

      int iOldFinalWidth = iFinalWidth;
      int iOldFinalHeight = iFinalHeight;
      iFinalWidth = frame->getWidth();
      iFinalHeight = frame->getHeight();
      iAllPixels = iFinalWidth * iFinalHeight;

      global::m_pCamProcessor->SetRealFrameWidth(iFinalWidth);
      global::m_pCamProcessor->SetRealFrameHeight(iFinalHeight);

      uint8_t* pOutbuf = NULL;
      int iJpgScale = JPG_SCALE_NONE;
      int iPixMulti = 1;
      if (frame->getPixFormat() == PIXFORMAT_JPEG) {
        global::m_pCamProcessor->camSetJpegProperties(frame->getWidth(), &iJpgScale, &iPixMulti, &iFinalWidth, &iFinalHeight);
      } else if (frame->getPixFormat() == PIXFORMAT_RGB888) {
        _DBGPRINTF_ULTRA(PSTR("TaskCamCode: RGB888 FORMAT\n"));
      } else if (frame->getPixFormat() == PIXFORMAT_RGB565) {
        _DBGPRINTF_ULTRA(PSTR("TaskCamCode: RGB565 FORMAT\n"));
      } else {
        _DBGPRINTF(PSTR("TaskCamCode: Error, UNEXPECTED Format %d - wait 1000ms!\n"), frame->getPixFormat());
        _taskDelay(1000);
        continue;
      }

      global::m_pCamProcessor->SetFrameWidth(iFinalWidth);
      global::m_pCamProcessor->SetFrameHeight(iFinalHeight);

      if (global::m_pCamProcessor->GetCamPoints() == NULL ||
          global::m_pCamProcessor->getCameraPointsUpdated() ||
          iOldFinalWidth != iFinalWidth ||
          iOldFinalHeight != iFinalHeight
         ) {

        global::m_pCamProcessor->camCalcCamPointAreas(iFinalWidth, iFinalHeight);

        global::m_pCamProcessor->setCameraPointsUpdated(false);

        if (global::m_pCamProcessor->GetCamPoints() == NULL) {
          _DBGPRINTF(PSTR("TaskCamCode: Error, failed alloc memory for CamPoints - wait 1000ms!\n"));
          _taskDelay(1000);
          continue;
        }
      }

      if (frame->getPixFormat() == PIXFORMAT_JPEG) {
        iAllPixels = iFinalWidth * iFinalHeight;

        if (global::GetCamJpegCalcDirect() == 0) {
          size_t rgbOutLen = (iFinalWidth * iFinalHeight * 3);
          _DBGPRINTF_ULTRA(PSTR("TaskCamCode: CONVERT FROM JPG FinalWidth=%d FinalHeight %d Inlen=%d Outlen=%d ESP.getFreeHeap=%d\n"), iFinalWidth, iFinalHeight, tzDataLen, rgbOutLen, ESP.getFreeHeap() );
          pOutbuf = (uint8_t *)_malloc_psram(rgbOutLen);

          if (!pOutbuf) {
            bFailedFramesCtr++;
            _taskYIELD();
            continue;
          }
          if (CamProcessor::camJpg2rgb888(uiData, tzDataLen, pOutbuf, (jpg_scale_t)iJpgScale)) {
            _DBGPRINTF_ULTRA(PSTR("TaskCamCode: SUCCESS camJpg2rgb888 conversion (Len=%d, Pixels=%d).\n"), rgbOutLen, iAllPixels);
            uiData = pOutbuf;

            s_nTimeJpg2Rgb = (millis() - s_nTimeStart);

            global::m_pCamProcessor->camCalcColorsFromCamPoints(uiData, frame->getPixFormat());
          } else {
            _DBGPRINTF_ULTRA(PSTR("TaskCamCode: FAIL camJpg2rgb888!\n"));
            bFailedFramesCtr++;
            _taskYIELD();
            free(pOutbuf);
            pOutbuf = NULL;
            continue;
          }
        } else {
          if (global::m_pCamProcessor->camJpg2rgbDirectProcess(uiData, tzDataLen, (jpg_scale_t)iJpgScale)) {
            _DBGPRINTF_ULTRA(PSTR("TaskCamCode: SUCCESS camJpg2rgbDirectProcess conversion (Pixels=%d).\n"), iAllPixels);
          } else {
            _DBGPRINTF_ULTRA(PSTR("TaskCamCode: FAIL camJpg2rgbDirectProcess! DataLen: %d\n"), tzDataLen);
            bFailedFramesCtr++;
            _taskYIELD();
            continue;
          }
        }
      }

      if (pOutbuf != NULL) free(pOutbuf);

      s_nTimeAfterColorCalc =  (millis() - s_nTimeStart);

      _taskYIELD();

      if (!pLedHelper->SetLedBufferFromCamSectors(s_camleds)) {
        _DBGPRINTF(PSTR("TaskCamCode: Error, UNEXPECTED CAM MODE %d\n"), global::m_pCamProcessor->GetCamMode());
        _taskDelay(1000);
        continue;
      }

      pLedHelper->ZeroConfiguredLeds();

      _DBGPRINTF_ULTRA(PSTR("TaskCamCode: before getProcessingLedNextStep %d ms\n"), (millis() - s_nTimeStart));

      while ( pLedHelper->getProcessingLedNextStep() ) {
        _taskYIELD();
      }

      _DBGPRINTF_ULTRA(PSTR("TaskCamCode: after getProcessingLedNextStep %d ms\n"), (millis() - s_nTimeStart));

      s_nTimeFinish =  (millis() - s_nTimeStart);

      global::CalcDynamicDelayLength( s_nTimeStart );

      if (global::GetCamAutoOnOff() == 1 && global::m_pCamProcessor->GetBlackSeconds() > global::GetCamAutoOnOffDelay()) {
        int iBlackScreenSleeptime = 1000 / global::GetCamFrameBuffer();

        _DBGPRINTF_ULTRA(PSTR("TaskCamCode: CamAutoOnOff enabled, BLACK SCREEN since %d/%d seconds - sleep %dms\n"), global::m_pCamProcessor->GetBlackSeconds(), global::GetCamAutoOnOffDelay(), iBlackScreenSleeptime);
        _taskDelay(iBlackScreenSleeptime);
      } else {
#if defined(ENABLE_RELAY)
        if (global::GetAmbibackRelaymodeEnabled() && global::GetAmbibackRelayModeDirect() == 1) {
          pLedHelper->RelayPrepareDirectLedData();
          pLedHelper->RelayLedData();
        } else
#endif
        {
          pLedHelper->setProcessingLedNextStep(true);

          pLedHelper->EnableTaskProcessNextStep();
        }
      }

      if (s_nTimeFPSCheck == 0 ||
          millis() > s_nTimeFPSCheck) {
        global::m_pCamProcessor->SetCurrentCamFps(global::m_pCamProcessor->GetCurrentCamFramesCtr());
        global::m_pCamProcessor->SetCurrentCamFramesCtr(0);

        bFailedFrames = bFailedFramesCtr;
        bFailedFramesCtr = 0;

        if (global::m_pCamProcessor->GetBlackFrames() >= global::m_pCamProcessor->GetCurrentCamFps()) {
          global::m_pCamProcessor->IncrementBlackSeconds();

#if defined(ESP32_CPU_AUTOLOW)
          if (getCpuFrequencyMhz() > 80 && global::m_pCamProcessor->GetBlackSeconds() > BLACK_AUTOCPULOWSECONDS ) {
            _DBGPRINTF(PSTR("TaskCamCode: CAM VIDEO BLACK SCREEN DETECTED since %d seconds, set CPU freq to 80 Mhz = %s!\n"),
                       global::m_pCamProcessor->GetCurrentCamFps(),
                       (setCpuFrequencyMhz(80) ? "SUCCESS" : "FAIL") );
          }
#endif
          global::m_pCamProcessor->ResetBlackFrames();
        } else if (global::m_pCamProcessor->GetBlackFrames() >= global::m_pCamProcessor->GetCurrentCamFps()) {
        } else {
          if (global::m_pCamProcessor->GetBlackSeconds() > 0) {
#if defined(ESP32_CPU_AUTOLOW)
            _DBGPRINTF(PSTR("TaskCamCode: CAM VIDEO DETECTED after %d seconds, set CPU freq to 240 Mhz = %s\n"),
                       global::m_pCamProcessor->GetBlackSeconds(),
                       (setCpuFrequencyMhz(240) ? "SUCCESS" : "FAIL") );
#endif
            global::m_pCamProcessor->ResetBlackSeconds();
          }
        }

        _DBGPRINTF(PSTR("TaskCamCode: CAPTURE PROCESSED in %d/%d/%d/%d ms, DelayLen=%d CamFPS=%d/%d BlackSec=%d Total/Skip/Black/White Pixels=%d/%d/%d/%d Total/Black/White/BelowWhite Sectors=%d/%d/%d/%d ColActTresh=%d AvrOutside=%d AvrInside=%d AecValue=%d AgcGain=%d \n"),
                   s_nTimeCapture,
                   s_nTimeJpg2Rgb,
                   s_nTimeAfterColorCalc,
                   s_nTimeFinish,
                   pLedHelper->GetDelayLength(),
                   global::m_pCamProcessor->GetCurrentCamFps(),
                   bFailedFrames,
                   global::m_pCamProcessor->GetBlackSeconds(),
                   global::m_pCamProcessor->GetTotalPixels(),
                   global::m_pCamProcessor->GetSkipPixels(),
                   global::m_pCamProcessor->GetBlackPixels(),
                   global::m_pCamProcessor->GetWhitePixels(),
                   global::m_pCamProcessor->GetTotalSectors(),
                   global::m_pCamProcessor->GetBlackSectors(),
                   global::m_pCamProcessor->GetWhiteSectors(),
                   global::m_pCamProcessor->GetBelowWhiteSectors(),
                   global::m_pCamProcessor->GetColActivationTreshold(),
                   global::m_pCamProcessor->GetAvrOutside(),
                   global::m_pCamProcessor->GetAvrInside(),
                   global::GetCamAecValue(),
                   global::GetCamAgcGain()
                  );

        s_nTimeFPSCheck = millis() + 1000;
      }

      global::m_pCamProcessor->IncrementCurrentCamFramesCtr();

      if (iCurrenFrameLength > s_nTimeFinish) {
        _taskDelay(iCurrenFrameLength - s_nTimeFinish);
      }
    }

    global::m_pCamProcessor->ResetBlackFrames();
    global::m_pCamProcessor->ResetBlackSeconds();
    global::m_pCamProcessor->SetCurrentCamFps(0);
    global::m_pCamProcessor->SetCurrentCamFramesCtr(0);

    global::m_pCamProcessor->SetSectorOutputBuffer(NULL);
    global::m_pCamProcessor->SetPshPixels(NULL);
    global::m_pCamProcessor->SetPshBlackPixels(NULL);

    _DBGPRINTF(PSTR("TaskCamCode: CamLOOP END!\n"));

    if (pLedHelper != NULL)
      pLedHelper->EnableTaskFadeOut();

    _taskYIELD();

    _DBGPRINTF(PSTR("TaskCamCode: Suspend TASK for ourself %d \n"), handleTaskCam);
    vTaskSuspend(handleTaskCam);
  } while (true);

  _DBGPRINTF(PSTR("TaskCamCode: EXIT\n"));
}

#endif /* ESP32 */
#endif /* ENABLE_CAM */
