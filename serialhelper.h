#ifndef serialhelper_h
# define serialhelper_h

class SerialHelper {
public:
    // Constructors
    SerialHelper ();

    // Declare helper functions
    void checkSerial(); 

    inline unsigned long GetLastUpdate() { return m_lLastUpdate; }
    void SetLastUpdate();

    /* Public for now */
    unsigned long m_lLastSerialTime; 
    size_t m_tzBufLen; // Used to store current buffer len
#if TCP_MSS < MAX_PAKET_LENGTH
    byte m_bBuf[MAX_PAKET_LENGTH];
#endif
private:
    // Declare private functions
    void InitSerial();
    void FlushSerialData();
    bool processLEDData(byte* pbBuf, int iIdx, int iExpPktLen);
    void commandInfo();
    void commandResetWifi();
    void commandSetWifi(char* pszSSID, char* pszKEY);
  
    // Declare helper variables
    bool m_bInit;
    unsigned long m_lLastUpdate;
    unsigned long m_lTimeSerialCheck;
};

#endif
