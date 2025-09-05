@echo off
REM Smart Lamp Controller - Windows Batch Script
REM Usage: lamp_control.bat [IP_ADDRESS] [COMMAND] [TIMER_MINUTES]
REM Commands: on, off, status, timer
REM Example: lamp_control.bat 192.168.1.100 on
REM Example: lamp_control.bat 192.168.1.100 timer 30

setlocal enabledelayedexpansion

REM Check if curl is available
curl --version >nul 2>&1
if errorlevel 1 (
    echo Error: curl is required but not found. Please install curl or use Windows 10/11.
    pause
    exit /b 1
)

REM Parse arguments
set LAMP_IP=%1
set COMMAND=%2
set TIMER_MINUTES=%3

REM If no arguments, show menu
if "%LAMP_IP%"=="" goto :show_menu
if "%COMMAND%"=="" goto :show_menu

REM Execute command
if "%COMMAND%"=="on" goto :turn_on
if "%COMMAND%"=="off" goto :turn_off
if "%COMMAND%"=="status" goto :get_status
if "%COMMAND%"=="timer" goto :set_timer
echo Invalid command: %COMMAND%
echo Valid commands: on, off, status, timer
exit /b 1

:show_menu
cls
echo.
echo ============================================
echo        Smart Lamp Controller
echo ============================================
echo.
if "%LAMP_IP%"=="" (
    set /p LAMP_IP=Enter lamp IP address: 
    if "!LAMP_IP!"=="" (
        echo IP address cannot be empty!
        pause
        goto :show_menu
    )
)

echo Current lamp IP: %LAMP_IP%
echo.
echo 1. Turn lamp ON
echo 2. Turn lamp OFF  
echo 3. Check status
echo 4. Set timer
echo 5. Quick timer - 5 minutes
echo 6. Quick timer - 30 minutes
echo 7. Quick timer - 1 hour
echo 8. Cancel timer
echo 9. Change IP address
echo 0. Exit
echo.
set /p choice=Select option (0-9): 

if "%choice%"=="1" goto :turn_on
if "%choice%"=="2" goto :turn_off
if "%choice%"=="3" goto :get_status
if "%choice%"=="4" goto :set_timer_menu
if "%choice%"=="5" set TIMER_MINUTES=5 && goto :set_timer
if "%choice%"=="6" set TIMER_MINUTES=30 && goto :set_timer
if "%choice%"=="7" set TIMER_MINUTES=60 && goto :set_timer
if "%choice%"=="8" set TIMER_MINUTES=0 && goto :set_timer
if "%choice%"=="9" set LAMP_IP= && goto :show_menu
if "%choice%"=="0" exit /b 0

echo Invalid choice!
pause
goto :show_menu

:set_timer_menu
echo.
set /p TIMER_MINUTES=Enter timer minutes (1-720, 0 to cancel): 
if "!TIMER_MINUTES!"=="" set TIMER_MINUTES=0
goto :set_timer

:turn_on
echo Turning lamp ON...
curl -s -f "http://%LAMP_IP%/on" >nul
if errorlevel 1 (
    echo Error: Could not connect to lamp at %LAMP_IP%
    echo Make sure the IP address is correct and lamp is online.
) else (
    echo Lamp turned ON successfully!
)
goto :continue

:turn_off
echo Turning lamp OFF...
curl -s -f "http://%LAMP_IP%/off" >nul
if errorlevel 1 (
    echo Error: Could not connect to lamp at %LAMP_IP%
    echo Make sure the IP address is correct and lamp is online.
) else (
    echo Lamp turned OFF successfully!
)
goto :continue

:get_status
echo Getting lamp status...
for /f "delims=" %%i in ('curl -s -f "http://%LAMP_IP%/status" 2^>nul') do set json_response=%%i
if "!json_response!"=="" (
    echo Error: Could not connect to lamp at %LAMP_IP%
    echo Make sure the IP address is correct and lamp is online.
    goto :continue
)

echo.
echo Status: %json_response%
echo.
REM Simple status display
echo %json_response% | findstr "true" >nul
if not errorlevel 1 (
    echo Lamp is currently ON
) else (
    echo Lamp is currently OFF
)
echo %json_response% | findstr "timeoutActive.*true" >nul
if not errorlevel 1 (
    echo Timer is active
) else (
    echo No timer active
)
goto :continue

:set_timer
if "%TIMER_MINUTES%"=="" goto :set_timer_menu
if %TIMER_MINUTES% lss 0 set TIMER_MINUTES=0
if %TIMER_MINUTES% gtr 720 set TIMER_MINUTES=720

if %TIMER_MINUTES% equ 0 (
    echo Cancelling timer...
) else (
    echo Setting timer for %TIMER_MINUTES% minutes...
)

curl -s -f "http://%LAMP_IP%/timeout?minutes=%TIMER_MINUTES%" >nul
if errorlevel 1 (
    echo Error: Could not connect to lamp at %LAMP_IP%
    echo Make sure the IP address is correct and lamp is online.
) else (
    if %TIMER_MINUTES% equ 0 (
        echo Timer cancelled successfully!
    ) else (
        echo Timer set for %TIMER_MINUTES% minutes. Lamp will turn ON and auto-off!
    )
)
goto :continue

:continue
if "%1"=="" (
    echo.
    pause
    goto :show_menu
)
exit /b 0
