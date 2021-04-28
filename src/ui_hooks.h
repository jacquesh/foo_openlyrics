#pragma once

#include "lyric_io.h"
#include "winstr_util.h"

void SpawnLyricEditor(const std::tstring& lyric_text, LyricUpdateHandle& update);

void repaint_all_lyric_panels();
