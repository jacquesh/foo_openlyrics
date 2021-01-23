#include "stdafx.h"

#define OPENLYRICS_VERSION "0.1"

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
);


// This will prevent users from renaming your component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_openlyrics.dll");
