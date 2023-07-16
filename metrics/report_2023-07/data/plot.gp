set term pngcairo notransparent truecolor
set datafile separator ","
set style fill solid
set nokey

# This is the "Set 1" palette from https://github.com/Gnuplotting/gnuplot-palettes
set linetype 1 lc rgb '#E41A1C' # red
set linetype 2 lc rgb '#377EB8' # blue
set linetype 3 lc rgb '#4DAF4A' # green
set linetype 4 lc rgb '#984EA3' # purple
set linetype 5 lc rgb '#FF7F00' # orange
set linetype 6 lc rgb '#FFFF33' # yellow
set linetype 7 lc rgb '#A65628' # brown
set linetype 8 lc rgb '#F781BF' # pink

# This is the "Set 2" palette from https://github.com/Gnuplotting/gnuplot-palettes
set linetype 1 lc rgb '#66C2A5' # teal
set linetype 2 lc rgb '#FC8D62' # orange
set linetype 3 lc rgb '#8DA0CB' # lilac
set linetype 4 lc rgb '#E78AC3' # magentat
set linetype 5 lc rgb '#A6D854' # lime green
set linetype 6 lc rgb '#FFD92F' # banana
set linetype 7 lc rgb '#E5C494' # tan
set linetype 8 lc rgb '#B3B3B3' # grey

set boxwidth 0.9 
set xtics scale 0
unset xlabel
unset ylabel
set format y "%g%%"
set yrange [0:115]


# SELECT
# 	DISTINCT metrics_ol_cfg_display_scroll_dir,
# 	100.0 * (COUNT(*) OVER (PARTITION BY metrics_ol_cfg_display_scroll_dir)) / (COUNT(*) OVER ()) as frac
# FROM metrics
# WHERE metrics_ol_num_panels >= 1
set xrange [-1:2]
set title "Scroll direction"
set output 'out_scroll_dir.png'
plot "export_scroll_dir.csv" using 0:2:($0+1):xtic(1) with boxes linecolor variable title columnhead, \
         '' using 0:2:(sprintf("%.2f%%", $2)) with labels offset 0,1 title columnhead


#WITH aggregated_versions AS (
#	SELECT
#		CASE
#		WHEN metrics_fb2k_version = "foobar2000 v2.0" THEN "v2.0"
#		WHEN metrics_fb2k_version LIKE "foobar2000 v2.1 preview%"  THEN "v2.0 beta"
#		WHEN metrics_fb2k_version LIKE "foobar2000 v2.0 beta%"  THEN "v2.0 beta"
#		WHEN metrics_fb2k_version LIKE "foobar2000 v1.5%" THEN "v1.5.*"
#		WHEN metrics_fb2k_version LIKE "foobar2000 v1.6%" THEN "v1.6.*"
#		ELSE metrics_fb2k_version
#		END AS version
#	FROM metrics
#	WHERE metrics_ol_num_panels > 0
#	)
#SELECT
#	DISTINCT version,
#	100.0 * (COUNT(*) OVER (PARTITION BY version)) / (COUNT(*) OVER ()) as frac
#FROM aggregated_versions
set xrange [-1:4]
set title "foobar2000 version"
set output 'out_fb2kversions.png'
plot "export_fb2kversions.csv" using 0:2:($0+1):xtic(1) with boxes linecolor variable title columnhead, \
         '' using 0:2:(sprintf("%.2f%%", $2)) with labels offset 0,1 title columnhead


#SELECT
#	DISTINCT metrics_fb2k_is_dark_mode,
#	100.0 * (COUNT(*) OVER (PARTITION BY metrics_fb2k_is_dark_mode)) / (COUNT(*) OVER ()) as frac
#FROM metrics
#WHERE metrics_ol_num_panels >= 1
set xrange [-1:2]
set title "UI colour mode"
set output 'out_darkmode.png'
plot "export_darkmode.csv" using 0:2:($0+1):xtic(1) with boxes linecolor variable title columnhead, \
         '' using 0:2:(sprintf("%.2f%%", $2)) with labels offset 0,1 title columnhead


