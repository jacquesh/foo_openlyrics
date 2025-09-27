function Ensure-DirectoryFromTarball {
    param (
        [Parameter(Mandatory = $true)]
        [string]$Url,

        [Parameter(Mandatory = $true)]
        [string]$ExpectedHash
    )

    # Strip the extension twice to handle .tar.gz (otherwise the result still has .tar)
    # This will still work with a file that's just a .tar though.
    $targetDirName = [System.IO.Path]::GetFileNameWithoutExtension([System.IO.Path]::GetFileNameWithoutExtension($Url))
    $targetDir = Join-Path $PSScriptRoot $targetDirName
    if (Test-Path -Path $targetDir -PathType Container) {
        Write-Host "Directory '$targetDir' already exists. Skipping download..."
        return
    }

    $tempFile = Join-Path $PSScriptRoot ([System.IO.Path]::GetFileName($Url))
    Write-Host "Downloading $Url to $tempFile..."
    try {
        Invoke-WebRequest -Uri $Url -OutFile $tempFile
    } catch {
        Write-Error "Download failed: $_"
        return
    }

    $actualHash = (Get-FileHash -Path $tempFile -Algorithm SHA256).Hash
    if ($actualHash -ne $ExpectedHash) {
        Remove-Item -Path $tempFile -Force
        Write-Error "Hash mismatch for $Url! Expected: $ExpectedHash, Got: $actualHash"
        return
    }
    Write-Host "Downloaded & verified. Extracting..."
    tar -xzf $tempFile -C $PSScriptRoot
    Remove-Item -Path $tempFile

    if (Test-Path -Path $targetDir -PathType Container) {
        Write-Host "Successfully extracted to '$targetDir'."
    } else {
        Write-Error "Extraction did not produce the expected directory '$targetDir'."
    }
}

Ensure-DirectoryFromTarball -Url "https://curl.se/download/curl-8.16.0.tar.gz" -ExpectedHash "A21E20476E39ECA5A4FC5CFB00ACF84BBC1F5D8443EC3853AD14C26B3C85B970"
Ensure-DirectoryFromTarball -Url "https://github.com/nghttp2/nghttp2/releases/download/v1.67.1/nghttp2-1.67.1.tar.gz" -ExpectedHash "DA8D640F55036B1F5C9CD950083248EC956256959DC74584E12C43550D6EC0EF"
