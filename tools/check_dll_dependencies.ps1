param(
    [Parameter(Mandatory=$true)]
    [string]$DllPath,

    [string]$DumpbinPath = "D:\Visual studio 2022\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $DllPath)) {
    Write-Error "DLL not found: $DllPath"
}

if (-not (Test-Path -LiteralPath $DumpbinPath)) {
    Write-Error "dumpbin not found: $DumpbinPath"
}

$output = & $DumpbinPath /dependents $DllPath
$blocked = @(
    "MSVCP140.dll",
    "VCRUNTIME140.dll",
    "VCRUNTIME140_1.dll",
    "D3DCOMPILER_47.dll"
)

$blocked += @(
    "api-ms-win-crt-runtime-l1-1-0.dll",
    "api-ms-win-crt-stdio-l1-1-0.dll",
    "api-ms-win-crt-utility-l1-1-0.dll",
    "api-ms-win-crt-string-l1-1-0.dll",
    "api-ms-win-crt-heap-l1-1-0.dll",
    "api-ms-win-crt-convert-l1-1-0.dll",
    "api-ms-win-crt-math-l1-1-0.dll"
)

$found = @()
foreach ($dependency in $blocked) {
    if ($output -match [regex]::Escape($dependency)) {
        $found += $dependency
    }
}

if ($found.Count -gt 0) {
    Write-Host $output
    Write-Error ("Blocked load-time dependencies found: " + ($found -join ", "))
}

Write-Host "dependency check passed: $DllPath"
