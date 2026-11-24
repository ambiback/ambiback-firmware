# Drittcode

Die folgenden Komponenten liegen in diesem Repository und behalten ihre eigene Lizenz.

| Komponente | Version | Ort | Urheber | Lizenz |
|---|---|---|---|---|
| [esp32cam](https://github.com/yoursunny/esp32cam) (verändert) | 0.0.20240626 | `src/esp32cam/` | Junxiao Shi, Anpassungen Andre Lorbach | ISC, siehe `src/esp32cam/LICENSE` |
| [jsHue](https://github.com/blargoner/jshue) | 2.1.1 | `web/HTML/include_jshue.js` | John Peloquin und Mitwirkende | MIT |
| [jQuery](https://jquery.org/license) | 1.10.2 | `web/HTML/jquery_min.js` | jQuery Foundation | MIT |
| [jQuery Mobile](https://jquerymobile.com/) | 1.4.5 | `web/HTML/jquery_mobile_min.*` | jQuery Foundation | MIT |
| [jQuery UI](https://jqueryui.com/) | 1.10.3, 1.11.4, 1.12.1 | `web/jquery_ui_*`, `web/HTML/jquery_ui*` | jQuery Foundation und Mitwirkende | MIT |
| jQuery UI Touch Punch | 0.2.2 | `web/jquery_ui_touch_punch_min.js`, `web/HTML/jquery_ui_touch_punch_min.js` | Dave Furfero | MIT oder GPL-2.0, hier unter MIT genutzt |

## Änderungen an esp32cam

`src/esp32cam/` basiert auf dem Upstream-Stand der Version 0.0.20240626 und wurde für AmbiBack angepasst. Antiflicker, PLL, Zoom und `taskYIELD` stammen aus dem früheren AmbiBack-Fork von esp32cam. Die Bibliothek ist Teil des Sketches, eine separat installierte esp32cam wird nicht gebraucht. Die Originallizenz liegt unverändert bei. Die übrige Firmware steht unter GPL-3.0.
