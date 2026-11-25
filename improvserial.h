#ifndef improvserial_h
# define improvserial_h

// Improv Wi-Fi Serial (https://www.improv-wifi.com/serial/)
// Lets browser tools (AmbiBack Web Flasher, ESP Web Tools) set Wi-Fi over USB.
// Coexists with the AmbiBack "%CMD:" serial protocol: SerialHelper feeds every
// byte that is not part of a "%" command into ImprovSerial::FeedByte().

#if defined(ENABLE_IMPROV_SERIAL)

# define IMPROV_SERIAL_VERSION        1
# define IMPROV_MAX_PACKET            256

// Packet types
# define IMPROV_TYPE_CURRENT_STATE    0x01
# define IMPROV_TYPE_ERROR_STATE      0x02
# define IMPROV_TYPE_RPC              0x03
# define IMPROV_TYPE_RPC_RESULT       0x04

// States
# define IMPROV_STATE_AUTHORIZED      0x02
# define IMPROV_STATE_PROVISIONING    0x03
# define IMPROV_STATE_PROVISIONED     0x04

// Errors
# define IMPROV_ERROR_NONE            0x00
# define IMPROV_ERROR_INVALID_RPC     0x01
# define IMPROV_ERROR_UNKNOWN_RPC     0x02
# define IMPROV_ERROR_UNABLE_CONNECT  0x03
# define IMPROV_ERROR_UNKNOWN         0xFF

// RPC commands
# define IMPROV_CMD_WIFI_SETTINGS     0x01
# define IMPROV_CMD_GET_STATE         0x02
# define IMPROV_CMD_GET_INFO          0x03
# define IMPROV_CMD_GET_NETWORKS      0x04

class ImprovSerial {
public:
    // Feed one received byte; handles a packet once it is complete
    static void FeedByte(uint8_t bByte);
    // Read all pending Serial bytes (for builds without ENABLE_SERIAL_INPUT)
    static void Poll();
    // True while a packet is being received (after the "IMPROV" header).
    // SerialHelper must then pass '%' bytes here instead of parsing a command.
    static bool IsReceiving() { return m_stPos >= 6; }

private:
    static void HandlePacket(uint8_t bType, const uint8_t* pbData, size_t stLen);
    static void HandleRpc(const uint8_t* pbData, size_t stLen);
    static void HandleWifiSettings(const uint8_t* pbData, size_t stLen);
    static void SendState(uint8_t bState);
    static void SendError(uint8_t bError);
    static void SendRpcResult(uint8_t bCommand, const char** ppszStrings, size_t stCount);
    static void SendPacket(uint8_t bType, const uint8_t* pbData, size_t stLen);
    static uint8_t GetCurrentState();
    static void GetDeviceUrl(char* pszUrl, size_t stLen);

    static uint8_t m_bBuf[IMPROV_MAX_PACKET];
    static size_t m_stPos;
};

#endif // ENABLE_IMPROV_SERIAL
#endif // improvserial_h
