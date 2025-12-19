@echo off
setlocal

:: Find Visual Studio
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo vswhere.exe not found. Is Visual Studio installed?
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)

if not defined VS_PATH (
    echo Visual Studio with C++ tools not found.
    exit /b 1
)

echo Found Visual Studio at: %VS_PATH%
call "%VS_PATH%\Common7\Tools\VsDevCmd.bat"

cd StardewValley
if not exist build mkdir build
cd build

cmake .. -G "Visual Studio 17 2022" -A Win32
if errorlevel 1 exit /b 1

cmake --build . --config Debug
if errorlevel 1 exit /b 1

:: The executable location varies, try to find it
if exist "bin\StardewValley\Debug\StardewValley.exe" (
    cd bin\StardewValley\Debug
    StardewValley.exe
) else if exist "Debug\StardewValley.exe" (
    cd Debug
    StardewValley.exe
) else (
    echo Could not find StardewValley.exe
    exit /b 1
)
