#include "stdafx.h"

#define OPENLYRICS_VERSION "0.4-WIP"

// Declaration of your component's version information
// Since foobar2000 v1.0 having at least one of these in your DLL is mandatory to let the troubleshooter tell different versions of your component apart.
// Note that it is possible to declare multiple components within one DLL, but it's strongly recommended to keep only one declaration per DLL.
// As for 1.1, the version numbers are used by the component update finder to find updates; for that to work, you must have ONLY ONE declaration per DLL. If there are multiple declarations, the component is assumed to be outdated and a version number of "0" is assumed, to overwrite the component with whatever is currently on the site assuming that it comes with proper version numbers.
DECLARE_COMPONENT_VERSION(
    "OpenLyrics",
    OPENLYRICS_VERSION,
    "foo_openlyrics " OPENLYRICS_VERSION "\n"
    "Open-source lyrics retrieval and display\n"
    "Source available at https://github.com/jacquesh/foo_openlyrics\n"
    "\n"
    "Built " __DATE__  "\n"
    "\n"
    "Changelog:\n"
    "Version 0.4-WIP:\n"
    "- Add support for saving lyrics to ID3 tags\n"
    "- Add support for configuring which ID3 tags are searched or saved to\n"
    "- Add status-bar descriptions for lyric panel context menu entries\n"
    "- Change lyric loading to not auto-reload whenever any lyrics file changes\n"
    "- Fix the release process producing incompatible *.fb2k-component archives\n"
    "- Fix config reset resetting to the last-saved value rather than the default\n"
    "- Fix a save format preview format error when playback is stopped\n"
    "\n"
    "Version 0.3:\n"
    "- Add built-in support for synchronising lines in the lyric editor\n"
    "- Add support for reading (unsynchronised) lyrics from ID3 tags\n"
    "\n"
    "Version 0.2:\n"
    "- Add support for LRC files with timestamps of the form [00.00.000]\n"
    "- Fix fallback to loading lyrics as plain text when LRC parsing fails\n"
    "- Fix saving lyrics for a track that has not yet saved any\n"
    "\n"
    "Version 0.1:\n"
    "- Initial release\n"
);


// This will prevent users from renaming your component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_openlyrics.dll");
