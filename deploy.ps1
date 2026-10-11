# deploy.ps1 - Copies newly built foo_openlyrics.dll to foobar2000 user-components
param(
    [switch]$RestartFoobar
)

$targetDir = "$env:APPDATA\foobar2000-v2\user-components\foo_openlyrics"
$targetDll = Join-Path $targetDir "foo_openlyrics.dll"
$sourceDll = Join-Path $PSScriptRoot "build\Release\foo_openlyrics.dll"

if (-not (Test-Path $sourceDll)) {
    Write-Error "Source DLL not found at $sourceDll. Please build Release first."
    exit 1
}

$fbProc = Get-Process -Name "foobar2000" -ErrorAction SilentlyContinue
if ($fbProc) {
    if ($RestartFoobar) {
        Write-Host "Closing foobar2000..."
        $fbProc | Stop-Process
        Start-Sleep -Seconds 1
    } else {
        Write-Warning "foobar2000 is running. Please close foobar2000 and re-run this script, or pass -RestartFoobar."
        exit 1
    }
}

if (Test-Path $targetDll) {
    Copy-Item -Path $targetDll -Destination "$targetDll.bak" -Force
}

Copy-Item -Path $sourceDll -Destination $targetDll -Force
Write-Host "Successfully installed foo_openlyrics.dll to $targetDll" -ForegroundColor Green

if ($RestartFoobar) {
    Write-Host "Restarting foobar2000..."
    $fbExe = "J:\Program Files (x86)\foobar2000\foobar2000.exe"
    if (Test-Path $fbExe) {
        Start-Process $fbExe
    }
}
