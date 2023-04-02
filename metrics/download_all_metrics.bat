@echo off
REM Change to the script's directory: https://serverfault.com/questions/95686/change-current-directory-to-the-batch-file-directory
pushd "%~dp0"

SET OUTPUT_DIR=./metrics_data/
rm -r %OUTPUT_DIR%
aws s3 sync s3://foo-openlyrics-metrics %OUTPUT_DIR%

popd
