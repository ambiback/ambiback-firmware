#ifndef eepromhelper_h
#define eepromhelper_h

#include <Preferences.h>

/* First 4 EEPROM bytes are ZERO and reserved */
/* TODO: Save Checksum of whole EEPROM into first 4 bytes */
#define EEPROM_START              0
#define WLAN_SSID_START           (4)
#define WLAN_KEY_START            (WLAN_SSID_START        + WLAN_SSID_MAX)
#define DEVICE_NAME_START         (WLAN_KEY_START         + WLAN_KEY_MAX)
#define COLOR_COR_RED_START       (DEVICE_NAME_START      + DEVICE_NAME_MAX)
#define COLOR_COR_GREEN_START     (COLOR_COR_RED_START    + BYTE_VARIABLE_MAX)
#define COLOR_COR_BLUE_START      (COLOR_COR_GREEN_START  + BYTE_VARIABLE_MAX)
#define COLOR_BRIGHTNESS_START    (COLOR_COR_BLUE_START   + BYTE_VARIABLE_MAX)
#define COLOR_MIN_LUMIN_START     (COLOR_BRIGHTNESS_START + INT_VARIABLE_MAX)
#define FRAME_LENGTH_START        (COLOR_MIN_LUMIN_START  + BYTE_VARIABLE_MAX)
#define DELAY_LENGTH_START        (FRAME_LENGTH_START     + BYTE_VARIABLE_MAX)
#define SETUP_VERTICAL_START      (DELAY_LENGTH_START     + INT_VARIABLE_MAX)
#define SETUP_HORIZONTAL_START    (SETUP_VERTICAL_START   + BYTE_VARIABLE_MAX)
#define SETUP_DIRECTION_START     (SETUP_HORIZONTAL_START + BYTE_VARIABLE_MAX)
#define COLOR_BOOST_START         (SETUP_DIRECTION_START  + BYTE_VARIABLE_MAX)
#define COLOR_VALUE_GAIN_START    (COLOR_BOOST_START      + BYTE_VARIABLE_MAX)
#define COLOR_BOOST_MIN_START     (COLOR_VALUE_GAIN_START + BYTE_VARIABLE_MAX)
#define SETUP_SIDELEFT_START      (COLOR_BOOST_MIN_START + BYTE_VARIABLE_MAX)
#define SETUP_SIDETOP_START       (SETUP_SIDELEFT_START + BYTE_VARIABLE_MAX)
#define SETUP_SIDERIGHT_START     (SETUP_SIDETOP_START + BYTE_VARIABLE_MAX)
#define SETUP_SIDEBOTTOM_START    (SETUP_SIDERIGHT_START + BYTE_VARIABLE_MAX)
#define FRAME_SMOOTHING_START     (SETUP_SIDEBOTTOM_START + BYTE_VARIABLE_MAX)

/* add new configs here */

