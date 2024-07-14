#include "shared.h"
#include "filedialogs.h"
#include <shlobj.h>

#define dTEXT(X) pfc::stringcvt::string_os_from_utf8(X)

static UINT_PTR CALLBACK uGetOpenFileName_Hook(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg) {
	case WM_INITDIALOG:
		{
			OPENFILENAME * ofn = reinterpret_cast<OPENFILENAME*>(lp);
			reinterpret_cast<modal_dialog_scope*>(ofn->lCustData)->initialize(FindOwningPopup(wnd));
		}
		return 0;
	default:
		return 0;
	}
}

static void ImportExtMask(pfc::array_t<TCHAR> & out, const char * in) {
	{
		pfc::stringcvt::string_os_from_utf8 temp(in);
		out.set_size(temp.length()+2);
		out.fill_null();
		pfc::memcpy_t(out.get_ptr(),temp.get_ptr(),temp.length());
	}

	for(t_size walk = 0; walk < out.get_size(); ++walk) {
		if (out[walk] == '|') out[walk] = 0;
	}
}

BOOL Vista_GetOpenFileName(HWND parent,const char * p_ext_mask,unsigned def_ext_mask,const char * p_def_ext,const char * p_title,const char * p_directory,pfc::string_base & p_filename,BOOL b_save);
BOOL Vista_GetOpenFileNameMulti(HWND parent,const char * p_ext_mask,unsigned def_ext_mask,const char * p_def_ext,const char * p_title,const char * p_directory,pfc::ptrholder_t<uGetOpenFileNameMultiResult> & out);
BOOL Vista_BrowseForFolder(HWND parent, const char * p_title, pfc::string_base & path);
puGetOpenFileNameMultiResult Vista_BrowseForFolderEx(HWND parent,const char * title, const char * initPath);

static bool UseVistaDialogs() {
#if FB2K_TARGET_MICROSOFT_STORE || _WIN32_WINNT >= 0x600
	return true;
#else
	return GetWindowsVersionCode() >= 0x600;
#endif
}

BOOL SHARED_EXPORT uGetOpenFileName(HWND parent,const char * p_ext_mask,unsigned def_ext_mask,const char * p_def_ext,const char * p_title,const char * p_directory,pfc::string_base & p_filename,BOOL b_save) {
	TRACK_CALL_TEXT("uGetOpenFileName");
	try {
		if (UseVistaDialogs()) return Vista_GetOpenFileName(parent, p_ext_mask, def_ext_mask, p_def_ext, p_title, p_directory, p_filename, b_save);
	} catch(pfc::exception_not_implemented) {}

	modal_dialog_scope scope;

	pfc::array_t<TCHAR> ext_mask;
	ImportExtMask(ext_mask,p_ext_mask);
	
	TCHAR buffer[4096];

	pfc::stringToBuffer(buffer,pfc::stringcvt::string_os_from_utf8(p_filename));

	pfc::stringcvt::string_os_from_utf8 def_ext(p_def_ext ? p_def_ext : ""),title(p_title ? p_title : ""),
		directory(p_directory ? p_directory : "");

	OPENFILENAME ofn = {};

	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner = parent;
	ofn.lpstrFilter = ext_mask.get_ptr();
	ofn.nFilterIndex = def_ext_mask + 1;
	ofn.lpstrFile = buffer;
	ofn.lpstrInitialDir = directory;
	ofn.nMaxFile = _countof(buffer);
	ofn.Flags = b_save ? OFN_NOCHANGEDIR|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT|OFN_ENABLEHOOK|OFN_ENABLESIZING : OFN_NOCHANGEDIR|OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_ENABLEHOOK|OFN_ENABLESIZING;
	ofn.lpstrDefExt = *(const TCHAR*)def_ext ? (const TCHAR*)def_ext : 0;
	ofn.lpstrTitle = *(const TCHAR*)title ? (const TCHAR*)title : 0;
	ofn.lCustData = reinterpret_cast<LPARAM>(&scope);
	ofn.lpfnHook = uGetOpenFileName_Hook;
	if (b_save ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn))
	{
		buffer[_countof(buffer)-1]=0;

		{
			t_size ptr = _tcslen(buffer);
			while(ptr>0 && buffer[ptr-1]==' ') buffer[--ptr] = 0;
		}

		p_filename = pfc::stringcvt::string_utf8_from_os(buffer,_countof(buffer));
		return TRUE;
	}
	else return FALSE;
}


