#ifndef camprocessor_h
#define camprocessor_h

#include "sectorgeometry.h"
#include "globalincludes.h"

#if defined(ENABLE_CAM)

#include <freertos/semphr.h>

/* CamProcessor: camera image -> sector colors (at :: scope). No dependency on LEDHelper. */
class CamProcessor {
public:
  CamProcessor();
  ~CamProcessor();

  /* Sector output buffer: set before ProcessFrame/camCalcColorsFromCamPoints. */
  void SetSectorOutputBuffer(void* pCamleds) { m_pVoidCamleds = pCamleds; }
  void SetPshPixels(short* pshPixels) { m_pshPixels = pshPixels; }
  void SetPshBlackPixels(short* pshBlackPixels) { m_pshBlackPixels = pshBlackPixels; }
  void SetFrameWidth(int iWidth) { m_frameWidth = iWidth; }
  void SetFrameHeight(int iHeight) { m_frameHeight = iHeight; }
  void SetRealFrameWidth(int iWidth) { m_realframeWidth = iWidth; }
  void SetRealFrameHeight(int iHeight) { m_realframeHeight = iHeight; }

  /* JPEG decode. camJpg2rgb888 is static; camJpg2rgbDirectProcess uses this instance for callbacks. */
  static bool camJpg2rgb888(const uint8_t* src, size_t src_len, uint8_t* out, int scale);
  bool camJpg2rgbDirectProcess(const uint8_t* src, size_t src_len, int scale);
  /* Used by JPEG direct decode callback (internal). */
  bool ProcessRgbDirectBlock(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* data, bool isStart, bool isEnd);

  /* Sector geometry: allocates and owns m_piCamPoints and m_pRectAreas. */
  int* camCalcCamPointAreas(int frameWidth, int frameHeight);
  int* GetCamPoints() { return m_piCamPoints; }
  rectanglepoints* GetMainRectArea() { return &m_MainRectArea; }
  rectanglepoints* GetMainInnerRectArea() { return &m_MainInnerRectArea; }
  rectanglepoints* GetRectAreas() { return m_pRectAreas; }

  /* Image -> sector colors. Call after SetSectorOutputBuffer/SetPshPixels/SetPshBlackPixels/SetFrameWidth/SetFrameHeight. */
  void camCalcColorsFromCamPoints(uint8_t* uiData, int iFormat);
  void camCalcColorsInitGlobalVariables();
  void camCalcColorsInitFrameVariables();
  void camCalcColorsProcessPixel(int x, int y, int* piRed, int* piGreen, int* piBlue, int iAvarage);
  void camCalcColorsFinishFrameVariables();
  void camSetJpegProperties(int iFrameWidth, int* piJpgScale, int* piMulti, int* piFinalWidth, int* piFinalHeight);

  /* Camera smoothing (used by LEDHelper during sector->strip mapping). */
  void PreProcessCameraSmoothing(byte iIndex, int* piRed, int* piGreen, int* piBlue, void* pVoidLedFrames);
  void PostProcessCameraSmoothing(void* pVoidLedFrames, void* pVoidLedsNext, int numLeds);
  void* GetVoidLedSmoothFramesPtr() { return m_pLedSmoothFrames; }

  /* Mutex for capture/snapshot. */
  bool camMutexLock();
  void camMutexRelease();

  /* Camera points */
  short getCameraPointX(byte index);
  short getCameraPointY(byte index);
  void setCameraPoint(byte index, short x, short y) { setCameraPoint(index, x, y, true); }
  void setCameraPoint(byte index, short x, short y, bool bEpprom);
  bool getCameraPointsUpdated() { return m_bCameraPointsUpdated; }
  void setCameraPointsUpdated(bool bNew) { m_bCameraPointsUpdated = bNew; }

  /* Cam mode and detail */
  byte GetCamMode() { return m_bCamMode; }
  void SetCamMode(byte bNew, bool bEpprom);
  void SetCamMode(byte bNew) { SetCamMode(bNew, true); }
  byte GetCamDetailLevel() { return m_bCamDetailLevel; }
  void SetCamDetailLevel(byte bNew, bool bEpprom);
  void SetCamDetailLevel(byte bNew) { SetCamDetailLevel(bNew, true); }
  /* Sector indices for cam-to-LED mapping (used by LEDHelper::camCalcNextLedStep*) */
  byte GetCamDetailLeft() { return m_bCamDetailLeft; }
  byte GetCamDetailTop() { return m_bCamDetailTop; }
  byte GetCamDetailRight() { return m_bCamDetailRight; }
  byte GetCamDetailBottom() { return m_bCamDetailBottom; }

  float GetCamInnerHori() { return m_flCamInnerHori; }
  void SetCamInnerHori(float flNew, bool bEpprom);
  void SetCamInnerHori(float flNew) { SetCamInnerHori(flNew, true); }
  float GetCamInnerVert() { return m_flCamInnerVert; }
  void SetCamInnerVert(float flNew, bool bEpprom);
  void SetCamInnerVert(float flNew) { SetCamInnerVert(flNew, true); }