#SELECT
#	DISTINCT metrics_fb2k_is_portable,
#	100.0 * (COUNT(*) OVER (PARTITION BY metrics_fb2k_is_portable)) / (COUNT(*) OVER ()) as frac
#FROM metrics
#WHERE metrics_ol_num_panels >= 1
set xrange [-1:2]
set title "fb2k installation portability"
set output 'out_portable.png'
plot "export_portable.csv" using 0:2:($0+1):xtic(1) with boxes linecolor variable title columnhead, \
         '' using 0:2:(sprintf("%.2f%%", $2)) with labels offset 0,1 title columnhead


#WITH sources AS (
#	SELECT 'Local files' AS Source FROM metrics WHERE metrics_ol_cfg_search_sources LIKE '%Local files%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT 'Metadata tags' AS Source FROM metrics WHERE metrics_ol_cfg_search_sources LIKE '%Metadata tags%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT 'Musixmatch' AS Source FROM metrics WHERE metrics_ol_cfg_search_sources LIKE '%Musixmatch%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT 'QQ Music' AS Source FROM metrics WHERE metrics_ol_cfg_search_sources LIKE '%QQ Music%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT 'NetEase Online Music' AS Source FROM metrics WHERE metrics_ol_cfg_search_sources LIKE '%NetEase Online Music%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT 'AZLyrics.com' AS Source FROM metrics WHERE metrics_ol_cfg_search_sources LIKE '%AZLyrics.com%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT 'Metal-Archives.com' AS Source FROM metrics WHERE metrics_ol_cfg_search_sources LIKE '%Metal-Archives.com%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT 'DarkLyrics.com' AS Source FROM metrics WHERE metrics_ol_cfg_search_sources LIKE '%DarkLyrics.com%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT 'Genius.com' AS Source FROM metrics WHERE metrics_ol_cfg_search_sources LIKE '%Genius.com%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT '<unknown>' AS Source FROM metrics WHERE metrics_ol_cfg_search_sources LIKE '%<unknown>%' AND metrics_ol_num_panels > 0
#)
#SELECT
#	DISTINCT Source,
#	100.0 * (COUNT(*) OVER (PARTITION BY Source)) / 2105.0 as frac
#FROM sources
set xrange [-1:9]
set xtics rotate by -70 left
set title "Sources enabled"
set output 'out_sources.png'
plot "export_sources.csv" using 0:2:($0+1):xtic(1) with boxes linecolor variable title columnhead, \
         '' using 0:2:(sprintf("%.2f%%", $2)) with labels offset 0,1 font ",10" title columnhead


#WITH accumlocal AS (
#	SELECT
#		metrics_ol_cfg_search_sources,
#		CASE
#		WHEN metrics_ol_cfg_search_sources LIKE "Local files%" THEN 1
#		WHEN metrics_ol_cfg_search_sources LIKE "Metadata%" THEN 1
#		ELSE 0
#		END AS search_local_first
#	FROM metrics
#	WHERE metrics_ol_num_panels > 0
#	)
#SELECT
#	DISTINCT search_local_first,
#	100.0 * (COUNT(*) OVER (PARTITION BY search_local_first)) / (COUNT(*) OVER ()) as frac
#FROM accumlocal
set xrange [-1:2]
set title "Do users have their save source configured as the first search source?"
set output 'out_localfirst.png'
plot "export_localfirst.csv" using 0:2:($0+1):xtic(1) with boxes linecolor variable title columnhead, \
         '' using 0:2:(sprintf("%.2f%%", $2)) with labels offset 0,1 title columnhead


#SELECT
#	DISTINCT metrics_ol_cfg_save_src,
#	100.0 * (COUNT(*) OVER (PARTITION BY metrics_ol_cfg_save_src)) / (COUNT(*) OVER ()) as frac
#FROM metrics
#WHERE metrics_ol_num_panels >= 1
set xrange [-1:3]
set title "Save source"
set output 'out_savesrc.png'
plot "export_savesrc.csv" using 0:2:($0+1):xtic(1) with boxes linecolor variable title columnhead, \
         '' using 0:2:(sprintf("%.2f%%", $2)) with labels offset 0,1 title columnhead


