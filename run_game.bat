@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
cd "C:\Users\A\Desktop\test-of-StardewValley\StardewValley\build"
msbuild StardewValley.sln /p:Configuration=Debug /p:Platform=Win32
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b %ERRORLEVEL%
)
cd "bin\StardewValley\Debug"
start StardewValley.exe
