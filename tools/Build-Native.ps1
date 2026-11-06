[CmdletBinding()]
param(
    [ValidateSet('Dll', 'Validation', 'UiPreview')][string]$Target = 'Dll',
    [ValidateSet('zh-CN', 'en-US')][string]$Language = 'zh-CN'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installation) { throw 'Visual Studio C++ tools not found.' }
$toolset = Get-ChildItem -LiteralPath (Join-Path $installation 'VC\Tools\MSVC') -Directory |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
$sdkRoot = (Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots').KitsRoot10
$sdk = Get-ChildItem -LiteralPath (Join-Path $sdkRoot 'Include') -Directory |
    Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'um\Windows.h') } |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (-not $toolset -or -not $sdk) { throw 'C++ toolset or Windows SDK not found.' }
$compiler = Join-Path $toolset.FullName 'bin\Hostx64\x64\cl.exe'
$buildRoot = Join-Path $repoRoot "native_dll\bin\native-$Target-$Language"
New-Item -ItemType Directory -Path $buildRoot -Force | Out-Null

$previousInclude = $env:INCLUDE
$previousLib = $env:LIB
$previousPath = $env:PATH
try {
    $env:INCLUDE = @(
        (Join-Path $toolset.FullName 'include'),
        (Join-Path $sdk.FullName 'ucrt'), (Join-Path $sdk.FullName 'shared'),
        (Join-Path $sdk.FullName 'um'), (Join-Path $sdk.FullName 'winrt')
    ) -join ';'
    $env:LIB = @(
        (Join-Path $toolset.FullName 'lib\x64'),
        (Join-Path $sdkRoot "Lib\$($sdk.Name)\ucrt\x64"),
        (Join-Path $sdkRoot "Lib\$($sdk.Name)\um\x64")
    ) -join ';'
    $env:PATH = (Split-Path -Parent $compiler) + ';' + $previousPath
    $sources = @('compatibility_gate.cpp', 'schema_probe.cpp', 'setup_view_hook.cpp') |
        ForEach-Object { Join-Path $repoRoot "native_dll\selfiestick_hlae\$_" }
    $options = @('/nologo', '/O2', '/MT', '/EHsc', '/std:c++20', '/permissive-', '/utf-8',
        '/DNOMINMAX', '/DUNICODE', '/D_UNICODE', '/D_CRT_SECURE_NO_WARNINGS',
        ('/Fo' + $buildRoot + '\'))
    if ($Target -eq 'Validation') {
        $sources += Join-Path $repoRoot 'native_dll\selfiestick_hlae_validation\selfiestick_hlae_validation.cpp'
        $output = Join-Path $buildRoot 'selfiestick_validation.exe'
    } else {
        $sources += Join-Path $repoRoot 'native_dll\selfiestick_hlae\imgui_impl_gdi_software.cpp'
        if ($Target -eq 'Dll') {
            $sources += Join-Path $repoRoot 'native_dll\selfiestick_hlae\selfiestick_hlae.cpp'
            $options += '/LD'
            $output = Join-Path $buildRoot 'selfiestick_hlae.dll'
        } else {
            $sources += Join-Path $repoRoot 'native_dll\selfiestick_ui_preview\selfiestick_ui_preview.cpp'
            $output = Join-Path $buildRoot 'selfiestick_ui_preview.exe'
        }
        $sources += @('imgui.cpp', 'imgui_draw.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp', 'backends\imgui_impl_win32.cpp') |
            ForEach-Object { Join-Path $repoRoot "native_dll\third_party\imgui\$_" }
        $options += @('/DIMGUI_IMPL_WIN32_DISABLE_GAMEPAD',
            ('/I' + (Join-Path $repoRoot 'native_dll\third_party\imgui')),
            ('/I' + (Join-Path $repoRoot 'native_dll\third_party\imgui\backends')),
            $(if ($Language -eq 'zh-CN') { '/DSELFIESTICK_LANG_ZH_CN' } else { '/DSELFIESTICK_LANG_EN_US' }))
    }
    & $compiler @options ('/Fe' + $output) @sources /link /DEBUG:NONE kernel32.lib user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib
    if ($LASTEXITCODE -ne 0) { throw "Native $Target build failed ($LASTEXITCODE)." }
    Write-Output $output
} finally {
    $env:INCLUDE = $previousInclude
    $env:LIB = $previousLib
    $env:PATH = $previousPath
}
