@echo off
setlocal enabledelayedexpansion

set vswhere="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set vswhereArgs=-latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64

rem Find Visual Studio version and path

for /f "usebackq tokens=*" %%i in (`%vswhere% %vswhereArgs% -property installationPath`) do (
	set VSInstallPath=%%i
)

for /f "usebackq tokens=*" %%i in (`%vswhere% %vswhereArgs% -property installationVersion`) do (
	set VSVersion=%%i
)

if exist "%VSInstallPath%\VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt" (
	set /p Version=<"%VSInstallPath%\VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt"
	set VCVersion=!Version: =!
)

rem Find Window SDK version and path

set WinSDKkey="HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\Windows\v10.0"

for /f "usebackq skip=2 tokens=3" %%a in (`reg query %WinSDKkey% /v "ProductVersion" 2^>nul`) do (
	set WinSDKVersion=%%a
)

rem Write to the paths file

if not exist build (mkdir build)
pushd build
echo #once> paths.bff
echo .VSInstallPath = '%VSInstallPath%'>> paths.bff
echo .VSVersion = '%VSVersion%'>> paths.bff
echo .VCVersion = '%VCVersion%'>> paths.bff
echo .WindowsSDKBasePath = 'C:\Program Files (x86)\Windows Kits\10'>> paths.bff
echo .WindowsSDKVersion = '%WinSDKVersion%.0'>> paths.bff
popd
