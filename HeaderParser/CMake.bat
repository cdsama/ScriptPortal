@echo off
cd /d "%~dp0"

rem VS2017U2 contains vswhere.exe
if "%VSWHERE%"=="" set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
  set VS_HOME=%%i
)

if not exist Build (
    md Build
)

cd Build

cmake -G "Visual Studio 15 2017 Win64" ..

echo %VS_HOME%

"%VS_HOME%\Common7\IDE\devenv.com" "HeaderParser.sln" /project "INSTALL" /rebuild "Release|x64"

pause