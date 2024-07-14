#pragma once

//! Stub implementation of file object, no file content, only info.
class file_streamstub : public file_readonly {
public:
	t_size read(void*, t_size, abort_callback&) override { return 0; }
	t_filesize get_size(abort_callback&) override { return filesize_invalid; }
	t_filesize get_position(abort_callback&) override { return 0; }
	bool get_content_type(pfc::string_base& out) override {
		if (m_contentType.length() > 0) { out = m_contentType; return true; } else return false;
	}
	bool is_remote() override { return m_remote; }
	void reopen(abort_callback&) override {}
	void seek(t_filesize, abort_callback&) override { throw exception_io_object_not_seekable(); }
	bool can_seek() override { return false; }

	pfc::string8 m_contentType;
	bool m_remote = true;
};
