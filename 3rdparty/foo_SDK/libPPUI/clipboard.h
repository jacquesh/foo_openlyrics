#pragma once

namespace ClipboardHelper {

	class OpenScope {
	public:
		OpenScope() : m_open(false) {}
		~OpenScope() {Close();}
		void Open(HWND p_owner);
		void Close();
	private:
		bool m_open;
		
		PFC_CLASS_NOT_COPYABLE_EX(OpenScope)
	};

	void SetRaw(UINT format,const void * buffer, t_size size);
	void SetString(const char * in);

	bool GetString(pfc::string_base & out);

	template<typename TArray>
	bool GetRaw(UINT format,TArray & out) {
		pfc::assert_byte_type<typename TArray::t_item>();
		HANDLE data = GetClipboardData(format);
		if (data == NULL) return false;
		CGlobalLockScope lock(data);
		out.set_size( lock.GetSize() );
		memcpy(out.get_ptr(), lock.GetPtr(), lock.GetSize() );
		return true;
	}
	bool IsTextAvailable();
};
