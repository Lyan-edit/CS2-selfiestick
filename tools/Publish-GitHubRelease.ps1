[CmdletBinding()]
param(
    [string]$Version = 'v0.4.4',
    [string]$Repository = 'Lyan-edit/CS2-selfiestick',
    [string]$AssetDirectory,
    [string]$ReleaseNotesPath
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($AssetDirectory)) {
    $AssetDirectory = Join-Path $repoRoot (Join-Path 'dist' $Version)
}
if ([string]::IsNullOrWhiteSpace($ReleaseNotesPath)) {
    $ReleaseNotesPath = Join-Path $repoRoot 'release\RELEASE_NOTES.md'
}
if ($Version -notmatch '^v\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$') {
    throw "Invalid release version: $Version"
}
if ($Repository -notmatch '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$') {
    throw "Invalid GitHub repository: $Repository"
}

$assets = @(Get-ChildItem -LiteralPath $AssetDirectory -File | Sort-Object Name)
if ($assets.Count -ne 5 -or -not ($assets.Name -contains 'SHA256SUMS.txt')) {
    throw "Expected exactly five release assets, including SHA256SUMS.txt, in $AssetDirectory"
}
if (-not (Test-Path -LiteralPath $ReleaseNotesPath -PathType Leaf)) {
    throw "Release notes are missing: $ReleaseNotesPath"
}

$credentialText = "protocol=https`nhost=github.com`n`n" | & git credential fill
if ($LASTEXITCODE -ne 0) {
    throw 'git credential fill failed.'
}
$credential = @{}
foreach ($line in $credentialText) {
    $separator = $line.IndexOf('=')
    if ($separator -gt 0) {
        $credential[$line.Substring(0, $separator)] = $line.Substring($separator + 1)
    }
}
if (-not $credential.ContainsKey('password') -or [string]::IsNullOrWhiteSpace($credential.password)) {
    throw 'No GitHub credential was returned by git credential fill.'
}

$headers = @{
    Accept = 'application/vnd.github+json'
    Authorization = "Bearer $($credential.password)"
    'X-GitHub-Api-Version' = '2022-11-28'
    'User-Agent' = 'Lyan-CS2-selfiestick-release'
}
$apiRoot = "https://api.github.com/repos/$Repository"

$release = $null
try {
    $release = Invoke-RestMethod -Method Get -Uri "$apiRoot/releases/tags/$Version" -Headers $headers
}
catch {
    $statusCode = $null
    if ($_.Exception.Response) {
        $statusCode = [int]$_.Exception.Response.StatusCode
    }
    if ($statusCode -ne 404) {
        throw
    }
}
if ($null -eq $release) {
    # GitHub's tag endpoint omits draft releases, so search the authenticated
    # release list before creating a new draft for the same tag.
    $releaseList = Invoke-RestMethod -Method Get -Uri "$apiRoot/releases?per_page=100" -Headers $headers
    $release = $releaseList | Where-Object { $_.draft -and $_.tag_name -eq $Version } | Select-Object -First 1
}
if ($null -ne $release -and -not $release.draft) {
    throw "A published GitHub Release already exists for $Version; refusing to replace it."
}

$tagRef = Invoke-RestMethod -Method Get -Uri "$apiRoot/git/ref/tags/$Version" -Headers $headers
if (-not $tagRef.object.sha) {
    throw "Remote tag $Version was not found. Push the commit and tag before publishing."
}

$releaseBody = Get-Content -LiteralPath $ReleaseNotesPath -Raw -Encoding UTF8
$releasePayload = @{
    tag_name = $Version
    target_commitish = 'main'
    name = "CS2 Selfiestick $Version"
    body = $releaseBody
    draft = $true
    prerelease = $false
    generate_release_notes = $false
} | ConvertTo-Json -Depth 5
if ($null -eq $release) {
    $release = Invoke-RestMethod -Method Post -Uri "$apiRoot/releases" -Headers $headers -ContentType 'application/json; charset=utf-8' -Body $releasePayload
}
else {
    Write-Host "Resuming draft release: $($release.html_url)"
}

try {
    $uploadRoot = $release.upload_url -replace '\{\?name,label\}$', ''
    $uploadedAssets = foreach ($asset in $assets) {
        $uploaded = @($release.assets | Where-Object name -EQ $asset.Name) | Select-Object -First 1
        if ($null -eq $uploaded) {
            $encodedName = [Uri]::EscapeDataString($asset.Name)
            $uploaded = Invoke-RestMethod -Method Post -Uri "${uploadRoot}?name=$encodedName" -Headers $headers -ContentType 'application/octet-stream' -InFile $asset.FullName
        }
        if ([int64]$uploaded.size -ne [int64]$asset.Length) {
            throw "Uploaded size mismatch for $($asset.Name)."
        }
        [PSCustomObject]@{
            Local = $asset
            Remote = $uploaded
        }
    }

    $verifyRoot = Join-Path ([IO.Path]::GetTempPath()) ("selfiestick-release-verify-" + [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $verifyRoot -Force | Out-Null
    try {
        foreach ($entry in $uploadedAssets) {
            $downloadPath = Join-Path $verifyRoot $entry.Local.Name
            $downloadHeaders = $headers.Clone()
            $downloadHeaders.Accept = 'application/octet-stream'
            Invoke-WebRequest -Uri $entry.Remote.url -Headers $downloadHeaders -OutFile $downloadPath
            $localHash = (Get-FileHash -LiteralPath $entry.Local.FullName -Algorithm SHA256).Hash
            $remoteHash = (Get-FileHash -LiteralPath $downloadPath -Algorithm SHA256).Hash
            if ($localHash -ne $remoteHash) {
                throw "Remote SHA-256 mismatch for $($entry.Local.Name)."
            }
        }
    }
    finally {
        if (Test-Path -LiteralPath $verifyRoot) {
            Remove-Item -LiteralPath $verifyRoot -Recurse -Force
        }
    }

    $publishPayload = @{
        draft = $false
        prerelease = $false
        make_latest = 'true'
    } | ConvertTo-Json
    $published = Invoke-RestMethod -Method Patch -Uri "$apiRoot/releases/$($release.id)" -Headers $headers -ContentType 'application/json; charset=utf-8' -Body $publishPayload
    Write-Host "Published: $($published.html_url)"
    $uploadedAssets | ForEach-Object {
        [PSCustomObject]@{
            Name = $_.Local.Name
            Size = $_.Local.Length
            SHA256 = (Get-FileHash -LiteralPath $_.Local.FullName -Algorithm SHA256).Hash
        }
    }
}
catch {
    Write-Warning "Draft release remains available for inspection: $($release.html_url)"
    throw
}
