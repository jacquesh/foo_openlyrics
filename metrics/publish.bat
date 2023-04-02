@echo off
REM Change to the script's directory: https://serverfault.com/questions/95686/change-current-directory-to-the-batch-file-directory
pushd "%~dp0"

SET ARCHIVE_NAME=published_metric_server.zip
SET TMP_DIR=publish_dir/

rm %ARCHIVE_NAME%
dotnet publish -o %TMP_DIR% metrics_server\OpenLyricsMetrics.csproj
C:\PROGRA~1\7-Zip\7z.exe a %ARCHIVE_NAME% .\publish_dir\*
rm -r %TMP_DIR%

popd
