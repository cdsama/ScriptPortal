@echo off
cd /d "%~dp0"

REM get visual studio installation directory from registry
for /f "tokens=1,2*" %%a in ('reg query "HKEY_CURRENT_USER\SOFTWARE\Microsoft\VisualStudio\14.0_Config" /v "InstallDir" ^| findstr "InstallDir"') do (
set VS_HOME=%%c
)

if not exist Build (
    md Build
)

cd Build

cmake -G "Visual Studio 14 2015 Win64" ..

echo %VS_HOME%

"%VS_HOME%devenv.com" "lua.sln" /project "INSTALL" /rebuild "Debug|x64"
"%VS_HOME%devenv.com" "lua.sln" /project "INSTALL" /rebuild "Release|x64"

pause