#pragma once

#include "stdafx.h"

#include "lyric_data.h"
#include "lyric_search.h" // TODO: Included just for LyricUpdateHandle, which ideally does not live in lyric_search.h

void SpawnLyricEditor(const std::string& lyric_text, LyricUpdateHandle& update);

