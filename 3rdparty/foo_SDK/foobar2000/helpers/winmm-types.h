#ifndef _WINMM_TYPES_H_
#define _WINMM_TYPES_H_

#ifdef _WIN32
#pragma warning (disable: 4201)	// nostandard ext used
#include <mmreg.h>
#pragma warning (default: 4201)	// nostandard ext used
#else // non _WIN32


typedef uint32_t DWORD;
typedef uint16_t WORD;


#define _WAVEFORMATEX_

typedef struct tWAVEFORMATEX
{
    WORD        wFormatTag;         /* format type */
    WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
    DWORD       nSamplesPerSec;     /* sample rate */
    DWORD       nAvgBytesPerSec;    /* for buffer estimation */
    WORD        nBlockAlign;        /* block size of data */
    WORD        wBitsPerSample;     /* number of bits per sample of mono data */
    WORD        cbSize;             /* the count in bytes of the size of */
    /* extra information (after cbSize) */
} __attribute__((packed)) WAVEFORMATEX, *PWAVEFORMATEX, *NPWAVEFORMATEX, *LPWAVEFORMATEX;


#define _WAVEFORMATEXTENSIBLE_

typedef struct {
    WAVEFORMATEX    Format;
    union {
        WORD wValidBitsPerSample;       /* bits of precision  */
        WORD wSamplesPerBlock;          /* valid if wBitsPerSample==0 */
        WORD wReserved;                 /* If neither applies, set to zero. */
    } Samples;
    DWORD           dwChannelMask;      /* which channels are */
    /* present in stream  */
    GUID            SubFormat;
} __attribute__((packed)) WAVEFORMATEXTENSIBLE, *PWAVEFORMATEXTENSIBLE;

typedef struct {
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
} __attribute__((packed)) WAVEFORMAT;

typedef struct {
    WAVEFORMAT wf;
    WORD       wBitsPerSample;
} __attribute__((packed)) PCMWAVEFORMAT;

#define WAVE_FORMAT_PCM             0x0001
#define WAVE_FORMAT_ADPCM           0x0002
#define WAVE_FORMAT_IEEE_FLOAT      0x0003
#define WAVE_FORMAT_ALAW            0x0006
#define WAVE_FORMAT_MULAW           0x0007
#define WAVE_FORMAT_MPEG            0x0050
#define WAVE_FORMAT_EXTENSIBLE      0xFFFE
#define WAVE_FORMAT_MPEGLAYER3      0x0055

#define SPEAKER_FRONT_LEFT 0x1
#define SPEAKER_FRONT_RIGHT 0x2
#define SPEAKER_FRONT_CENTER 0x4
#define SPEAKER_LOW_FREQUENCY 0x8
#define SPEAKER_BACK_LEFT 0x10
#define SPEAKER_BACK_RIGHT 0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER 0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER 0x80
#define SPEAKER_BACK_CENTER 0x100
#define SPEAKER_SIDE_LEFT 0x200
#define SPEAKER_SIDE_RIGHT 0x400
#define SPEAKER_TOP_CENTER 0x800
#define SPEAKER_TOP_FRONT_LEFT 0x1000
#define SPEAKER_TOP_FRONT_CENTER 0x2000
#define SPEAKER_TOP_FRONT_RIGHT 0x4000
#define SPEAKER_TOP_BACK_LEFT 0x8000
#define SPEAKER_TOP_BACK_CENTER 0x10000
#define SPEAKER_TOP_BACK_RIGHT 0x20000
#define SPEAKER_ALL 0x80000000

static const GUID KSDATAFORMAT_SUBTYPE_PCM =			{0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};	// "00000001-0000-0010-8000-00aa00389b71"
static const GUID KSDATAFORMAT_SUBTYPE_IEEE_FLOAT =	{0x00000003, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};	// "00000003-0000-0010-8000-00aa00389b71"

static const GUID KSDATAFORMAT_SUBTYPE_ALAW =			{0x00000006, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};   // "00000006-0000-0010-8000-00aa00389b71"
static const GUID KSDATAFORMAT_SUBTYPE_MULAW =			{0x00000007, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};	// "00000007-0000-0010-8000-00aa00389b71"
static const GUID KSDATAFORMAT_SUBTYPE_ADPCM =			{0x00000002, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};	// "00000002-0000-0010-8000-00aa00389b71"
static const GUID KSDATAFORMAT_SUBTYPE_MPEG =			{0x00000050, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};	// "00000050-0000-0010-8000-00aa00389b71"

#define mmioFOURCC(ch0, ch1, ch2, ch3) \
MAKEFOURCC(ch0, ch1, ch2, ch3)

#define MAKEFOURCC(ch0, ch1, ch2, ch3)  \
((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |  \
 ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

#endif

#endif // _WINMM_TYPES_H_
