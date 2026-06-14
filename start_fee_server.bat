@echo off
setlocal enabledelayedexpansion

set "ROOT_DIR=%~dp0"
set "FQBN=arduino:avr:uno"
set "PORT=3000"
set "GATE_SERIAL_PORT=COM3"
set "SLOT_SERIAL_PORT=COM5"
set "LCD_SERIAL_PORT=COM7"
set "SERIAL_BAUD_RATE=9600"
set "HOST=0.0.0.0"
set "TAILSCALE_IP="
set "TAILSCALE_URL_FILE=%ROOT_DIR%tailscale_admin_url.txt"
set "SERVER_DIR="
set "UNO_GATE_SKETCH=%ROOT_DIR%sketches\uno_gate"
set "UNO_SLOTS_SKETCH=%ROOT_DIR%sketches\uno_slots"
set "UNO_LCD_SKETCH=%ROOT_DIR%sketches\uno_lcd"

for /d %%D in ("%ROOT_DIR%*") do (
  if exist "%%~fD\fee-server\package.json" (
    set "SERVER_DIR=%%~fD\fee-server"
  )
)

if "%SERVER_DIR%"=="" (
  echo Cannot find fee-server folder.
  pause
  exit /b 1
)

for /f "usebackq delims=" %%I in (`tailscale ip -4 2^>nul`) do (
  if not defined TAILSCALE_IP set "TAILSCALE_IP=%%I"
)

if not defined TAILSCALE_IP (
  for /f "usebackq delims=" %%I in (`powershell -NoProfile -Command "(Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.InterfaceAlias -eq 'Tailscale' } | Select-Object -First 1 -ExpandProperty IPAddress)"`) do (
    if not defined TAILSCALE_IP set "TAILSCALE_IP=%%I"
  )
)

where arduino-cli >nul 2>nul
if errorlevel 1 (
  echo arduino-cli not found.
  echo Install arduino-cli and make sure it is available in PATH.
  pause
  exit /b 1
)

if not exist "%UNO_GATE_SKETCH%\uno_gate.ino" (
  echo Cannot find Uno 1 sketch: %UNO_GATE_SKETCH%
  pause
  exit /b 1
)

if not exist "%UNO_SLOTS_SKETCH%\uno_slots.ino" (
  echo Cannot find Uno 2 sketch: %UNO_SLOTS_SKETCH%
  pause
  exit /b 1
)

if not exist "%UNO_LCD_SKETCH%\uno_lcd.ino" (
  echo Cannot find Uno 3 sketch: %UNO_LCD_SKETCH%
  pause
  exit /b 1
)

echo ========================================
echo Smart Car Park Auto Upload + Server Start
echo ========================================
echo ROOT_DIR=%ROOT_DIR%
echo SERVER_DIR=%SERVER_DIR%
echo FQBN=%FQBN%
echo GATE_SERIAL_PORT=%GATE_SERIAL_PORT%
echo SLOT_SERIAL_PORT=%SLOT_SERIAL_PORT%
echo LCD_SERIAL_PORT=%LCD_SERIAL_PORT%
echo SERIAL_BAUD_RATE=%SERIAL_BAUD_RATE%
echo HOST=%HOST%
echo PORT=%PORT%
echo TAILSCALE_IP=%TAILSCALE_IP%
echo TAILSCALE_URL_FILE=%TAILSCALE_URL_FILE%
echo.

echo [1/6] Compiling Uno 1 sketch...
call arduino-cli compile --fqbn %FQBN% "%UNO_GATE_SKETCH%"
if errorlevel 1 goto :compile_fail

echo [2/6] Uploading Uno 1 sketch to %GATE_SERIAL_PORT%...
call arduino-cli upload -p %GATE_SERIAL_PORT% --fqbn %FQBN% "%UNO_GATE_SKETCH%"
if errorlevel 1 goto :upload_fail

echo [3/6] Compiling Uno 2 sketch...
call arduino-cli compile --fqbn %FQBN% "%UNO_SLOTS_SKETCH%"
if errorlevel 1 goto :compile_fail

echo [4/6] Uploading Uno 2 sketch to %SLOT_SERIAL_PORT%...
call arduino-cli upload -p %SLOT_SERIAL_PORT% --fqbn %FQBN% "%UNO_SLOTS_SKETCH%"
if errorlevel 1 goto :upload_fail

echo [5/6] Compiling Uno 3 sketch...
call arduino-cli compile --fqbn %FQBN% "%UNO_LCD_SKETCH%"
if errorlevel 1 goto :compile_fail

echo [6/6] Uploading Uno 3 sketch to %LCD_SERIAL_PORT%...
call arduino-cli upload -p %LCD_SERIAL_PORT% --fqbn %FQBN% "%UNO_LCD_SKETCH%"
if errorlevel 1 goto :upload_fail

pushd "%SERVER_DIR%"
if errorlevel 1 (
  echo Failed to enter server folder.
  pause
  exit /b 1
)

if not exist "node_modules" (
  echo node_modules not found. Running npm install...
  call npm install
  if errorlevel 1 (
    echo npm install failed.
    popd
    pause
    exit /b 1
  )
)

echo.
echo Upload completed. Starting parking fee server...
echo Local URL: http://localhost:%PORT%/admin
if defined TAILSCALE_IP (
  echo Tailscale URL: http://%TAILSCALE_IP%:%PORT%/admin
) else (
  echo Tailscale IP was not detected.
)
echo.

start "Smart Car Park Server" cmd /k "cd /d ""%SERVER_DIR%"" && set PORT=%PORT% && set HOST=%HOST% && set GATE_SERIAL_PORT=%GATE_SERIAL_PORT% && set SLOT_SERIAL_PORT=%SLOT_SERIAL_PORT% && set LCD_SERIAL_PORT=%LCD_SERIAL_PORT% && set SERIAL_BAUD_RATE=%SERIAL_BAUD_RATE% && npm start"

timeout /t 5 >nul

if defined TAILSCALE_IP (
  > "%TAILSCALE_URL_FILE%" echo http://%TAILSCALE_IP%:%PORT%/admin
  echo Tailscale URL saved to: %TAILSCALE_URL_FILE%
  start "" "http://%TAILSCALE_IP%:%PORT%/admin"
) else (
  echo Tailscale URL file was not created because Tailscale IP was not detected.
  start "" "http://localhost:%PORT%/admin"
)

popd
pause
exit /b 0

:compile_fail
echo.
echo Arduino compile failed.
pause
exit /b 1

:upload_fail
echo.
echo Arduino upload failed.
echo Check COM port usage, board connection, and whether Serial Monitor is closed.
pause
exit /b 1
