@echo off
setlocal

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo [selfiestick] vswhere not found: "%VSWHERE%"
  exit /b 1
)

for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
if not defined VSINSTALL (
  echo [selfiestick] Visual Studio with C++ toolchain not found.
  exit /b 1
)

set "MSBUILD=%VSINSTALL%\MSBuild\Current\Bin\amd64\MSBuild.exe"
if not exist "%MSBUILD%" (
  echo [selfiestick] MSBuild not found: "%MSBUILD%"
  exit /b 1
)

set "ROOT=%~dp0"
set "SOLUTION=%ROOT%native_dll\selfiestick_hlae.sln"
set "BIN_DLL=%ROOT%bin\selfiestick_hlae.dll"
set "ZH_RELEASE=%ROOT%release\zh-CN"
set "ZH_RELEASE_DLL=__ZH_DLL__"
set "EN_RELEASE=%ROOT%release\en-US"
set "EN_RELEASE_DLL=Lyan's selfiestick.dll"

if not exist "%SOLUTION%" (
  echo [selfiestick] solution not found: "%SOLUTION%"
  exit /b 1
)

if not exist "%ZH_RELEASE%" mkdir "%ZH_RELEASE%"
if not exist "%EN_RELEASE%" mkdir "%EN_RELEASE%"

call :build_localized SELFIESTICK_LANG_ZH_CN zh-CN "%ZH_RELEASE%" "%ZH_RELEASE_DLL%"
if errorlevel 1 exit /b 1

call :build_localized SELFIESTICK_LANG_EN_US en-US "%EN_RELEASE%" "%EN_RELEASE_DLL%"
if errorlevel 1 exit /b 1

echo [selfiestick] chinese release package updated:
echo [selfiestick]   %ZH_RELEASE%
echo [selfiestick] english release package updated:
echo [selfiestick]   %EN_RELEASE%
exit /b 0

:build_localized
set "LANG_MACRO=%~1"
set "LANG_TAG=%~2"
set "TARGET_DIR=%~3"
set "TARGET_DLL=%~4"
if not defined TARGET_DLL set "TARGET_DLL=selfiestick_hlae.dll"

echo [selfiestick] building %LANG_TAG%...
"%MSBUILD%" "%SOLUTION%" /t:Rebuild /p:Configuration=Release /p:Platform=x64 /p:VCToolsVersion=14.44.35207 /p:SelfieStickLangMacro=%LANG_MACRO% /m /nologo
if errorlevel 1 (
  echo [selfiestick] build failed for %LANG_TAG%.
  exit /b 1
)

if not exist "%BIN_DLL%" (
  echo [selfiestick] output dll missing after %LANG_TAG% build: "%BIN_DLL%"
  exit /b 1
)

if /i "%TARGET_DLL%"=="__ZH_DLL__" (
  powershell -NoProfile -Command "$dllName = 'Lyan_CS2' + [char]0x81EA + [char]0x62CD + [char]0x6746 + '.dll'; $targetDir = '%TARGET_DIR%'; $target = Join-Path $targetDir $dllName; Copy-Item -Path '%BIN_DLL%' -Destination $target -Force; Get-ChildItem -Path $targetDir -Filter '*.dll' | Where-Object { $_.Name -ne $dllName } | Remove-Item -Force"
  if errorlevel 1 (
    echo [selfiestick] failed to copy dll into %TARGET_DIR%.
    exit /b 1
  )
) else (
  copy /y "%BIN_DLL%" "%TARGET_DIR%\%TARGET_DLL%" >nul
  if errorlevel 1 (
    echo [selfiestick] failed to copy dll into %TARGET_DIR%.
    exit /b 1
  )

  if /i not "%TARGET_DLL%"=="selfiestick_hlae.dll" (
    if exist "%TARGET_DIR%\selfiestick_hlae.dll" del /q "%TARGET_DIR%\selfiestick_hlae.dll"
  )
)

echo [selfiestick] %LANG_TAG% package ready.
exit /b 0