#SELECT
#	DISTINCT metrics_ol_num_panels,
#	100.0 * (COUNT(*) OVER (PARTITION BY metrics_ol_num_panels)) / (COUNT(*) OVER ()) as frac
#FROM metrics
set xrange [-1:5]
set title "Number of lyrics panels"
set output 'out_numpanels.png'
plot "export_numpanels.csv" using 0:2:($0+1):xtic(1) with boxes linecolor variable title columnhead, \
         '' using 0:2:(sprintf("%.2f%%", $2)) with labels offset 0,1 title columnhead



#WITH autoedits AS (
#	SELECT 'Unknown' AS autoedit FROM metrics WHERE metrics_ol_cfg_search_auto_edit LIKE '%0%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT 'CreateInstrumental' AS autoedit FROM metrics WHERE metrics_ol_cfg_search_auto_edit LIKE '%1%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT 'Replace HTML characters' AS autoedit FROM metrics WHERE metrics_ol_cfg_search_auto_edit LIKE '%2%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT 'Remove repeated spaces' AS autoedit FROM metrics WHERE metrics_ol_cfg_search_auto_edit LIKE '%3%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT 'Remove repeated blank lines' AS autoedit FROM metrics WHERE metrics_ol_cfg_search_auto_edit LIKE '%4%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT 'Remove all blank lines' AS autoedit FROM metrics WHERE metrics_ol_cfg_search_auto_edit LIKE '%5%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT 'Reset capitalisation' AS autoedit FROM metrics WHERE metrics_ol_cfg_search_auto_edit LIKE '%6%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT 'Fix bad timestamps' AS autoedit FROM metrics WHERE metrics_ol_cfg_search_auto_edit LIKE '%7%' AND metrics_ol_num_panels > 0
#	UNION ALL
#	SELECT 'Remove timestamps' AS autoedit FROM metrics WHERE metrics_ol_cfg_search_auto_edit LIKE '%8%' AND metrics_ol_num_panels > 0
#)
#SELECT
#	DISTINCT autoedit,
#	100.0 * (COUNT(*) OVER (PARTITION BY autoedit)) / 2105.0 as frac
#FROM autoedits
set xrange [-1:7]
set xtics rotate by -70 left
set title "Automatically-run auto-edits"
set output 'out_autoedits.png'
plot "export_autoedits.csv" using 0:2:($0+1):xtic(1) with boxes linecolor variable title columnhead, \
         '' using 0:2:(sprintf("%.2f%%", $2)) with labels offset 0,1 title columnhead
set xtics norotate autojustify
set nokey


#SELECT
#	DISTINCT metrics_ol_cfg_save_autosave,
#	100.0 * (COUNT(*) OVER (PARTITION BY metrics_ol_cfg_save_autosave)) / (COUNT(*) OVER ()) as frac
#FROM metrics
#WHERE metrics_ol_num_panels >= 1
set xrange [-1:4]
set title "Auto-save mode"
set output 'out_autosave.png'
plot "export_autosave.csv" using 0:2:($0+1):xtic(1) with boxes linecolor variable title columnhead, \
         '' using 0:2:(sprintf("%.2f%%", $2)) with labels offset 0,1 title columnhead


#SELECT
#	DISTINCT metrics_ol_cfg_save_dir_type,
#	100.0 * (COUNT(*) OVER (PARTITION BY metrics_ol_cfg_save_dir_type)) / (COUNT(*) OVER ()) as frac
#FROM metrics
#WHERE metrics_ol_num_panels >= 1
set xrange [-1:3]
set title "Save directory type"
set output 'out_savedir.png'
plot "export_savedir.csv" using 0:2:($0+1):xtic(1) with boxes linecolor variable title columnhead, \
         '' using 0:2:(sprintf("%.2f%%", $2)) with labels offset 0,1 title columnhead