puGetOpenFileNameMultiResult SHARED_EXPORT uGetOpenFileNameMulti(HWND parent,const char * p_ext_mask,unsigned def_ext_mask,const char * p_def_ext,const char * p_title,const char * p_directory) {
	TRACK_CALL_TEXT("uGetOpenFileNameMulti");
	try {
		if (UseVistaDialogs()) {
			pfc::ptrholder_t<uGetOpenFileNameMultiResult> result;
			if (!Vista_GetOpenFileNameMulti(parent,p_ext_mask,def_ext_mask,p_def_ext,p_title,p_directory,result)) return NULL;
			return result.detach();
		}
	} catch(pfc::exception_not_implemented) {}

	modal_dialog_scope scope;

	pfc::array_t<TCHAR> ext_mask;
	ImportExtMask(ext_mask,p_ext_mask);
	
	TCHAR buffer[0x4000];
	buffer[0]=0;

	pfc::stringcvt::string_os_from_utf8 def_ext(p_def_ext ? p_def_ext : ""),title(p_title ? p_title : ""),
		directory(p_directory ? p_directory : "");

	OPENFILENAME ofn = {};

	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner = parent;
	ofn.lpstrFilter = ext_mask.get_ptr();
	ofn.nFilterIndex = def_ext_mask + 1;
	ofn.lpstrFile = buffer;
	ofn.lpstrInitialDir = directory;
	ofn.nMaxFile = _countof(buffer);
	ofn.Flags = OFN_NOCHANGEDIR|OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_ALLOWMULTISELECT|OFN_ENABLEHOOK|OFN_ENABLESIZING;
	ofn.lpstrDefExt = *(const TCHAR*)def_ext ? (const TCHAR*)def_ext : 0;
	ofn.lpstrTitle = *(const TCHAR*)title ? (const TCHAR*)title : 0;
	ofn.lCustData = reinterpret_cast<LPARAM>(&scope);
	ofn.lpfnHook = uGetOpenFileName_Hook;
	if (GetOpenFileName(&ofn))
	{
		buffer[_countof(buffer)-1]=0;
		buffer[_countof(buffer)-2]=0;

		pfc::ptrholder_t<uGetOpenFileNameMultiResult_impl> result = new uGetOpenFileNameMultiResult_impl;

		TCHAR * p=buffer;
		while(*p) p++;
		p++;
		if (!*p)
		{
			{
				t_size ptr = _tcslen(buffer);
				while(ptr>0 && buffer[ptr-1]==' ') buffer[--ptr] = 0;
			}

			result->AddItem(pfc::stringcvt::string_utf8_from_os(buffer));
		}
		else
		{
			pfc::string_formatter s = (const char*) pfc::stringcvt::string_utf8_from_os(buffer,_countof(buffer));
			t_size ofs = s.length();
			if (ofs>0 && s[ofs-1]!='\\') {s.add_char('\\');ofs++;}
			while(*p)
			{
				s.truncate(ofs);
				s += pfc::stringcvt::string_utf8_from_os(p);
				s.skip_trailing_char(' ');
				result->AddItem(s);
				while(*p) p++;
				p++;
			}
		}
		return result.detach();
	}
	else return 0;
}





struct browse_for_dir_struct
{
	const TCHAR * m_initval;
	const TCHAR * m_tofind;

	modal_dialog_scope m_scope;
};

static bool file_exists(const TCHAR * p_path)
{
	DWORD val = GetFileAttributes(p_path);
	if (val == (-1) || (val & FILE_ATTRIBUTE_DIRECTORY)) return false;
	return true;
}

static void browse_proc_check_okbutton(HWND wnd,const browse_for_dir_struct * p_struct,const TCHAR * p_path)
{
	TCHAR temp[MAX_PATH+1];
	pfc::stringToBuffer(temp, p_path);

	t_size len = _tcslen(temp);
	if (len < MAX_PATH && len > 0)
	{
		if (temp[len-1] != '\\')
			temp[len++] = '\\';
	}
	t_size idx = 0;
	while(p_struct->m_tofind[idx] && idx+len < MAX_PATH)
	{
		temp[len+idx] = p_struct->m_tofind[idx];
		idx++;
	}
	temp[len+idx] = 0;

	SendMessage(wnd,BFFM_ENABLEOK,0,!!file_exists(temp));

}

