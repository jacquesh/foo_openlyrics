param (
        [Parameter(Mandatory)]
        [string]$output_path
      )
$version_string = git describe HEAD
if ($version_string[0] -Ne 'v') {
    Write-Output "Current commit description ""$version_string"" does not have the expected format, terminating..."
    Return 1
}
$version_string = $version_string.Substring(1) # Remove the leading 'v'

$output_dir = Split-Path -Parent $output_path
New-Item -ItemType "directory" -Force $output_dir | Out-Null
Write-Output "#pragma once`n`n#define OPENLYRICS_VERSION ""$version_string""" > $output_path