#WITH dpistats AS (
#SELECT
#	CASE metrics_os_dpi_x WHEN 96 THEN '96' ELSE 'Over 96' END AS dpi,
#	metrics_ol_num_panels
#FROM metrics
#)
#SELECT
#	DISTINCT dpi,
#	100.0 * (COUNT(*) OVER (PARTITION BY dpi)) / (COUNT(*) OVER ()) as frac
#FROM dpistats
#WHERE metrics_ol_num_panels >= 1
set xrange [-1:2]
set title "Screen DPI"
set output 'out_dpi.png'
plot "export_dpi.csv" using 0:2:($0+1):xtic(1) with boxes linecolor variable title columnhead, \
         '' using 0:2:(sprintf("%.2f%%", $2)) with labels offset 0,1 title columnhead


#SELECT
#	DISTINCT metrics_ol_cfg_display_scroll_type,
#	100.0 * (COUNT(*) OVER (PARTITION BY metrics_ol_cfg_display_scroll_type)) / (COUNT(*) OVER ()) as frac
#FROM metrics
#WHERE metrics_ol_num_panels >= 1
set xrange [-1:2]
set title "Scroll type"
set output 'out_scrolltype.png'
plot "export_scrolltype.csv" using 0:2:($0+1):xtic(1) with boxes linecolor variable title columnhead, \
         '' using 0:2:(sprintf("%.2f%%", $2)) with labels offset 0,1 title columnhead


#WITH timebuckets AS (
#	SELECT
#		CASE
#		WHEN metrics_ol_cfg_display_scroll_time = "1.7976931348623157e+308" THEN "Continuous"
#		WHEN metrics_ol_cfg_display_scroll_time > 0.5 THEN "More than 0.5"
#		WHEN metrics_ol_cfg_display_scroll_time = 0.5 THEN "0.5"
#		WHEN metrics_ol_cfg_display_scroll_time < 0.5 THEN "Less than 0.5"
#		ELSE "-1"
#		END AS timebucket
#	FROM metrics
#)
#SELECT timebucket, COUNT(*) FROM timebuckets GROUP BY timebucket
set xrange [-1:4]
set title "Scroll time"
set output 'out_scrolltime.png'
plot "export_scrolltime.csv" using 0:2:($0+1):xtic(1) with boxes linecolor variable title columnhead, \
         '' using 0:2:(sprintf("%.2f%%", $2)) with labels offset 0,1 title columnhead


#WITH hashvers AS (
#	SELECT
#		metrics_ol_num_panels,
#		CASE metrics_ol_version
#		WHEN '99ad88156180b5005281bec5c01aee3c7c6d41572f1c318085ea4c4cf58e4a6b' THEN 'v1.5 x64'
#		WHEN 'a1852f3d73ff8945f21b1771594d3fca54dd3fcfbacb687c62ed2ed0dddc6e86' THEN 'v1.5 x86'
#		WHEN '9a6d4dc4036caa44888079c36ab890f8e125831c8d60d1deb5e3c436107ce7ad' THEN 'v1.6 x64'
#		WHEN '715c3da7dd4cbd2d938739704e4b95a40161c09931c6d24b4b0e0ebcb20d61f7' THEN 'v1.6 x86'
#		ELSE 'Unknown'
#		END AS version
#	FROM metrics
#)
#SELECT
#	DISTINCT version,
#	100.0 * (COUNT(*) OVER (PARTITION BY version)) / (COUNT(*) OVER ()) as frac
#FROM hashvers
#WHERE metrics_ol_num_panels >= 1
set xrange [-1:5]
set title "OpenLyrics version"
set output 'out_openlyricsver.png'
plot "export_openlyricsver.csv" using 0:2:($0+1):xtic(1) with boxes linecolor variable title columnhead, \
         '' using 0:2:(sprintf("%.2f%%", $2)) with labels offset 0,1 title columnhead


#SELECT
#	SUBSTR(ingest_time_utc, 0, 11) AS in_date,
#	COUNT(*) AS cnt
#FROM metrics
#WHERE metrics_ol_num_panels > 0
#GROUP BY in_date
#ORDER BY in_date ASC
set format y "%g"
set yrange [0:50]
set boxwidth 1.0 
set xrange [-1:101]
set noxtics
set title "Metrics submissions by date"
set output 'out_indate.png'
plot "export_indate.csv" using 0:2:xtic(35) with boxes linecolor 1 title columnhead
set format y "%g%%"
set yrange [0:115]
set boxwidth 0.9 
set xtics norotate autojustify
