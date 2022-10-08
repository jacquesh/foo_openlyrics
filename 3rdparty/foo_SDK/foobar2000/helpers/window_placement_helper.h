#pragma once

#ifdef FOOBAR2000_DESKTOP_WINDOWS

class cfg_window_placement_common : public cfg_var {
public:
	cfg_window_placement_common(const GUID& guid) : cfg_var(guid) {}
	bool read_from_window(HWND window);
	bool apply_to_window(HWND window, bool allowHidden);
protected:
	void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) override;
	void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override;
	WINDOWPLACEMENT m_data = {};
};

class cfg_window_placement : public cfg_window_placement_common
{
public:
	bool on_window_creation(HWND window, bool allowHidden = false);//returns true if window position has been changed, false if not
	void on_window_creation_silent(HWND window);
	void on_window_destruction(HWND window);
	cfg_window_placement(const GUID& guid) : cfg_window_placement_common(guid) {}
protected:
	void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) override;
private:
	pfc::list_hybrid_t<HWND,2> m_windows;
};

class cfg_window_placement_v2 : public cfg_window_placement_common {
public:
	cfg_window_placement_v2(const GUID& guid) : cfg_window_placement_common(guid) {}
	// All already in cfg_window_placement_common
};



class cfg_window_size : public cfg_var
{
public:
	bool on_window_creation(HWND window);//returns true if window position has been changed, false if not
	void on_window_destruction(HWND window);
	bool read_from_window(HWND window);
	void get_data_raw(stream_writer * p_stream,abort_callback & p_abort);
	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort);
	cfg_window_size(const GUID & p_guid);
private:
	pfc::list_hybrid_t<HWND,2> m_windows;
	t_uint32 m_width,m_height;
};

#endif // FOOBAR2000_DESKTOP_WINDOWS