static int _stdcall browse_proc(HWND wnd,UINT msg,LPARAM lp,LPARAM dat)
{
	browse_for_dir_struct * p_struct = reinterpret_cast<browse_for_dir_struct*>(dat);
	switch(msg)
	{
	case BFFM_INITIALIZED:
		p_struct->m_scope.initialize(wnd);
		SendMessage(wnd,BFFM_SETSELECTION,1,(LPARAM)p_struct->m_initval);
		if (p_struct->m_tofind) browse_proc_check_okbutton(wnd,p_struct,p_struct->m_initval);
		break;
	case BFFM_SELCHANGED:
		if (p_struct->m_tofind)
		{
			if (lp != 0)
			{
				TCHAR temp[MAX_PATH+1];
				if (SHGetPathFromIDList(reinterpret_cast<LPCITEMIDLIST>(lp),temp))
				{
					temp[MAX_PATH] = 0;
					browse_proc_check_okbutton(wnd,p_struct,temp);
				}
				else
					SendMessage(wnd,BFFM_ENABLEOK,0,FALSE);
			}
			else SendMessage(wnd,BFFM_ENABLEOK,0,FALSE);
		}
		break;
	}
	return 0;
}

static BOOL BrowseForFolderHelper(HWND p_parent,const TCHAR * p_title,TCHAR (&p_out)[MAX_PATH],const TCHAR * p_file_to_find)
{
	pfc::com_ptr_t<IMalloc> mallocptr;
	
	if (FAILED(SHGetMalloc(mallocptr.receive_ptr()))) return FALSE;
	if (mallocptr.is_empty()) return FALSE;

	browse_for_dir_struct data;
	data.m_initval = p_out;
	data.m_tofind = p_file_to_find;


	BROWSEINFO bi=
	{
		p_parent,
		0,
		0,
		p_title,
		BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX,
		browse_proc,
		reinterpret_cast<LPARAM>(&data),
		0
	};

	LPITEMIDLIST li = SHBrowseForFolder(&bi);
	if (li == NULL) return FALSE;
	BOOL state = SHGetPathFromIDList(li,p_out);
	mallocptr->Free(li);
	return state;
}

BOOL SHARED_EXPORT uBrowseForFolder(HWND parent,const char * p_title,pfc::string_base & out) {
	TRACK_CALL_TEXT("uBrowseForFolder");
	try {
		if (UseVistaDialogs()) {
			return Vista_BrowseForFolder(parent,p_title,out);
		}
	} catch(pfc::exception_not_implemented) {}

	TCHAR temp[MAX_PATH];
	pfc::stringToBuffer(temp,dTEXT(out));
	BOOL rv = BrowseForFolderHelper(parent,dTEXT(p_title),temp,0);
	if (rv) {
		out = pfc::stringcvt::string_utf8_from_os(temp,_countof(temp));
	}
	return rv;
}

BOOL SHARED_EXPORT uBrowseForFolderWithFile(HWND parent,const char * title,pfc::string_base & out,const char * p_file_to_find)
{
	TRACK_CALL_TEXT("uBrowseForFolderWithFile");
	TCHAR temp[MAX_PATH];
	pfc::stringToBuffer(temp,dTEXT(out));
	BOOL rv = BrowseForFolderHelper(parent,dTEXT(title),temp,dTEXT(p_file_to_find));
	if (rv) {
		out = pfc::stringcvt::string_utf8_from_os(temp,_countof(temp));
	}
	return rv;
}


puGetOpenFileNameMultiResult SHARED_EXPORT uBrowseForFolderEx(HWND parent,const char * title, const char * initPath) {
	TRACK_CALL_TEXT("uBrowseForFolderEx");
	try {
		if (UseVistaDialogs()) {
			return Vista_BrowseForFolderEx(parent,title, initPath);
		} 
	} catch(pfc::exception_not_implemented) {}

	pfc::string8 temp;
	if (initPath) temp = initPath;
	if (!uBrowseForFolder(parent, title, temp)) return NULL;
	pfc::ptrholder_t<uGetOpenFileNameMultiResult_impl> result = new uGetOpenFileNameMultiResult_impl;
	result->AddItem(temp);
	return result.detach();
}
