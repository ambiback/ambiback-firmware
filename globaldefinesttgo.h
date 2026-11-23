/***************************************
 *  Custom PinOUTs
 **************************************/

#if ESP32_CUSTOM_CAM == TTGO_T_CAMERA_MIC_V16
  #define PWDN_GPIO_NUM       -1 
  #define RESET_GPIO_NUM      -1
  #define XCLK_GPIO_NUM       4
  #define SIOD_GPIO_NUM       18
  #define SIOC_GPIO_NUM       23
  
  #define Y9_GPIO_NUM         36
  #define Y8_GPIO_NUM         37
  #define Y7_GPIO_NUM         38
  #define Y6_GPIO_NUM         39
  #define Y5_GPIO_NUM         35
  #define Y4_GPIO_NUM         14
  #define Y3_GPIO_NUM         13
  #define Y2_GPIO_NUM         34
  #define VSYNC_GPIO_NUM      5
  #define HREF_GPIO_NUM       27
  #define PCLK_GPIO_NUM       25
  #define AS312_PIN           19
  
  #define I2C_SDA             21
  #define I2C_SCL             22

//todo
//https://github.com/Xinyuan-LilyGO/TTGO-Camer-Mic
  // Maybe voice commands?
  #define IIS_SCK             26
  #define IIS_WS              32
  #define IIS_DOUT            33

  // Set Display Type
  #define SSD130_MODLE_TYPE   GEOMETRY_128_64
#elif ESP32_CUSTOM_CAM == TTGO_T_CAMERA_V05 || ESP32_CUSTOM_CAM == TTGO_T_CAMERA_V1_7_2
  #define PWDN_GPIO_NUM       26
  #define RESET_GPIO_NUM      -1
  #define XCLK_GPIO_NUM       32
  #define SIOD_GPIO_NUM       13
  #define SIOC_GPIO_NUM       12
  
  #define Y9_GPIO_NUM         39
  #define Y8_GPIO_NUM         36
  #define Y7_GPIO_NUM         23
  #define Y6_GPIO_NUM         18
  #define Y5_GPIO_NUM         15
  #define Y4_GPIO_NUM         4
  #define Y3_GPIO_NUM         14
  #define Y2_GPIO_NUM         5
  #define VSYNC_GPIO_NUM      27
  #define HREF_GPIO_NUM       25
  #define PCLK_GPIO_NUM       19
  #define AS312_PIN           33
  
  #define I2C_SDA             21
  #define I2C_SCL             22

  #define SSD130_MODLE_TYPE   GEOMETRY_128_64
#elif ESP32_CUSTOM_CAM == TTGO_T_JORNAL
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     25
  #define SIOC_GPIO_NUM     23
  
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM       5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       17
  #define VSYNC_GPIO_NUM    22
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21
  
  #define I2C_SDA           14
  #define I2C_SCL           13

  #define SSD130_MODLE_TYPE   GEOMETRY_128_32
  
  #undef ENABLE_AS312
#elif ESP32_CUSTOM_CAM == TTGO_T_CAMERA_PLUS
  #define PWDN_GPIO_NUM       -1
  #define RESET_GPIO_NUM      -1
  #define XCLK_GPIO_NUM       4
  #define SIOD_GPIO_NUM       18
  #define SIOC_GPIO_NUM       23
  
  #define Y9_GPIO_NUM         36
  #define Y8_GPIO_NUM         37
  #define Y7_GPIO_NUM         38
  #define Y6_GPIO_NUM         39
  #define Y5_GPIO_NUM         35
  #define Y4_GPIO_NUM         26
  #define Y3_GPIO_NUM         13
  #define Y2_GPIO_NUM         34
  #define VSYNC_GPIO_NUM      5
  #define HREF_GPIO_NUM       27
  #define PCLK_GPIO_NUM       25
  
  #define I2C_SDA             -1
  #define I2C_SCL             -1

  #undef ENABLE_SSD1306
  #undef ENABLE_AS312
