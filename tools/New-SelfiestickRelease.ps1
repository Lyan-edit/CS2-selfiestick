[CmdletBinding()]
param(
    [string]$Version = 'v0.4.4',
    [string]$Cs2Build = '26144732',
    [string]$HlaeVersion = '2.192.10'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$outputRoot = Join-Path $repoRoot (Join-Path 'dist' $Version)
$stageRoot = Join-Path $outputRoot '_stage'

if ($Version -notmatch '^v\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$') {
    throw "Invalid release version: $Version"
}
if ($Cs2Build -notmatch '^\d+$') {
    throw "Invalid CS2 build id: $Cs2Build"
}
if ($HlaeVersion -notmatch '^\d+\.\d+\.\d+$') {
    throw "Invalid HLAE version: $HlaeVersion"
}

$expectedOutputRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot (Join-Path 'dist' $Version)))
if ([IO.Path]::GetFullPath($outputRoot) -ne $expectedOutputRoot) {
    throw 'Refusing to clean an unexpected release output directory.'
}
if (Test-Path -LiteralPath $outputRoot) {
    Remove-Item -LiteralPath $outputRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $stageRoot -Force | Out-Null

$chineseDllName = 'Lyan_CS2' + [char]0x81EA + [char]0x62CD + [char]0x6746 + '.dll'
$chineseManualName = [string]::Concat([char]0x64CD, [char]0x4F5C, [char]0x624B, [char]0x518C, '.md')
$englishSource = Join-Path $repoRoot "release\en-US\Lyan's selfiestick.dll"
$chineseSource = Join-Path (Join-Path $repoRoot 'release\zh-CN') $chineseDllName
$englishAsset = Join-Path $outputRoot "Lyan_selfiestick-$Version-EN.dll"
$chineseAsset = Join-Path $outputRoot "Lyan_selfiestick-$Version-zh-CN.dll"
$binaryZip = Join-Path $outputRoot "Lyan_selfiestick-$Version-CS2-$Cs2Build-HLAE-$HlaeVersion-windows-x64.zip"
$sourceZip = Join-Path $outputRoot "Lyan_selfiestick-$Version-source.zip"
$checksumFile = Join-Path $outputRoot 'SHA256SUMS.txt'

foreach ($requiredFile in @(
    $englishSource,
    $chineseSource,
    (Join-Path $repoRoot 'release\en-US\Manual.md'),
    (Join-Path $repoRoot 'release\en-US\selfiestick_bind.cfg'),
    (Join-Path $repoRoot 'release\en-US\selfiestick.ini'),
    (Join-Path (Join-Path $repoRoot 'release\zh-CN') $chineseManualName),
    (Join-Path $repoRoot 'release\zh-CN\selfiestick_bind.cfg'),
    (Join-Path $repoRoot 'release\zh-CN\selfiestick.ini'),
    (Join-Path $repoRoot 'release\RELEASE_NOTES.md')
)) {
    if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) {
        throw "Required release file is missing: $requiredFile"
    }
}

Copy-Item -LiteralPath $englishSource -Destination $englishAsset
Copy-Item -LiteralPath $chineseSource -Destination $chineseAsset

$packageRoot = Join-Path $stageRoot "CS2-selfiestick-$Version"
$packageEnglish = Join-Path $packageRoot 'en-US'
$packageChinese = Join-Path $packageRoot 'zh-CN'
New-Item -ItemType Directory -Path $packageEnglish, $packageChinese -Force | Out-Null
Copy-Item -LiteralPath $englishSource -Destination (Join-Path $packageEnglish "Lyan's selfiestick.dll")
Copy-Item -LiteralPath (Join-Path $repoRoot 'release\en-US\Manual.md') -Destination $packageEnglish
Copy-Item -LiteralPath (Join-Path $repoRoot 'release\en-US\selfiestick_bind.cfg') -Destination $packageEnglish
Copy-Item -LiteralPath (Join-Path $repoRoot 'release\en-US\selfiestick.ini') -Destination $packageEnglish
Copy-Item -LiteralPath $chineseSource -Destination (Join-Path $packageChinese $chineseDllName)
Copy-Item -LiteralPath (Join-Path (Join-Path $repoRoot 'release\zh-CN') $chineseManualName) -Destination $packageChinese
Copy-Item -LiteralPath (Join-Path $repoRoot 'release\zh-CN\selfiestick_bind.cfg') -Destination $packageChinese
Copy-Item -LiteralPath (Join-Path $repoRoot 'release\zh-CN\selfiestick.ini') -Destination $packageChinese
Copy-Item -LiteralPath (Join-Path $repoRoot 'release\RELEASE_NOTES.md') -Destination $packageRoot
Compress-Archive -LiteralPath $packageRoot -DestinationPath $binaryZip -CompressionLevel Optimal

$sourceRoot = Join-Path $stageRoot "CS2-selfiestick-$Version-source"
New-Item -ItemType Directory -Path $sourceRoot -Force | Out-Null
$trackedAndUntracked = @(& git -C $repoRoot -c core.quotepath=false ls-files --cached --others --exclude-standard)
if ($LASTEXITCODE -ne 0 -or $trackedAndUntracked.Count -eq 0) {
    throw 'git ls-files failed while creating the source package.'
}
foreach ($relativePath in $trackedAndUntracked) {
    if ([string]::IsNullOrWhiteSpace($relativePath)) {
        continue
    }
    $sourcePath = Join-Path $repoRoot $relativePath
    if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
        continue
    }
    $targetPath = Join-Path $sourceRoot $relativePath
    $targetDirectory = Split-Path -Parent $targetPath
    New-Item -ItemType Directory -Path $targetDirectory -Force | Out-Null
    Copy-Item -LiteralPath $sourcePath -Destination $targetPath
}
Compress-Archive -LiteralPath $sourceRoot -DestinationPath $sourceZip -CompressionLevel Optimal

$assetsToHash = @($englishAsset, $chineseAsset, $binaryZip, $sourceZip)
$checksumLines = foreach ($asset in $assetsToHash) {
    $hash = (Get-FileHash -LiteralPath $asset -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $([IO.Path]::GetFileName($asset))"
}
[IO.File]::WriteAllLines($checksumFile, $checksumLines, [Text.UTF8Encoding]::new($false))

Remove-Item -LiteralPath $stageRoot -Recurse -Force

Write-Host "Release package ready: $outputRoot"
Get-ChildItem -LiteralPath $outputRoot -File |
    Sort-Object Name |
    Select-Object Name, Length, @{Name = 'SHA256'; Expression = { (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }}
