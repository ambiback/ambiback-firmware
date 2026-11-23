#ifndef sectorgeometry_h
#define sectorgeometry_h

/* Shared sector geometry types and macros for camera (CamProcessor, LEDHelper cam-to-LED) and Hue. */

/* Helper struct to store campoints as % */
struct campoint {
  short x;
  short y;
};

struct rectangle {
  short top;
  short bottom;
  short left;
  short right;
};

struct rectanglepoints {
  campoint topleft;
  campoint topright;
  campoint bottomleft;
  campoint bottomright;
  short shPixelCount;
  short shPixelMinCount;
};

/* Minimum amount of pixels before a Sector is considered black */
#define MIN_DEFAULT_PIXEL_AMOUNT 25

enum CAMPOINTS {
  POINT_TOP_LEFT = 1,
  POINT_TOP_RIGHT = 2,
  POINT_BOTTOM_LEFT = 3,
  POINT_BOTTOM_RIGHT = 4,
};

/* Single color sector index. Shared by Hue/Cam sector calc. */
#define LEDSINGLECOLOR 0x00

/* Sector count: 4 sides * detailLevel + 1 (SingleColor index). Shared by Hue/Cam sector calc. */
#define CAM_SECTOR_COUNT(detailLevel) (((detailLevel) * 4) + 1)

/* NEED All Sides + Single Color Area Index = 0!
 * CAM_MAX_CALC_LEDS_ALL needs to be divisible through 8 (See memmove for m_pBlackDetected )!
 */
#define CAM_MAX_CALC_LEDS_ALL 64
#define CAM_MAX_CALC_LEDS_SIDE 15
#define CAM_MIN_CALC_LEDS_SIDE 3
#define CAM_DEFAULT_CALC_LEDS_SIDE 9

/* Configurable inner image exclusion */
#define CAM_MAX_CALC_INNER_IMAGE_HORI 0.16
#define CAM_MAX_CALC_INNER_IMAGE_VERT 0.16

/* Maximum Frames that can be smoothed */
#define CAM_SMOOTH_FRAMES_MIN 1
#define CAM_SMOOTH_FRAMES_MAX 8
#define CAM_SMOOTH_FRAMES_MIN_DIFF 8
#define CAM_SMOOTH_FRAMES_MAX_DIFF 240
#define CAM_SMOOTH_FRAMES_DEFAULT 4
#define CAM_SMOOTH_FRAMES_DIFF_DEFAULT 128
#define CAM_SMOOTH_MODE_CUSTOM 255

#endif /* sectorgeometry_h */