#elif ESP32_CUSTOM_CAM == M5_ESP32CAM
// SEE https://github.com/m5stack/M5Stack-Camera/blob/master/wifi/wifi_ap/main/config.h for correct PINLAYOUT!

  #define PWDN_GPIO_NUM       -1
  #define RESET_GPIO_NUM      15
  #define XCLK_GPIO_NUM       27
  #define SIOC_GPIO_NUM       23
  #define SIOD_GPIO_NUM       25
  
  #define VSYNC_GPIO_NUM      22
  #define HREF_GPIO_NUM       26
  #define PCLK_GPIO_NUM       21

  #define Y2_GPIO_NUM         17
  // 32 
  #define Y3_GPIO_NUM         35
  #define Y4_GPIO_NUM         34
  #define Y5_GPIO_NUM         5
  #define Y6_GPIO_NUM         39
  #define Y7_GPIO_NUM         18
  #define Y8_GPIO_NUM         36
  #define Y9_GPIO_NUM         19
  
  #define I2C_SDA             -1
  #define I2C_SCL             -1

  //XCLK 20MHz or 10MHz
  #define CAM_XCLK_FREQ       FREQ_20

  // Maybe voice commands?
  #define MIC_PIN_SPQ2410     32
  
  #undef ENABLE_SSD1306
  #undef ENABLE_AS312

#elif ESP32_CUSTOM_CAM == M5_ESP32CAM_X
// SEE  https://github.com/m5stack/M5Stack-Camera/blob/master/wifi/wifi_ap/main/config.h for correct PINLAYOUT!
//      https://docs.m5stack.com/#/en/unit/m5camera_x?id=specification

  #define PWDN_GPIO_NUM       -1
  #define RESET_GPIO_NUM      15
  #define XCLK_GPIO_NUM       27
  #define SIOC_GPIO_NUM       23
  #define SIOD_GPIO_NUM       22
  
  #define VSYNC_GPIO_NUM      25
  #define HREF_GPIO_NUM       26
  #define PCLK_GPIO_NUM       21

  #define Y2_GPIO_NUM         32
  #define Y3_GPIO_NUM         35
  #define Y4_GPIO_NUM         34
  #define Y5_GPIO_NUM         5
  #define Y6_GPIO_NUM         39
  #define Y7_GPIO_NUM         18
  #define Y8_GPIO_NUM         36
  #define Y9_GPIO_NUM         19
  
  #define I2C_SDA             -1
  #define I2C_SCL             -1

  //XCLK 20MHz or 10MHz
  #define CAM_XCLK_FREQ       FREQ_20

  // Maybe voice commands?
  #define MIC_PIN_SPQ2410     32
//  MIC(SPM1423)  M5Camera
//  SCL IO2
//  SDA IO4

  #undef ENABLE_SSD1306
  #undef ENABLE_AS312

#elif ESP32_CUSTOM_CAM == M5_ESP32CAM_TIMER_X
// SEE  https://github.com/m5stack/TimerCam-arduino for correct PINLAYOUT!
//      https://docs.m5stack.com/#/en/unit/m5camera_x?id=specification

  #define PWDN_GPIO_NUM       -1
  #define RESET_GPIO_NUM      15
  #define XCLK_GPIO_NUM       27
  #define SIOC_GPIO_NUM       23
  #define SIOD_GPIO_NUM       25
  
  #define VSYNC_GPIO_NUM      22
  #define HREF_GPIO_NUM       26
  #define PCLK_GPIO_NUM       21

  #define Y2_GPIO_NUM         32
  #define Y3_GPIO_NUM         35
  #define Y4_GPIO_NUM         34
  #define Y5_GPIO_NUM         5
  #define Y6_GPIO_NUM         39
  #define Y7_GPIO_NUM         18
  #define Y8_GPIO_NUM         36
  #define Y9_GPIO_NUM         19
  
  #define I2C_SDA             -1
  #define I2C_SCL             -1

  //XCLK 20MHz or 10MHz
  #define CAM_XCLK_FREQ       FREQ_20

  #undef ENABLE_SSD1306
  #undef ENABLE_AS312

#else
# error "Please select board type"
#endif
