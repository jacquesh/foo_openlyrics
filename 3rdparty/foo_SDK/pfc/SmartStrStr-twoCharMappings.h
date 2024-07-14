#pragma once

static constexpr struct {
	unsigned from;
	const char* to;
} twoCharMappings[] = {
	{0x00C6, "AE"},
	{0x00E6, "ae"},
	{0x00DF, "ss"},

#if 0 
	// umlauts
	// the problem with these is that changing them to two-letter represenatations prevents search by non-umlaut vowel from working.
	{0x00C4, "AE"},
	{0x00E4, "ae"},
	{0x00D6, "OE"},
	{0x00F6, "oe"},
	{0x00DC, "UE"},
	{0x00FC, "ue"},
#endif

#if 0 
	// Incomplete list, hence disabled. Nobody uses these.
	{0x01E2, "AE"},
	{0x01FC, "AE"},
	{0x01E3, "ae"},
	{0x01FD, "ae"},
	{0x0152, "OE"},
	{0x0153, "oe"},
	{0x0276, "oe"},
	{0x01C3, "dz"},
	{0x01C4, "DZ"},
	{0x01C5, "Dz"},
	{0x01C6, "dz"},
	{0x01F1, "DZ"},
	{0x01F2, "Dz"},
	{0x01F3, "dz"},
	{0x02A3, "dz"},
	{0x02A5, "dz"},
	{0x01C7, "LJ"},
	{0x01C8, "Lj"},
	{0x01C9, "lj"},
	{0x01CA, "NJ"},
	{0x01CB, "Nj"},
	{0x01CC, "nj"},
	{0x0132, "IJ"},
	{0x0133, "ij"},
#endif
};
