#pragma once

#include "stdafx.h"

class cfg_font_t : public cfg_var
{
    LOGFONT m_value;

public:
    cfg_font_t(const GUID& guid);
    void get_data_raw(stream_writer* stream, abort_callback& abort) override;
    void set_data_raw(stream_reader* stream, t_size size_hint, abort_callback& abort) override;

    void set_value(const LOGFONT& value);
    const LOGFONT& get_value();
    bool operator==(const LOGFONT& other);
};
