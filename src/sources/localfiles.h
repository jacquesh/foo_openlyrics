#pragma once

#include "stdafx.h"

class LocalFilesLyricSource
{
public:
	bool Query(const pfc::string8& title, pfc::string8& out_lyrics);
};