  /* Color boost (cam-specific) */
  int GetColorBoostMaxPercent() { return m_iColorBoostMaxPercent; }
  void SetColorBoostMaxPercent(int iNew, bool bEpprom);
  void SetColorBoostMaxPercent(int iNew) { SetColorBoostMaxPercent(iNew, true); }

  /* Black threshold smoothing */
  byte GetCamBlackThresholdSmoothing() { return m_bCamBlackThresholdSmoothing; }
  void SetCamBlackThresholdSmoothing(byte bNew, bool bEpprom);
  void SetCamBlackThresholdSmoothing(byte bNew) { SetCamBlackThresholdSmoothing(bNew, true); }

  /* Smoothing */
  byte GetCamDetailSmooth() { return m_bCamDetailSmooth; }
  void SetCamDetailSmooth(byte bNew, bool bEpprom);
  void SetCamDetailSmooth(byte bNew) { SetCamDetailSmooth(bNew, true); }
  byte GetCamSmoothEnable() { return m_bCamSmoothEnable; }
  void SetCamSmoothEnable(byte bNew, bool bEpprom);
  void SetCamSmoothEnable(byte bNew) { SetCamSmoothEnable(bNew, true); }
  byte GetCamSmoothMaxFrames() { return m_bCamSmoothMaxFrames; }
  void SetCamSmoothMaxFrames(byte bNew, bool bEpprom);
  void SetCamSmoothMaxFrames(byte bNew) { SetCamSmoothMaxFrames(bNew, true); }
  byte GetCamSmoothMaxDiff() { return m_bCamSmoothMaxDiff; }
  void SetCamSmoothMaxDiff(byte bNew, bool bEpprom);
  void SetCamSmoothMaxDiff(byte bNew) { SetCamSmoothMaxDiff(bNew, true); }
  byte GetCamSmooth() { return m_bCamSmooth; }
  void SetCamSmooth(byte bNew, bool bEpprom);
  void SetCamSmooth(byte bNew) { SetCamSmooth(bNew, true); }

  /* FPS / black detection (for TaskCamCode and UI) */
  byte GetCurrentCamFps() { return m_bCurrentCamFps; }
  void SetCurrentCamFps(byte bNew) { m_bCurrentCamFps = bNew; }
  byte GetCurrentCamFramesCtr() { return m_bCurrentCamFramesCtr; }
  void SetCurrentCamFramesCtr(byte bNew) { m_bCurrentCamFramesCtr = bNew; }
  void IncrementCurrentCamFramesCtr() { m_bCurrentCamFramesCtr++; }
  int GetBlackFrames() { return m_iBlackFrames; }
  void IncrementBlackFrames() { m_iBlackFrames++; }
  void ResetBlackFrames() { m_iBlackFrames = 0; }
  int GetBlackSeconds() { return m_iBlackSeconds; }
  void IncrementBlackSeconds() { m_iBlackSeconds++; }
  void ResetBlackSeconds() { m_iBlackSeconds = 0; }
  unsigned long GetAdptLastUpdate() { return m_lAdptLastUpdate; }
  void SetAdptLastUpdate() { m_lAdptLastUpdate = millis(); }
  unsigned long GetGainLastUpdateUp() { return m_lGainLastUpdateUp; }
  void SetGainLastUpdateUp() { m_lGainLastUpdateUp = millis(); }
  unsigned long GetGainLastUpdateDown() { return m_lGainLastUpdateDown; }
  void SetGainLastUpdateDown() { m_lGainLastUpdateDown = millis(); }

  /* Frame/sector stats (for UI and debug) */
  int GetFrameWidth() { return m_frameWidth; }
  int GetFrameHeight() { return m_frameHeight; }
  int GetRealFrameWidth() { return m_realframeWidth; }
  int GetRealFrameHeight() { return m_realframeHeight; }
  int GetTopFirstLine() { return m_iTopFirstLine; }
  int GetBottomLastLine() { return m_iBottomLastLine; }
  int GetLeftFirstPixel() { return m_iLeftFirstPixel; }
  int GetRightLastPixel() { return m_iRightLastPixel; }
  bool GetInnerRectangle() { return m_bInnerRectangle; }
  int GetInnerTopLastLine() { return m_iInnerTopLastLine; }
  int GetInnerBottomFirstLine() { return m_iInnerBottomFirstLine; }
  int GetInnerLeftLastPixel() { return m_iInnerLeftLastPixel; }
  int GetInnerRightFirstPixel() { return m_iInnerRightFirstPixel; }
  int Getlev1size() { return m_lev1size; }
  int Getlev2size() { return m_lev2size; }
  byte GetColBlackRemoveTreshold() { return m_bColBlackRemoveTreshold; }
  int GetBelowWhiteSectors() { return m_iBelowWhiteSectors; }
  int GetTotalSectors() { return m_bSectors; }
  int GetWhiteSectors() { return m_iWhiteSectors; }
  int GetBlackSectors() { return m_iBlackSectors; }
  int GetTotalPixels() { return m_iTotalPixels; }
  int GetWhitePixels() { return m_iWhitePixels; }
  int GetBlackPixels() { return m_iBlackPixels; }
  int GetSkipPixels() { return m_iSkipPixels; }
  void IncrementSkipPixels() { m_iSkipPixels++; }
  void SetSkipPixels(int iNew) { m_iSkipPixels = iNew; }
  int GetAvrOutside() { return m_iAvrOutside; }
  void AddAvrOutside(int iAdd) { m_iAvrOutsideCtr += iAdd; }
  int GetAvrInside() { return m_iAvrInside; }
  void AddAvrInside(int iAdd) { m_iAvrInsideCtr += iAdd; }
  byte GetColActivationTreshold() { return m_bColActivationTreshold; }
  byte GetSectorsMatchCount() { return m_bSectorsMatchCount; }
  byte* GetSectorsMatch() { return &m_bSectorsMatch[0]; }

