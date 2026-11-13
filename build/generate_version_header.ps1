param (
        [Parameter(Mandatory)]
        [string]$output_path
      )
$version_string = git describe --tags --first-parent HEAD
Write-Output "Computing version header from version string '$version_string'..."
if ([string]::IsNullOrEmpty($version_string)) {
    Write-Output "No version tag found, using development version..."
    $version_string = "v0.0-alpha"
}

if ($version_string[0] -eq 'v') {
    $version_string = $version_string.Substring(1) # Remove the leading 'v'
}

$output_dir = Split-Path -Parent $output_path
New-Item -ItemType "directory" -Force $output_dir | Out-Null
Write-Output "#pragma once`n`n#define OPENLYRICS_VERSION ""$version_string""" > $output_path
Write-Output "Version header saved to $output_path"
