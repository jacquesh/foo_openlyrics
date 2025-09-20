# This flattens all the JSON files into CSV files, combines them all into one big
# CSV file and then imports that into a sqlite database so that it can be queried.
# It requires the following cli tools:
# jq: https://jqlang.github.io/jq/
# xsv: https://github.com/BurntSushi/xsv
# sqlite3: https://www.sqlite.org/index.html

mkdir metrics_data/csv
mkdir metrics_data/errors
foreach($name in Get-ChildItem "metrics_data" -File) {
    $flattened_pairs = jq -r 'tostream | select(length==2) | (.[0] | join(\"-\")) as $key | .[1] as $value | {\"\($key)\": \"\($value)\"}' metrics_data\$name
    if($LastExitCode -ne 0) {
        echo "Error extracting from input file: $name"
        Move-Item -Path "metrics_data/$name" -Destination "metrics_data/errors/$name"
        continue
    }

    $csvified_data = $flattened_pairs | jq -r -s '. | reduce .[] as $item ({}; . + $item) | to_entries | (map(.key)), (map(.value)) | @csv'
    if($LastExitCode -ne 0) {
        echo "Error reducing from input file: $name"
    }

    Write-Output $csvified_data | Out-File ./metrics_data/csv/$name.csv -encoding utf8
}

$all_csvs = Get-ChildItem "metrics_data/csv"

$accum_in = "__tmp1.csv"
$accum_out = "__tmp2.csv"
Remove-Item -Force metrics_data/csv/$accum_in
Remove-Item -Force metrics_data/csv/$accum_out

touch metrics_data/csv/$accum_in
foreach($filename in $all_csvs) {
    if($filename.Name -eq $accum_in) { continue }
    if($filename.Name -eq $accum_out) { continue }

    xsv cat rows $filename.FullName metrics_data/csv/$accum_in -o metrics_data/csv/$accum_out

    $tmp = $accum_in
    $accum_in = $accum_out
    $accum_out = $tmp
}

Remove-Item -Force metrics.db
sqlite3 metrics.db ".import --csv metrics_data/csv/$accum_in openlyrics_metrics"

rm $name.csv
