@echo off
setlocal enabledelayedexpansion

set "GATE_SERIAL_PORT=COM3"
set "SLOT_SERIAL_PORT=COM5"
set "LCD_SERIAL_PORT=COM7"
set "SERIAL_BAUD_RATE=9600"
set "SERVER_DIR="

for /d %%D in ("%~dp0*") do (
  if exist "%%~fD\fee-server\package.json" (
    set "SERVER_DIR=%%~fD\fee-server"
  )
)

if "%SERVER_DIR%"=="" (
  echo Cannot find fee-server folder.
  pause
  exit /b 1
)

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
    pause
    exit /b 1
  )
)

echo Starting parking fee server...
echo SERVER_DIR=%SERVER_DIR%
echo GATE_SERIAL_PORT=%GATE_SERIAL_PORT%
echo SLOT_SERIAL_PORT=%SLOT_SERIAL_PORT%
echo LCD_SERIAL_PORT=%LCD_SERIAL_PORT%
echo SERIAL_BAUD_RATE=%SERIAL_BAUD_RATE%
echo.

call npm start

popd
pause
