@echo off
setlocal

set UE_ROOT=C:\Games\Epic\UE_5.7
set PROJECT=C:\UnrealMCP\unreal-mcp-main\MCPGameProject\RobotzZz.uproject

echo [1/3] Killing Unreal Editor...
taskkill /F /IM UnrealEditor.exe >nul 2>nul
timeout /t 3 /nobreak >nul

echo [2/3] Building...
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" MCPGameProjectEditor Win64 Development -Project="%PROJECT%" -WaitMutex -FromMsBuild -architecture=x64
if %ERRORLEVEL% GEQ 3 (
    echo BUILD FAILED
    pause
    exit /b %ERRORLEVEL%
)

echo [3/3] Launching Editor...
start "" "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT%"
