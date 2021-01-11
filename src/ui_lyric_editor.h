#pragma once

#include "stdafx.h"

// TODO: These types apply more widely than for just the UI, we should move them to a non-UI header
enum class LyricFormat
{
    Unknown,
    Plaintext,
    Timestamped
};
struct LyricEditData
{
    LyricFormat format;
    pfc::string8 file_title;
    pfc::string8 text;
};
void SpawnLyricEditor(const LyricEditData& edit_data);

