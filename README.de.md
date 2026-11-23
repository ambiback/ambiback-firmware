# AmbiBack Firmware

[English](README.md) · **Deutsch**

ESP32-Firmware für AmbiBack. Sie steuert LED-Streifen passend zum Bildschirminhalt und verbindet DreamScreen, Philips Hue, Kamera und Netzwerk-Clients.

Sie ist der Kern des [AmbiBack-Projekts](https://github.com/ambiback). [AmbiBack UI](https://github.com/ambiback/ambiback-ui) und [AmbiBack Manager](https://github.com/ambiback/ambiback-manager) konfigurieren und steuern das Gerät, [AmbiBack PC](https://github.com/ambiback/ambiback-pc) kann ihm Farbdaten senden.

## Status

- Version im Quellcode: 0.5.231.
- Acht der 13 Gerätetypen bauen mit dem angepassten ESP32-Core 2.0.0, der für frühere Releases verwendet wurde. Dieser Core ist noch nicht veröffentlicht. `ttgo`, `ttgotm`, `ttgojr`, `wroover` und `ttgopl` bauen noch nicht. Mit dem öffentlichen Core 3.3.7 bauen die Kamera-Typen `m5esp32`, `m5camx` und `m5camtx` nicht, `m5camtx` überschreitet dabei die App-Partition.
- **Die Firmware wurde seit der Migration nicht auf echter Hardware getestet.**
- AmbiBack-Geräte werden seit rund fünf Jahren nicht mehr verkauft. Dieses Repository dient dem Eigenbau und der Weiterentwicklung, ohne Produktsupport. Mehr auf [ambiback.de](https://www.ambiback.de/).

## Betriebsarten

- **VIDEO:** empfängt Farbdaten per UDP oder TCP von externen Anwendungen.
- **CAM:** ermittelt die Farben mit der Kamera des Moduls.
- **AMBIENT:** zeigt statische Farben und Szenen.
- **RELAY:** leitet LED-Daten an andere AmbiBack-Geräte weiter.
- **DEMO:** dient zum Testen.

## Unterstützte Gerätetypen

Das Zielgerät wird in `globalincludes.h` über `ESP32_DEVICE_TYPE` gewählt. Pro Build ist genau ein Typ aktiv:

`ESP32_BRIDGE`, `ESP32_NAKED`, `ESP32_WROOVER_CAM`, `ESP32_TTGO_CAM`, `ESP32_TTGO_CAM_MIC`, `ESP32_TTGO_CAM_JOURNAL`, `ESP32_TTGO_CAM_PLUS`, `ESP32_M5ST_CAM`, `ESP32_M5ST_CAM_X`, `ESP32_M5ST_ATOMLITE`, `ESP32_M5ST_ATOMMATRIX`, `ESP32_AMBIBACK_LEDV1`, `ESP32_M5ST_CAM_TIMER_X`

## Bauteile

Diese Teile passen zu den Firmware-Zielen. Mit dem aktuellen Firmware-Stand sind sie noch nicht getestet.

- [M5Stack Atom Lite](https://www.amazon.de/dp/B0CTGKJPRW?tag=ambiback-21): LED-Controller, Ziel `ESP32_M5ST_ATOMLITE`, LED-Daten an GPIO 26. Der AtomS3 Lite wird nicht unterstützt.
- M5Stack Timer Camera X: Kamera, Ziel `ESP32_M5ST_CAM_TIMER_X`
- [BTF-LIGHTING WS2812B, 5 m, 60 LEDs/m](https://www.amazon.de/dp/B01CDTEJBG?tag=ambiback-21)
- [LEICKE Netzteil 5 V, 10 A](https://www.amazon.de/dp/B07YVBHH6K?tag=ambiback-21)
- [WAGO-Klemmen 221-415](https://www.amazon.de/dp/B078XL56P7?tag=ambiback-21)
- [Seeed Grove-Kabel, 4-polig](https://www.amazon.de/dp/B01CNZ9RJO?tag=ambiback-21)

Die vollständige Liste steht auf der [AmbiBack-Teileseite](https://www.ambiback.de/selbstbau/teile/). Platine und 3D-Druckdaten gibt es bei den [Baudaten](https://www.ambiback.de/selbstbau/druckdaten/).

*Die Amazon-Links sind Partnerlinks. Als Amazon-Partner verdiene ich an qualifizierten Verkäufen.*

## Bauen

Voraussetzungen sind die Arduino IDE oder `arduino-cli` mit ESP32-Board-Unterstützung sowie Node.js und Python für die Web-Oberfläche. Als externe Bibliotheken werden unter anderem AsyncTCP, ESPAsyncWebServer, FastLED und IRremoteESP8266 verwendet.

1. Abhängigkeiten der Web-Oberfläche in `web/` installieren: `npm install`
2. Die eingebetteten Web-Header erzeugen: `bash build-web.sh` (Windows: `build-web.bat`)
3. In `globalincludes.h` das Zielgerät wählen.
4. `AmbiBack.Controller.ino` kompilieren und flashen.

`ambiwebserver_html_raw.h` und `ambiwebserver_html_raw_min.h` werden erzeugt und nicht eingecheckt.

### Build-Einstellungen

Diese Werte lassen sich als Compiler-Flag überschreiben:

- `AMBIBACK_FIRMWARE_BASE_URL`: Basisadresse für Firmware-Updates. Der Gerätepfad wird automatisch angehängt. Die Verfügbarkeit des Standardservers ist nicht zugesichert, setze für eigene Builds daher eine eigene Adresse.
- `AMBIBACK_SYSLOG_SERVER`: IP-Adresse des Syslog-Servers. Standardmäßig leer, der Wert lässt sich auch im Gerät setzen.
- `AMBIBACK_AP_PASSWORD_SEED`: 32-Bit-Startwert für das erzeugte Access-Point-Passwort, definiert in `globalincludes.h`. Der Standard `0x0000FFFF` behält das Passwort bestehender Geräte bei. Setze für eigene Builds einen eigenen Wert, checke ihn nicht ein und verwende im AmbiBack Manager denselben Wert.

## Sicherheit

Geräte mit Access Point öffnen ihr Einrichtungsfenster nur, wenn noch kein WLAN eingerichtet ist, oder nach einer Anforderung direkt am Gerät: die Taste 4 bis 7 Sekunden halten oder den Strom dreimal im Abstand weniger Sekunden einschalten. Der AmbiBack Manager koppelt sich dann mit dem Gerät und speichert einen Geräteschlüssel. Nach der Kopplung brauchen WLAN-Einrichtung, Access-Point- und Debug-Einstellungen, Neustart, WPS und Firmware-Updates diesen Schlüssel.

Verbleibende Grenzen: Die Ersteinrichtung ist nicht dagegen geschützt, dass jemand in Funkreichweite sich zuerst koppelt. Das erzeugte Access-Point-Passwort wird weiter aus der MAC-Adresse abgeleitet, ein eigener Startwert ist kein vollwertiger Schutz, und die Übertragung ist unverschlüsselt. LED- und Modussteuerung bleiben im lokalen Netz offen, und Gerätetypen ohne Access Point haben keine Kopplung. Betreibe die Geräte nur in einem vertrauenswürdigen Netz.

## Marken

Philips Hue und DreamScreen sind Marken ihrer jeweiligen Inhaber. Dieses Projekt steht in keiner Verbindung zu ihnen.

## Lizenz

Copyright (C) 2020-2026 Andre Lorbach

Dieses Programm ist freie Software und steht unter der [GNU General Public License Version 3](LICENSE). Es wird ohne jede Gewährleistung bereitgestellt.

Enthaltener Drittcode behält seine eigene Lizenz, siehe [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
