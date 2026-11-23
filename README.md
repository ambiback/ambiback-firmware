# AmbiBack Firmware

ESP32-Firmware für AmbiBack: Sie steuert LED-Streifen passend zum Bildschirminhalt und verbindet DreamScreen, Philips Hue, Kamera und Netzwerk-Clients.

Die AmbiBack-Geräte werden seit rund fünf Jahren nicht mehr verkauft. Dieses Repository stellt den Quellcode für den Eigenbau und die Weiterentwicklung bereit. Mehr zum Projekt steht auf [ambiback.de](https://www.ambiback.de/).

## Betriebsarten

- **VIDEO:** empfängt Farbdaten per UDP oder TCP von externen Anwendungen.
- **CAM:** ermittelt die Farben mit der Kamera des Moduls.
- **AMBIENT:** zeigt statische Farben und Szenen.
- **RELAY:** leitet LED-Daten an andere AmbiBack-Geräte weiter.
- **DEMO:** dient zum Testen.

## Unterstützte Geräte

Das Zielgerät wird in `globalincludes.h` über `ESP32_DEVICE_TYPE` gewählt. Pro Build ist genau ein Typ aktiv:

`ESP32_BRIDGE`, `ESP32_NAKED`, `ESP32_WROOVER_CAM`, `ESP32_TTGO_CAM`, `ESP32_TTGO_CAM_MIC`, `ESP32_TTGO_CAM_JOURNAL`, `ESP32_TTGO_CAM_PLUS`, `ESP32_M5ST_CAM`, `ESP32_M5ST_CAM_X`, `ESP32_M5ST_ATOMLITE`, `ESP32_M5ST_ATOMMATRIX`, `ESP32_AMBIBACK_LEDV1`, `ESP32_M5ST_CAM_TIMER_X`

## Bauen

Voraussetzungen sind die Arduino IDE oder `arduino-cli` mit ESP32-Board-Unterstützung sowie Node.js und Python für die Web-Oberfläche. Als externe Bibliotheken werden unter anderem AsyncTCP, ESPAsyncWebServer, FastLED und IRremoteESP8266 verwendet.

1. Abhängigkeiten der Web-Oberfläche in `web/` installieren: `npm install`
2. Die eingebetteten Web-Header erzeugen: `./build-web.sh` (Windows: `build-web.bat`)
3. In `globalincludes.h` das Zielgerät wählen.
4. Die Firmware `AmbiBack.Controller.ino` kompilieren und flashen.

`ambiwebserver_html_raw.h` und `ambiwebserver_html_raw_min.h` werden erzeugt und nicht eingecheckt.

### Eigene Build-Einstellungen

Diese Werte lassen sich als Compiler-Flag überschreiben:

- `AMBIBACK_FIRMWARE_BASE_URL`: Basisadresse für Firmware-Updates. Standard ist `http://firmware.ambiback.com/firmware/esp32`, der Gerätepfad wird automatisch angehängt.
- `AMBIBACK_SYSLOG_SERVER`: IP-Adresse des Syslog-Servers. Standardmäßig ist keine eingetragen, der Wert lässt sich auch im Gerät setzen.

## Lizenz

Copyright (C) 2020-2026 Andre Lorbach

Dieses Programm ist freie Software und steht unter der [GNU General Public License Version 3](LICENSE). Es wird ohne jede Gewährleistung bereitgestellt.

Enthaltener Drittcode behält seine eigene Lizenz, siehe [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
