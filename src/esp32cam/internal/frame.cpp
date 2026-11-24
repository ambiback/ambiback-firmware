#include "frame.hpp"
#include "config.hpp"

#include <Arduino.h>
#include <esp_camera.h>
#include <AsyncTCP.h>

namespace esp32cam {

static const int PIXFORMAT_BMP = -101;

struct Frame::CameraFbT : public camera_fb_t {};

Frame::Frame() = default;

Frame::Frame(void* fb)
  : m_fb(static_cast<CameraFbT*>(fb))
  , m_data(m_fb->buf)
  , m_size(m_fb->len)
  , m_width(m_fb->width)
  , m_height(m_fb->height)
  , m_pixFormat(m_fb->format) {}

Frame::~Frame() {
  releaseFb();
  if (m_ownedData && m_data != nullptr) {
    free(m_data);
    m_data = nullptr;
    m_ownedData = false;
  }
}

void
Frame::releaseFb() {
  if (m_fb != nullptr) {
    esp_camera_fb_return(m_fb);
    m_fb = nullptr;
    m_data = nullptr;
    m_ownedData = false;
  }
}

void
Frame::setData(uint8_t* data, size_t size, bool takeOwnership) {
  releaseFb();
  m_data = data;
  m_size = size;
  m_ownedData = takeOwnership;
  m_pixFormat = PIXFORMAT_RGB888;
}

bool
Frame::writeTo(AsyncClient& client, int timeout) {
  if (m_data == nullptr || m_size == 0) return false;
  auto startTime = millis();
  size_t written = 0;
  while (written < m_size && client.connected() && client.canSend()) {
    size_t n = client.write(reinterpret_cast<const char*>(m_data + written), m_size - written);
    if (n == 0) return false;
    written += n;
    if (millis() - startTime > static_cast<unsigned long>(timeout)) return false;
    taskYIELD();
  }
  return written == m_size;
}

bool
Frame::writeTo(Print& os, int timeout) {
  return writeToImpl(os, timeout, nullptr);
}

bool
Frame::writeTo(Client& os, int timeout) {
  return writeToImpl(os, timeout, &os);
}

bool
Frame::writeToImpl(Print& os, int timeout, Client* client) {
  auto startTime = millis();
  for (size_t i = 0; i < m_size; i += os.write(&m_data[i], m_size - i)) {
    if (millis() - startTime > static_cast<unsigned long>(timeout) ||
        (client != nullptr && !client->connected())) {
      return false;
    }
    taskYIELD();
  }
  return true;
}

bool
Frame::isJpeg() const {
  return m_pixFormat == PIXFORMAT_JPEG;
}

bool
Frame::toJpeg(int quality) {
  uint8_t* data;
  size_t size;
  bool ok = fmt2jpg(m_data, m_size, m_width, m_height, static_cast<pixformat_t>(m_pixFormat),
                    detail::convertJpegQuality(quality), &data, &size);
  if (!ok) {
    return false;
  }
  releaseFb();
  m_data = data;
  m_size = size;
  m_pixFormat = PIXFORMAT_JPEG;
  return true;
}

bool
Frame::isBmp() const {
  return m_pixFormat == PIXFORMAT_BMP;
}

bool
Frame::toBmp() {
  uint8_t* data;
  size_t size;
  bool ok =
    fmt2bmp(m_data, m_size, m_width, m_height, static_cast<pixformat_t>(m_pixFormat), &data, &size);
  if (!ok) {
    return false;
  }
  releaseFb();
  m_data = data;
  m_size = size;
  m_pixFormat = PIXFORMAT_BMP;
  return true;
}

} // namespace esp32cam
