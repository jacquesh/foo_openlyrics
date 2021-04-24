#include "stdafx.h"

#include "config/config_font.h"
#include "winstr_util.h"

cfg_font_t::cfg_font_t(const GUID& guid) : cfg_var(guid)
{
    HDC dc = GetDC(nullptr);
    int default_point_size = 9;
    int default_height = -MulDiv(default_point_size, GetDeviceCaps(dc, LOGPIXELSY), 72);
    ReleaseDC(nullptr, dc);

    LOGFONT default_font = {};
    default_font.lfHeight = default_height;
    default_font.lfWeight = FW_REGULAR;
    default_font.lfCharSet = DEFAULT_CHARSET;
    default_font.lfOutPrecision = OUT_DEFAULT_PRECIS;
    default_font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    default_font.lfQuality = DEFAULT_QUALITY;
    default_font.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    _tcsncpy_s(default_font.lfFaceName, _T("Segoe UI"), _TRUNCATE);
    m_value = default_font;
}

void cfg_font_t::get_data_raw(stream_writer* stream, abort_callback& abort)
{
    std::string name = from_tstring(std::tstring_view{m_value.lfFaceName, LF_FACESIZE});

    stream->write_lendian_t(m_value.lfHeight, abort);
    stream->write_lendian_t(m_value.lfWidth, abort);
    stream->write_lendian_t(m_value.lfEscapement, abort);
    stream->write_lendian_t(m_value.lfOrientation, abort);
    stream->write_lendian_t(m_value.lfWeight, abort);
    stream->write_lendian_t(m_value.lfItalic, abort);
    stream->write_lendian_t(m_value.lfUnderline, abort);
    stream->write_lendian_t(m_value.lfStrikeOut, abort);
    stream->write_lendian_t(m_value.lfCharSet, abort);
    stream->write_lendian_t(m_value.lfOutPrecision, abort);
    stream->write_lendian_t(m_value.lfClipPrecision, abort);
    stream->write_lendian_t(m_value.lfQuality, abort);
    stream->write_lendian_t(m_value.lfPitchAndFamily, abort);
    stream->write_string(name.c_str(), name.length(), abort);
}

void cfg_font_t::set_data_raw(stream_reader* stream, t_size /*size_hint*/, abort_callback& abort)
{
    pfc::string8 name;

    stream->read_lendian_t(m_value.lfHeight, abort);
    stream->read_lendian_t(m_value.lfWidth, abort);
    stream->read_lendian_t(m_value.lfEscapement, abort);
    stream->read_lendian_t(m_value.lfOrientation, abort);
    stream->read_lendian_t(m_value.lfWeight, abort);
    stream->read_lendian_t(m_value.lfItalic, abort);
    stream->read_lendian_t(m_value.lfUnderline, abort);
    stream->read_lendian_t(m_value.lfStrikeOut, abort);
    stream->read_lendian_t(m_value.lfCharSet, abort);
    stream->read_lendian_t(m_value.lfOutPrecision, abort);
    stream->read_lendian_t(m_value.lfClipPrecision, abort);
    stream->read_lendian_t(m_value.lfQuality, abort);
    stream->read_lendian_t(m_value.lfPitchAndFamily, abort);
    stream->read_string(name, abort);

    std::tstring tname = to_tstring(name);
    _tcscpy_s(m_value.lfFaceName, tname.c_str());
}

void cfg_font_t::set_value(const LOGFONT& value)
{
    m_value = value;
}

const LOGFONT& cfg_font_t::get_value()
{
    return m_value;
}

bool cfg_font_t::operator==(const LOGFONT& other)
{
    bool equal = true;
    equal &= (m_value.lfHeight == other.lfHeight);
    equal &= (m_value.lfWidth == other.lfWidth);
    equal &= (m_value.lfEscapement == other.lfEscapement);
    equal &= (m_value.lfOrientation == other.lfOrientation);
    equal &= (m_value.lfWeight == other.lfWeight);
    equal &= (m_value.lfItalic == other.lfItalic);
    equal &= (m_value.lfUnderline == other.lfUnderline);
    equal &= (m_value.lfStrikeOut == other.lfStrikeOut);
    equal &= (m_value.lfCharSet == other.lfCharSet);
    equal &= (m_value.lfOutPrecision == other.lfOutPrecision);
    equal &= (m_value.lfClipPrecision == other.lfClipPrecision);
    equal &= (m_value.lfQuality == other.lfQuality);
    equal &= (m_value.lfPitchAndFamily == other.lfPitchAndFamily);
    equal &= (_tcscmp(m_value.lfFaceName, other.lfFaceName) == 0);
    return equal;
}