#if defined(ENABLE_CAM)
# define CAM_POINT_1_START        (FRAME_SMOOTHING_START  + INT_VARIABLE_MAX)
# define CAM_POINT_2_START        (CAM_POINT_1_START      + INT_VARIABLE_MAX)
# define CAM_POINT_3_START        (CAM_POINT_2_START      + INT_VARIABLE_MAX)
# define CAM_POINT_4_START        (CAM_POINT_3_START      + INT_VARIABLE_MAX)
# define CAM_INDEX_START          (CAM_POINT_4_START      + INT_VARIABLE_MAX)
# define CAM_CONTRAST_START       (CAM_INDEX_START        + INT_VARIABLE_MAX)
# define CAM_BRIGHTNESS_START     (CAM_CONTRAST_START     + INT_VARIABLE_MAX)
# define CAM_SATURATION_START     (CAM_BRIGHTNESS_START   + INT_VARIABLE_MAX)
# define CAM_SPECIALEFFECT_START  (CAM_SATURATION_START   + INT_VARIABLE_MAX)
# define CAM_WBMODE_START         (CAM_SPECIALEFFECT_START+ INT_VARIABLE_MAX)
# define CAM_AELEVELS_START       (CAM_WBMODE_START       + INT_VARIABLE_MAX)
# define CAM_QUALITY_START        (CAM_AELEVELS_START     + INT_VARIABLE_MAX)
# define CAM_AEC2_START           (CAM_QUALITY_START      + INT_VARIABLE_MAX)
# define CAM_MODE_START           (CAM_AEC2_START         + BYTE_VARIABLE_MAX)
# define CAM_AECVALUE_START       (CAM_MODE_START         + INT_VARIABLE_MAX)
# define CAM_COLOR_RED_START      (CAM_AECVALUE_START     + BYTE_VARIABLE_MAX)
# define CAM_COLOR_GREEN_START    (CAM_COLOR_RED_START    + BYTE_VARIABLE_MAX)
# define CAM_COLOR_BLUE_START     (CAM_COLOR_GREEN_START  + BYTE_VARIABLE_MAX)
# define CAM_COLOR_ACTRSHLD_START (CAM_COLOR_BLUE_START   + BYTE_VARIABLE_MAX)
# define CAM_PRESET_START         (CAM_COLOR_ACTRSHLD_START+ INT_VARIABLE_MAX)
# define CAM_DETAILLEV_START      (CAM_PRESET_START       + BYTE_VARIABLE_MAX)
# define CAM_AWBGAIN_START        (CAM_DETAILLEV_START    + INT_VARIABLE_MAX)
# define CAM_HMIRROR_START        (CAM_AWBGAIN_START      + INT_VARIABLE_MAX)
# define CAM_VVLIP_START          (CAM_HMIRROR_START      + INT_VARIABLE_MAX)

/* LAST Config variable, needs to incremented if new values are being added */
# define LAST_CONFIG             (CAM_VVLIP_START+INT_VARIABLE_MAX)
#else
/* LAST Config variable, needs to incremented if new values are being added */
# define LAST_CONFIG             (FRAME_SMOOTHING_START+INT_VARIABLE_MAX)
#endif

class EEPROMHelper{
public:
    // Constructors
    EEPROMHelper();
    ~EEPROMHelper();

    // Declare helper functions
    int GetEepromSize() { return m_iEepromSize; }
    unsigned long GetLastUpdate() { return m_lLastUpdate; }
    void SetLastUpdate() { m_lLastUpdate = millis(); }

    void LoadSettings(); 

    /* Set Functions */
    void SetSettingString(int startaddress, char* psz, size_t stMaxLength);
    void SetSettingString(int startaddress, char* psz, size_t  stLength, size_t stMaxLength);
    void SetSettingString(int startaddress, String str, size_t stMaxLength);
    void SetSettingInt(int startaddress, int value);
    void SetSettingUInt(int startaddress, unsigned int value);
    void SetSettingByte(int startaddress, byte value);
    
    void SetSettingString(const char* pszSettingID, char* psz, size_t stMaxLength);
    void SetSettingString(const char* pszSettingID, char* psz, size_t  stLength, size_t stMaxLength);
    void SetSettingString(const char* pszSettingID, String str, size_t stMaxLength);
    void SetSettingByte(const char* pszSettingID, byte value);
    void SetSettingInt(const char* pszSettingID, int value);
    void SetSettingFloat(const char* pszSettingID, float value);
    
    float GetSettingFloat(const char* pszSettingID, float flDefault, bool bShowDebug);
    float GetSettingFloat(const char* pszSettingID, float flDefault) { return this->GetSettingFloat(pszSettingID, flDefault, true); }
    int GetSettingInt(const char* pszSettingID, int iDefault, bool bShowDebug);
    int GetSettingInt(const char* pszSettingID, int iDefault) { return this->GetSettingInt(pszSettingID, iDefault, true); }
    byte GetSettingByte(const char* pszSettingID, byte bDefault, bool bShowDebug);
    byte GetSettingByte(const char* pszSettingID, byte bDefault) { return this->GetSettingByte(pszSettingID, bDefault, true); }

    // Writes changes to EEPROM finally
    void CommitChanges(); 
    void FactoryReset(); 

private:
    // Declare helper variables
    int m_iEepromSize;
    unsigned long m_lLastUpdate; 
};

#endif 
