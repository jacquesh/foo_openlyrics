#pragma once

#include "stdafx.h"

#include "lyric_data.h"

namespace sources::localfiles
{
    const extern GUID src_guid;

    pfc::string8 GetLyricsDir();
}