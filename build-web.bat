@echo off
REM Generate embedded web UI headers from web\HTML and web\HTML_MIN.
REM Run from repo root. After this, compile in Arduino IDE (or arduino-cli).

cd /d "%~dp0web"
call npm install
python convert.py ..\ambiwebserver_html_raw.h HTML
if errorlevel 1 exit /b 1
python convert.py ..\ambiwebserver_html_raw_min.h HTML_MIN
if errorlevel 1 exit /b 1
cd /d "%~dp0"
echo.
echo Web headers ready: ambiwebserver_html_raw.h, ambiwebserver_html_raw_min.h
echo You can now compile the firmware in Arduino IDE.
