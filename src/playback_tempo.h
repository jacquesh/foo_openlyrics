#pragma once

#include "stdafx.h"

// Computes the playback tempo/speed multiplier based on active DSPs (e.g. foo_dsp_effect's
// Tempo Shift and Playback Rate Shift) and optional track metadata tag overrides ("tempo_amt", "pbrate_amt").
// Returns 1.0 when no tempo alteration is active.
double get_playback_tempo_multiplier(const metadb_v2_rec_t& track_info);

#if MVTF_TESTS_ENABLED
// Unit test helpers for parsing tempo/rate preset structures
double parse_tempo_dsp_preset(const uint8_t* data, size_t size);
double parse_rate_dsp_preset(const uint8_t* data, size_t size);
#endif
