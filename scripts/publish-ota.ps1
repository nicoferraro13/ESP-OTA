param(
    [string]$Version,
    [string]$Notes,
    [switch]$Build,
    [string]$RepoOwner = "nicoferraro13",
    [string]$RepoName = "ESP-OTA"
)

$ErrorActionPreference = "Stop"

function Get-AppVersionFromIni {
    param(
        [string]$PlatformIoIniPath
    )

    $content = Get-Content -LiteralPath $PlatformIoIniPath -Raw
    $match = [regex]::Match($content, 'APP_VERSION=\\"([^\\"]+)\\"')
    if (-not $match.Success) {
        throw "No se pudo leer APP_VERSION desde platformio.ini."
    }

    return $match.Groups[1].Value
}

$projectRoot = Split-Path -Parent $PSScriptRoot
$platformIoIni = Join-Path $projectRoot "platformio.ini"
$buildFirmware = Join-Path $projectRoot ".pio\build\esp32devkitv1\firmware.bin"
$otaDir = Join-Path $projectRoot "ota"
$otaFirmware = Join-Path $otaDir "firmware.bin"
$manifestPath = Join-Path $otaDir "manifest.txt"

if (-not $Version) {
    $Version = Get-AppVersionFromIni -PlatformIoIniPath $platformIoIni
}

if (-not $Notes) {
    $Notes = "Actualizacion OTA $Version"
}

if ($Build) {
    $pioPath = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"
    if (-not (Test-Path -LiteralPath $pioPath)) {
        throw "No se encontro pio.exe en $pioPath."
    }

    & $pioPath run
    if ($LASTEXITCODE -ne 0) {
        throw "La compilacion de PlatformIO fallo."
    }
}

if (-not (Test-Path -LiteralPath $buildFirmware)) {
    throw "No existe el firmware compilado en $buildFirmware. Ejecuta primero un Build o usa -Build."
}

if (-not (Test-Path -LiteralPath $otaDir)) {
    New-Item -ItemType Directory -Path $otaDir | Out-Null
}

Copy-Item -LiteralPath $buildFirmware -Destination $otaFirmware -Force

$binUrl = "https://raw.githubusercontent.com/$RepoOwner/$RepoName/main/ota/firmware.bin"
$manifestLines = @(
    "# Generado por scripts/publish-ota.ps1",
    "version=$Version",
    "bin_url=$binUrl",
    "notes=$Notes"
)

Set-Content -LiteralPath $manifestPath -Value $manifestLines -Encoding ascii

Write-Host ""
Write-Host "OTA preparado correctamente."
Write-Host "Version: $Version"
Write-Host "Firmware copiado a: $otaFirmware"
Write-Host "Manifest actualizado en: $manifestPath"
Write-Host "URL esperada del binario: $binUrl"
Write-Host ""
Write-Host "Siguientes pasos:"
Write-Host "  git add ota/firmware.bin ota/manifest.txt"
Write-Host "  git commit -m `"Publica OTA $Version`""
Write-Host "  git push"