  /* Helpers for geometry (used by camCalcCamPointAreas / camCalcColorsProcessPixel). */
  float camCalcColorsArea(int x1, int y1, int x2, int y2, int x3, int y3);
  bool camCalcColorsIsPixelInside(float A, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, int x, int y);

private:
  void* m_pVoidCamleds;
  short* m_pshPixels;
  short* m_pshBlackPixels;
  int m_frameWidth;
  int m_frameHeight;
  int m_realframeWidth;
  int m_realframeHeight;

  bool m_bCameraPointsUpdated;
  campoint m_aCameraPoint[5]; /* index 1..4 used */

  int m_iColorBoostMaxPercent;
  byte m_bCamMode;
  byte m_bCamDetailLevel;
  byte m_bCamDetailSmooth;
  byte m_bCamDetailTop;
  byte m_bCamDetailRight;
  byte m_bCamDetailBottom;
  byte m_bCamDetailLeft;

  byte m_bCamSmooth;
  byte m_bCamSmoothEnable;
  byte m_bCamSmoothMaxFrames;
  byte m_bCamSmoothMaxDiff;
  byte m_bCamSmoothCurrentMaxStep[MAX_LEDS];
  byte m_bCamSmoothFrameStep[MAX_LEDS];
  byte m_bCamBlackThresholdSmoothing;

  float m_flCamInnerHori;
  float m_flCamInnerVert;

  byte m_bCurrentCamFps;
  byte m_bCurrentCamFramesCtr;
  int m_iBlackFrames;
  int m_iBlackSeconds;
  unsigned long m_lAdptLastUpdate;
  unsigned long m_lGainLastUpdateUp;
  unsigned long m_lGainLastUpdateDown;

  int m_lev1size;
  int m_lev2size;
  int m_iTopFirstLine;
  int m_iBottomLastLine;
  int m_iLeftFirstPixel;
  int m_iRightLastPixel;
  bool m_bInnerRectangle;
  int m_iInnerTopLastLine;
  int m_iInnerBottomFirstLine;
  int m_iInnerLeftLastPixel;
  int m_iInnerRightFirstPixel;

  int m_iaRed[MAX_LEDS];
  int m_iaGreen[MAX_LEDS];
  int m_iaBlue[MAX_LEDS];
  int m_iWhiteSectors;
  int m_iBelowWhiteSectors;
  int m_iBlackSectors;
  int m_iBlackPixels;
  int m_iWhitePixels;
  int m_iTotalPixels;
  byte m_bColCorRed;
  byte m_bColCorGreen;
  byte m_bColCorBlue;
  bool m_bColTresholdHit;
  byte m_bColActivationTreshold;
  byte m_bColBlackRemoveTreshold;
  byte m_bWhiteActivationTreshold;
  byte m_bBeforeWhiteActivationTreshold;
  byte m_bWhiteAdaptivePercent;
  int m_iWhiteAdaptiveChangeDelay;
  byte m_bSectors;
  byte m_bSectorSteps;
  byte m_bSectorStart;
  byte m_bSectorsMatchCount;
  byte m_bSectorsMatch[CAM_MAX_CALC_LEDS_ALL];
  byte m_bWhiteMinimumPercent;
  byte m_bCamAdaptiveGainDown;
  byte m_bCamAdaptiveGainUp;
  byte m_bCamAdaptiveGainMax;
  int m_iSkipPixels;
  int m_iAvrOutsideCtr;
  int m_iAvrOutside;
  int m_iAvrInsideCtr;
  int m_iAvrInside;

  void* m_pLedSmoothFrames; /* CRGB* for cam smoothing frames */
  int* m_pBlackDetected;
  int m_iBlackDetectedSize;
  int m_iCamCurrentJpegBlocks;
  bool m_bCamCalcJpegBlocks;
  byte* m_pCamPixelsInJpegBlocks;

  SemaphoreHandle_t m_cam_mutex;

  int* m_piCamPoints;
  rectanglepoints m_MainRectArea;
  rectanglepoints m_MainInnerRectArea;
  rectanglepoints* m_pRectAreas;
};

#endif /* ENABLE_CAM */
#endif /* camprocessor_h */
