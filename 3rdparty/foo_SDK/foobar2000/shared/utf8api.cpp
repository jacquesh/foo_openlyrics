#include "shared.h"


#include <lmcons.h>

#ifndef BIF_NEWDIALOGSTYLE
#define BIF_NEWDIALOGSTYLE 0x0040
#endif

using namespace pfc;

class param_os_from_utf8
{
	bool m_is_null;
	WORD m_low_word;
	stringcvt::string_os_from_utf8 m_cvt;
public:
	param_os_from_utf8(const char * p) : 
		m_is_null(p==NULL), 
		m_low_word( ((t_size)p & ~0xFFFF) == 0 ? (WORD)((t_size)p & 0xFFFF) : 0),
		m_cvt( p != NULL && ((t_size)p & ~0xFFFF) != 0 ? p : "") 
		{}
	operator const TCHAR *()
	{
		return get_ptr();
	}
	const TCHAR * get_ptr()
	{
		return m_low_word ? (const TCHAR*)(t_size)m_low_word : m_is_null ? 0 : m_cvt.get_ptr();
	}
	
};



extern "C" {

LRESULT SHARED_EXPORT uSendMessageText(HWND wnd,UINT msg,WPARAM wp,const char * p_text)
{
	if (p_text == NULL)
		return SendMessage(wnd,msg,wp,0);
	else {
		stringcvt::string_os_from_utf8 temp;
		temp.convert(p_text);
		return SendMessage(wnd,msg,wp,(LPARAM)temp.get_ptr());
	}
}

LRESULT SHARED_EXPORT uSendDlgItemMessageText(HWND wnd,UINT id,UINT msg,WPARAM wp,const char * text)
{
	return uSendMessageText(uGetDlgItem(wnd,id),msg,wp,text);//SendDlgItemMessage(wnd,id,msg,wp,(long)(const TCHAR*)string_os_from_utf8(text));
}

BOOL SHARED_EXPORT uGetWindowText(HWND wnd,string_base & out)
{
	PFC_ASSERT( wnd != NULL );
	int len = GetWindowTextLength(wnd);
	if (len>0)
	{
		len++;
		pfc::array_t<TCHAR> temp;
		temp.set_size(len);
		temp[0]=0;		
		if (GetWindowText(wnd,temp.get_ptr(),len)>0)
		{
			out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
			return TRUE;
		}
		else return FALSE;
	}
	else
	{
		out.reset();
		return TRUE;
	}
}

BOOL SHARED_EXPORT uSetWindowTextEx(HWND wnd,const char * p_text,size_t p_text_length)
{
	return SetWindowText(wnd,stringcvt::string_os_from_utf8(p_text, p_text_length));
}


BOOL SHARED_EXPORT uGetDlgItemText(HWND wnd,UINT id,string_base & out)
{
	return uGetWindowText(GetDlgItem(wnd,id),out);
}

BOOL SHARED_EXPORT uSetDlgItemTextEx(HWND wnd,UINT id,const char * p_text,size_t p_text_length)
{
	return SetDlgItemText(wnd,id,stringcvt::string_os_from_utf8(p_text,p_text_length));
}

int SHARED_EXPORT uMessageBox(HWND wnd,const char * text,const char * caption,UINT type)
{
	modal_dialog_scope scope(wnd);
	return MessageBox(wnd,param_os_from_utf8(text),param_os_from_utf8(caption),type);
}

void SHARED_EXPORT uOutputDebugString(const char * msg) {OutputDebugString(stringcvt::string_os_from_utf8(msg));}

BOOL SHARED_EXPORT uAppendMenu(HMENU menu,UINT flags,UINT_PTR id,const char * content)
{
	return AppendMenu(menu,flags,id,param_os_from_utf8(content));
}

BOOL SHARED_EXPORT uInsertMenu(HMENU menu,UINT position,UINT flags,UINT_PTR id,const char * content)
{
	return InsertMenu(menu,position,flags,id,param_os_from_utf8(content));
}

int SHARED_EXPORT uCharCompare(t_uint32 p_char1,t_uint32 p_char2) {
#ifdef UNICODE
	wchar_t temp1[4],temp2[4];
	temp1[utf16_encode_char(p_char1,temp1)]=0;
	temp2[utf16_encode_char(p_char2,temp2)]=0;
	return lstrcmpiW(temp1,temp2);
#else
	wchar_t temp1[4],temp2[4];
	char ctemp1[20],ctemp2[20];
	temp1[utf16_encode_char(p_char1,temp1)]=0;
	temp2[utf16_encode_char(p_char2,temp2)]=0;
	WideCharToMultiByte(CP_ACP,0,temp1,-1,ctemp1,_countof(ctemp1),0,0);
	WideCharToMultiByte(CP_ACP,0,temp2,-1,ctemp2,_countof(ctemp2),0,0);
	return lstrcmpiA(ctemp1,ctemp2);
#endif
}

int SHARED_EXPORT uStringCompare(const char * elem1, const char * elem2) {
	for(;;) {
		unsigned c1,c2; t_size l1,l2;
		l1 = utf8_decode_char(elem1,c1);
		l2 = utf8_decode_char(elem2,c2);
		if (l1==0 && l2==0) return 0;
		if (c1!=c2) {
			int test = uCharCompare(c1,c2);
			if (test) return test;
		}
		elem1 += l1;
		elem2 += l2;
	}
}

int SHARED_EXPORT uStringCompare_ConvertNumbers(const char * elem1,const char * elem2) {
	for(;;) {
		if (pfc::char_is_numeric(*elem1) && pfc::char_is_numeric(*elem2)) {
			t_size delta1 = 1, delta2 = 1;
			while(pfc::char_is_numeric(elem1[delta1])) delta1++;
			while(pfc::char_is_numeric(elem2[delta2])) delta2++;
			int test = pfc::compare_t(pfc::atoui64_ex(elem1,delta1),pfc::atoui64_ex(elem2,delta2));
			if (test != 0) return test;
			elem1 += delta1;
			elem2 += delta2;
		} else {
			unsigned c1,c2; t_size l1,l2;
			l1 = utf8_decode_char(elem1,c1);
			l2 = utf8_decode_char(elem2,c2);
			if (l1==0 && l2==0) return 0;
			if (c1!=c2) {
				int test = uCharCompare(c1,c2);
				if (test) return test;
			}
			elem1 += l1;
			elem2 += l2;
		}
	}
}

HINSTANCE SHARED_EXPORT uLoadLibrary(const char * name)
{
	return LoadLibrary(param_os_from_utf8(name));
}

HANDLE SHARED_EXPORT uCreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes,BOOL bManualReset,BOOL bInitialState, const char * lpName)
{
	return CreateEvent(lpEventAttributes,bManualReset,bInitialState, param_os_from_utf8(lpName));
}

DWORD SHARED_EXPORT uGetModuleFileName(HMODULE hMod,string_base & out)
{
	try {
		pfc::array_t<TCHAR> buffer; buffer.set_size(256);
		for(;;) {
			DWORD ret = GetModuleFileName(hMod,buffer.get_ptr(), (DWORD)buffer.get_size());
			if (ret == 0) return 0;
			if (ret < buffer.get_size()) break;
			buffer.set_size(buffer.get_size() * 2);
		}
		out = stringcvt::string_utf8_from_os(buffer.get_ptr(),buffer.get_size());
		return (DWORD) out.length();
	} catch(...) {
		return 0;
	}
}

BOOL SHARED_EXPORT uSetClipboardRawData(UINT format,const void * ptr,t_size size) {
	try {
		HANDLE buffer = GlobalAlloc(GMEM_DDESHARE,size);
		if (buffer == NULL) throw std::bad_alloc();
		try {
			CGlobalLockScope lock(buffer);
			PFC_ASSERT(lock.GetSize() == size);
			memcpy(lock.GetPtr(),ptr,size);
		} catch(...) {
			GlobalFree(buffer); throw;
		}

		if (SetClipboardData(format,buffer) == NULL) throw pfc::exception_bug_check();
		return TRUE;
	} catch(...) {
		return FALSE;
	}
}
BOOL SHARED_EXPORT uSetClipboardString(const char * ptr)
{
	try {
		CClipboardOpenScope scope;
		if (!scope.Open(NULL)) return FALSE;
		EmptyClipboard();
		stringcvt::string_os_from_utf8 temp(ptr);
		return uSetClipboardRawData(
	#ifdef UNICODE
				CF_UNICODETEXT
	#else
				CF_TEXT
	#endif
				,temp.get_ptr(), (temp.length() + 1) * sizeof(TCHAR));
	} catch(...) {
		return FALSE;
	}
}

BOOL SHARED_EXPORT uGetClipboardString(pfc::string_base & p_out) {
	try {
		CClipboardOpenScope scope;
		if (!scope.Open(NULL)) return FALSE;
		HANDLE data = GetClipboardData(
	#ifdef UNICODE
			CF_UNICODETEXT
	#else
			CF_TEXT
	#endif
			);
		if (data == NULL) return FALSE;

		CGlobalLockScope lock(data);
		p_out = pfc::stringcvt::string_utf8_from_os( (const TCHAR*) lock.GetPtr(), lock.GetSize() / sizeof(TCHAR) );
		return TRUE;
	} catch(...) {
		return FALSE;
	}
}


BOOL SHARED_EXPORT uGetClassName(HWND wnd,string_base & out)
{
	TCHAR temp[512];
	temp[0]=0;
	if (GetClassName(wnd,temp,_countof(temp))>0)
	{
		out = stringcvt::string_utf8_from_os(temp,_countof(temp));
		return TRUE;
	}
	else return FALSE;
}

t_size SHARED_EXPORT uCharLength(const char * src) {return utf8_char_len(src);}

BOOL SHARED_EXPORT uDragQueryFile(HDROP hDrop,UINT idx,string_base & out)
{
	UINT len = DragQueryFile(hDrop,idx,0,0);
	if (len>0 && len!=(UINT)(~0))
	{
		len++;
		array_t<TCHAR> temp;
		temp.set_size(len);
		temp[0] =0 ;
		if (DragQueryFile(hDrop,idx,temp.get_ptr(),len)>0)
		{
			out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
			return TRUE;
		}
	}
	return FALSE;
}

UINT SHARED_EXPORT uDragQueryFileCount(HDROP hDrop)
{
	return DragQueryFile(hDrop,-1,0,0);
}



BOOL SHARED_EXPORT uGetTextExtentPoint32(HDC dc,const char * text,UINT cb,LPSIZE size)
{
	stringcvt::string_os_from_utf8 temp(text,cb);
	return GetTextExtentPoint32(dc,temp,pfc::downcast_guarded<int>(temp.length()),size);
}

BOOL SHARED_EXPORT uExtTextOut(HDC dc,int x,int y,UINT flags,const RECT * rect,const char * text,UINT cb,const int * lpdx)
{
	stringcvt::string_os_from_utf8 temp(text,cb);
	return ExtTextOut(dc,x,y,flags,rect,temp,pfc::downcast_guarded<int>(_tcslen(temp)),lpdx);
}

static UINT_PTR CALLBACK choose_color_hook(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			CHOOSECOLOR * cc = reinterpret_cast<CHOOSECOLOR*>(lp);
			reinterpret_cast<modal_dialog_scope*>(cc->lCustData)->initialize(FindOwningPopup(wnd));
		}
		return 0;
	default:
		return 0;
	}
}

BOOL SHARED_EXPORT uChooseColor(DWORD * p_color,HWND parent,DWORD * p_custom_colors)
{
	modal_dialog_scope scope;

	CHOOSECOLOR cc = {};
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = parent;
	cc.rgbResult = *p_color;
	cc.lpCustColors = p_custom_colors;
	cc.Flags = CC_ANYCOLOR|CC_FULLOPEN|CC_RGBINIT|CC_ENABLEHOOK;
	cc.lpfnHook = choose_color_hook;
	cc.lCustData = reinterpret_cast<LPARAM>(&scope);
	BOOL rv = ChooseColor(&cc);
	if (rv)
	{
		*p_color = cc.rgbResult;
		return TRUE;
	}
	else return FALSE;
}

HCURSOR SHARED_EXPORT uLoadCursor(HINSTANCE hIns,const char * name)
{
	return LoadCursor(hIns,param_os_from_utf8(name));
}

HICON SHARED_EXPORT uLoadIcon(HINSTANCE hIns,const char * name)
{
	return LoadIcon(hIns,param_os_from_utf8(name));
}

HMENU SHARED_EXPORT uLoadMenu(HINSTANCE hIns,const char * name)
{
	return LoadMenu(hIns,param_os_from_utf8(name));
}



BOOL SHARED_EXPORT uGetEnvironmentVariable(const char * name,string_base & out)
{
	stringcvt::string_os_from_utf8 name_t(name);
	DWORD size = GetEnvironmentVariable(name_t,0,0);
	if (size>0)
	{
		size++;
		array_t<TCHAR> temp;
		temp.set_size(size);
		temp[0]=0;
		if (GetEnvironmentVariable(name_t,temp.get_ptr(),size)>0)
		{
			out = stringcvt::string_utf8_from_os(temp.get_ptr(),size);
			return TRUE;
		}
	}
	return FALSE;
}

HMODULE SHARED_EXPORT uGetModuleHandle(const char * name)
{
	return GetModuleHandle(param_os_from_utf8(name));
}

UINT SHARED_EXPORT uRegisterWindowMessage(const char * name)
{
	return RegisterWindowMessage(stringcvt::string_os_from_utf8(name));
}

BOOL SHARED_EXPORT uMoveFile(const char * src,const char * dst)
{
	return MoveFile(stringcvt::string_os_from_utf8(src),stringcvt::string_os_from_utf8(dst));
}

BOOL SHARED_EXPORT uDeleteFile(const char * fn)
{
	return DeleteFile(stringcvt::string_os_from_utf8(fn));
}

DWORD SHARED_EXPORT uGetFileAttributes(const char * fn)
{
	PFC_ASSERT( ! pfc::string_has_prefix_i( fn, "file://" ) );
	return GetFileAttributes(stringcvt::string_os_from_utf8(fn));
}

BOOL SHARED_EXPORT uRemoveDirectory(const char * fn)
{
	return RemoveDirectory(stringcvt::string_os_from_utf8(fn));
}

HANDLE SHARED_EXPORT uCreateFile(const char * fn,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES blah,DWORD creat,DWORD flags,HANDLE tmpl)
{
	return CreateFile(stringcvt::string_os_from_utf8(fn),access,share,blah,creat,flags,tmpl);
}

BOOL SHARED_EXPORT uCreateDirectory(const char * fn,LPSECURITY_ATTRIBUTES blah)
{
	return CreateDirectory(stringcvt::string_os_from_utf8(fn),blah);
}

HANDLE SHARED_EXPORT uCreateMutex(LPSECURITY_ATTRIBUTES blah,BOOL bInitialOwner,const char * name)
{
	return name ? CreateMutex(blah,bInitialOwner,stringcvt::string_os_from_utf8(name)) : CreateMutex(blah,bInitialOwner,0);
}

BOOL SHARED_EXPORT uGetFullPathName(const char * name,string_base & out)
{
	stringcvt::string_os_from_utf8 name_os(name);
	unsigned len = GetFullPathName(name_os,0,0,0);
	if (len==0) return FALSE;
	array_t<TCHAR> temp;
	temp.set_size(len+1);
	TCHAR * blah;
	if (GetFullPathName(name_os,len+1,temp.get_ptr(),&blah)==0) return FALSE;
	out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
	return TRUE;
}

BOOL SHARED_EXPORT uGetLongPathName(const char * name,string_base & out)
{
	TCHAR temp[4096];
	temp[0]=0;
	BOOL state = GetLongPathName(stringcvt::string_os_from_utf8(name),temp,_countof(temp));
	if (state) out = stringcvt::string_utf8_from_os(temp,_countof(temp));
	return state;
}

void SHARED_EXPORT uGetCommandLine(string_base & out)
{
	out = stringcvt::string_utf8_from_os(GetCommandLine());
}

BOOL SHARED_EXPORT uGetTempPath(string_base & out)
{
	TCHAR temp[MAX_PATH+1];
	temp[0]=0;
	if (GetTempPath(_countof(temp),temp))
	{
		out = stringcvt::string_utf8_from_os(temp,_countof(temp));
		return TRUE;
	}
	return FALSE;

}
BOOL SHARED_EXPORT uGetTempFileName(const char * path_name,const char * prefix,UINT unique,string_base & out)
{
	if (path_name==0 || prefix==0) return FALSE;
	TCHAR temp[MAX_PATH+1];
	temp[0]=0;
	if (GetTempFileName(stringcvt::string_os_from_utf8(path_name),stringcvt::string_os_from_utf8(prefix),unique,temp))
	{
		out = stringcvt::string_utf8_from_os(temp,_countof(temp));
		return TRUE;
	}
	return FALSE;
}

class uFindFile_i : public uFindFile
{
	string8 fn;
	WIN32_FIND_DATA fd;
	HANDLE hFF;
public:
	uFindFile_i() {hFF = INVALID_HANDLE_VALUE;}
	bool FindFirst(const char * path)
	{
		hFF = FindFirstFile(stringcvt::string_os_from_utf8(path),&fd);
		if (hFF==INVALID_HANDLE_VALUE) return false;
		fn = stringcvt::string_utf8_from_os(fd.cFileName,_countof(fd.cFileName));
		return true;
	}
	virtual BOOL FindNext()
	{
		if (hFF==INVALID_HANDLE_VALUE) return FALSE;
		BOOL rv = FindNextFile(hFF,&fd);
		if (rv) fn = stringcvt::string_utf8_from_os(fd.cFileName,_countof(fd.cFileName));
		return rv;
	}

	virtual const char * GetFileName()
	{
		return fn;
	}

	virtual t_uint64 GetFileSize()
	{
		union
		{
			t_uint64 val64;
			struct
			{
				DWORD lo,hi;
			};
		} ret;
		
		ret.hi = fd.nFileSizeHigh;
		ret.lo = fd.nFileSizeLow;
		return ret.val64;

	}
	virtual DWORD GetAttributes()
	{
		return fd.dwFileAttributes;
	}

	virtual FILETIME GetCreationTime()
	{
		return fd.ftCreationTime;
	}
	virtual FILETIME GetLastAccessTime()
	{
		return fd.ftLastAccessTime;
	}
	virtual FILETIME GetLastWriteTime()
	{
		return fd.ftLastWriteTime;
	}
	virtual ~uFindFile_i()
	{
		if (hFF!=INVALID_HANDLE_VALUE) FindClose(hFF);
	}
};

puFindFile SHARED_EXPORT uFindFirstFile(const char * path)
{
	pfc::ptrholder_t<uFindFile_i> ptr = new uFindFile_i;
	if (!ptr->FindFirst(path)) {
		ptr.release();
		return NULL;
	} else {
		return ptr.detach();
	}
}

HINSTANCE SHARED_EXPORT uShellExecute(HWND wnd,const char * oper,const char * file,const char * params,const char * dir,int cmd)
{
	modal_dialog_scope modal; // IDIOCY - ShellExecute may spawn a modal dialog
	if (wnd) modal.initialize(wnd);
	return ShellExecute(wnd,param_os_from_utf8(oper),param_os_from_utf8(file),param_os_from_utf8(params),param_os_from_utf8(dir),cmd);
}

HWND SHARED_EXPORT uCreateStatusWindow(LONG style,const char * text,HWND parent,UINT id)
{
	return CreateStatusWindow(style,param_os_from_utf8(text),parent,id);
}

HWND SHARED_EXPORT uCreateWindowEx(DWORD dwExStyle,const char * lpClassName,const char * lpWindowName,DWORD dwStyle,int x,int y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam)
{
	return CreateWindowEx(dwExStyle,param_os_from_utf8(lpClassName),param_os_from_utf8(lpWindowName),dwStyle,x,y,nWidth,nHeight,hWndParent,hMenu,hInstance,lpParam);
}

HANDLE SHARED_EXPORT uLoadImage(HINSTANCE hIns,const char * name,UINT type,int x,int y,UINT flags)
{
	return LoadImage(hIns,param_os_from_utf8(name),type,x,y,flags);
}

BOOL SHARED_EXPORT uGetSystemDirectory(string_base & out)
{
	UINT len = GetSystemDirectory(0,0);
	if (len==0) len = MAX_PATH;
	len++;
	array_t<TCHAR> temp;
	temp.set_size(len);
	if (GetSystemDirectory(temp.get_ptr(),len)==0) return FALSE;
	out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
	return TRUE;	
}

BOOL SHARED_EXPORT uGetWindowsDirectory(string_base & out)
{
	UINT len = GetWindowsDirectory(0,0);
	if (len==0) len = MAX_PATH;
	len++;
	array_t<TCHAR> temp;
	temp.set_size(len);
	if (GetWindowsDirectory(temp.get_ptr(),len)==0) return FALSE;
	out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
	return TRUE;	
}

BOOL SHARED_EXPORT uSetCurrentDirectory(const char * path)
{
	return SetCurrentDirectory(stringcvt::string_os_from_utf8(path));
}

BOOL SHARED_EXPORT uGetCurrentDirectory(string_base & out)
{
	UINT len = GetCurrentDirectory(0,0);
	if (len==0) len = MAX_PATH;
	len++;
	array_t<TCHAR> temp;
	temp.set_size(len);
	if (GetCurrentDirectory(len,temp.get_ptr())==0) return FALSE;
	out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
	return TRUE;		
}

BOOL SHARED_EXPORT uExpandEnvironmentStrings(const char * src,string_base & out)
{
	stringcvt::string_os_from_utf8 src_os(src);
	UINT len = ExpandEnvironmentStrings(src_os,0,0);
	if (len==0) len = 256;
	len++;
	array_t<TCHAR> temp;
	temp.set_size(len);
	if (ExpandEnvironmentStrings(src_os,temp.get_ptr(),len)==0) return FALSE;
	out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
	return TRUE;
}

BOOL SHARED_EXPORT uGetUserName(string_base & out)
{
	TCHAR temp[UNLEN+1];
	DWORD len = _countof(temp);
	if (GetUserName(temp,&len))
	{
		out = stringcvt::string_utf8_from_os(temp,_countof(temp));
		return TRUE;
	}
	else return FALSE;
}

BOOL SHARED_EXPORT uGetShortPathName(const char * src,string_base & out)
{
	stringcvt::string_os_from_utf8 src_os(src);
	UINT len = GetShortPathName(src_os,0,0);
	if (len==0) len = MAX_PATH;
	len++;
	array_t<TCHAR> temp; temp.set_size(len);
	if (GetShortPathName(src_os,temp.get_ptr(),len))
	{
		out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
		return TRUE;
	}
	else return FALSE;
}


#ifdef UNICODE
#define DDE_CODEPAGE CP_WINUNICODE
#else
#define DDE_CODEPAGE CP_WINANSI
#endif


HSZ SHARED_EXPORT uDdeCreateStringHandle(DWORD ins,const char * src)
{
	return DdeCreateStringHandle(ins,stringcvt::string_os_from_utf8(src),DDE_CODEPAGE);
}

BOOL SHARED_EXPORT uDdeQueryString(DWORD ins,HSZ hsz,string_base & out)
{
	array_t<TCHAR> temp;
	UINT len = DdeQueryString(ins,hsz,0,0,DDE_CODEPAGE);
	if (len==0) len = MAX_PATH;
	len++;
	temp.set_size(len);
	if (DdeQueryString(ins,hsz,temp.get_ptr(),len,DDE_CODEPAGE))
	{
		out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
		return TRUE;
	}
	else return FALSE;
}

UINT SHARED_EXPORT uDdeInitialize(LPDWORD pidInst,PFNCALLBACK pfnCallback,DWORD afCmd,DWORD ulRes)
{
	return DdeInitialize(pidInst,pfnCallback,afCmd,ulRes);
}

BOOL SHARED_EXPORT uDdeAccessData_Text(HDDEDATA data,string_base & out)
{
	const TCHAR * ptr = (const TCHAR*) DdeAccessData(data,0);
	if (ptr)
	{
		out = stringcvt::string_utf8_from_os(ptr);
		return TRUE;
	}
	else return FALSE;
}

uSortString_t SHARED_EXPORT uSortStringCreate(const char * src) {
	t_size lenEst = pfc::stringcvt::estimate_utf8_to_wide(src,SIZE_MAX);
	TCHAR * ret = pfc::__raw_malloc_t<TCHAR>(lenEst);
	pfc::stringcvt::convert_utf8_to_wide(ret,lenEst,src,SIZE_MAX);
	return reinterpret_cast<uSortString_t>( ret );
}

int SHARED_EXPORT uSortStringCompareEx(uSortString_t string1, uSortString_t string2,uint32_t flags) {
	return CompareString(LOCALE_USER_DEFAULT,flags,reinterpret_cast<const TCHAR*>(string1),-1,reinterpret_cast<const TCHAR*>(string2),-1);
}

int SHARED_EXPORT uSortStringCompare(uSortString_t string1, uSortString_t string2) {
	return lstrcmpi(reinterpret_cast<const TCHAR*>(string1),reinterpret_cast<const TCHAR*>(string2));
}

void SHARED_EXPORT uSortStringFree(uSortString_t string) {
	pfc::__raw_free_t(reinterpret_cast<TCHAR*>(string));
}

HTREEITEM SHARED_EXPORT uTreeView_InsertItem(HWND wnd,const uTVINSERTSTRUCT * param)
{
	stringcvt::string_os_from_utf8 temp;
	temp.convert(param->item.pszText);


	TVINSERTSTRUCT l_param = {};
	l_param.hParent = param->hParent;
	l_param.hInsertAfter = param->hInsertAfter;
	l_param.item.mask = param->item.mask;
	l_param.item.hItem = param->item.hItem;
	l_param.item.state = param->item.state;
	l_param.item.stateMask = param->item.stateMask;
	l_param.item.pszText = const_cast<TCHAR*>(temp.get_ptr());
	l_param.item.cchTextMax = 0;
	l_param.item.iImage = param->item.iImage;
	l_param.item.iSelectedImage = param->item.iImage;
	l_param.item.cChildren = param->item.cChildren;
	l_param.item.lParam = param->item.lParam;
	if (param->item.mask & TVIF_INTEGRAL)
	{
		l_param.itemex.iIntegral = param->itemex.iIntegral;
	}

	return (HTREEITEM) uSendMessage(wnd,TVM_INSERTITEM,0,(LPARAM)&l_param);
}

UINT SHARED_EXPORT uGetFontHeight(HFONT font)
{
	UINT ret;
	HDC dc = CreateCompatibleDC(0);
	SelectObject(dc,font);
	ret = uGetTextHeight(dc);
	DeleteDC(dc);
	return ret;
}


HIMAGELIST SHARED_EXPORT uImageList_LoadImage(HINSTANCE hi, const char * lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags)
{
	return ImageList_LoadImage(hi,param_os_from_utf8(lpbmp),cx,cGrow,crMask,uType,uFlags);
}

int SHARED_EXPORT uTabCtrl_InsertItem(HWND wnd,t_size idx,const uTCITEM * item)
{
	param_os_from_utf8 text((item->mask & TCIF_TEXT) ? item->pszText : 0);
	TCITEM l_item;
	assert(sizeof(l_item)==sizeof(*item));//meh lazy
	memcpy(&l_item,item,sizeof(l_item));
	l_item.pszText = const_cast<TCHAR*>(text.get_ptr());
	l_item.cchTextMax = 0;
	return TabCtrl_InsertItem(wnd,idx,&l_item);
}

int SHARED_EXPORT uTabCtrl_SetItem(HWND wnd,t_size idx,const uTCITEM * item)
{
	param_os_from_utf8 text((item->mask & TCIF_TEXT) ? item->pszText : 0);
	TCITEM l_item;
	PFC_STATIC_ASSERT(sizeof(l_item)==sizeof(*item));//meh lazy
	memcpy(&l_item,item,sizeof(l_item));
	l_item.pszText = const_cast<TCHAR*>(text.get_ptr());
	l_item.cchTextMax = 0;
	return TabCtrl_SetItem(wnd,idx,&l_item);
}

int SHARED_EXPORT uGetKeyNameText(LONG lparam,string_base & out)
{
	TCHAR temp[256];
	temp[0]=0;
	if (!GetKeyNameText(lparam,temp,_countof(temp))) return 0;
	out = stringcvt::string_utf8_from_os(temp,_countof(temp));
	return 1;
}

HANDLE SHARED_EXPORT uCreateFileMapping(HANDLE hFile,LPSECURITY_ATTRIBUTES lpFileMappingAttributes,DWORD flProtect,DWORD dwMaximumSizeHigh,DWORD dwMaximumSizeLow,const char * lpName)
{
	return CreateFileMapping(hFile,lpFileMappingAttributes,flProtect,dwMaximumSizeHigh,dwMaximumSizeLow,param_os_from_utf8(lpName));
}

BOOL SHARED_EXPORT uListBox_GetText(HWND listbox,UINT index,string_base & out)
{
	t_size len = uSendMessage(listbox,LB_GETTEXTLEN,index,0);
	if (len==LB_ERR || len>16*1024*1024) return FALSE;
	if (len==0) {out.reset();return TRUE;}

	array_t<TCHAR> temp; temp.set_size(len+1);
	pfc::memset_t(temp,(TCHAR)0);
	len = uSendMessage(listbox,LB_GETTEXT,index,(LPARAM)temp.get_ptr());
	if (len==LB_ERR) return false;
	out = stringcvt::string_utf8_from_os(temp.get_ptr());
	return TRUE;
}
/*
void SHARED_EXPORT uPrintf(string_base & out,const char * fmt,...)
{
	va_list list;
	va_start(list,fmt);
	uPrintfV(out,fmt,list);
	va_end(list);
}
*/
void SHARED_EXPORT uPrintfV(string_base & out,const char * fmt,va_list arglist)
{
	pfc::string_printf_here_va(out, fmt, arglist);
}

int SHARED_EXPORT uCompareString(DWORD flags,const char * str1,size_t len1,const char * str2,size_t len2)
{
	return CompareString(LOCALE_USER_DEFAULT,flags,stringcvt::string_os_from_utf8(str1,len1),-1,stringcvt::string_os_from_utf8(str2,len2),-1);
}

class uResource_i : public uResource
{
	unsigned size;
	const void * ptr;
public:
	inline uResource_i(const void * p_ptr,unsigned p_size) : ptr(p_ptr), size(p_size)
	{
	}
	virtual const void * GetPointer()
	{
		return ptr;
	}
	virtual unsigned GetSize()
	{
		return size;
	}
	virtual ~uResource_i()
	{
	}
};

puResource SHARED_EXPORT uLoadResource(HMODULE hMod,const char * name,const char * type,WORD wLang)
{
	HRSRC res = uFindResource(hMod,name,type,wLang);
	if (res==0) return 0;
	HGLOBAL hglob = LoadResource(hMod,res);
	if (hglob)
	{
		void * ptr = LockResource(hglob);
		if (ptr)
		{
			return new uResource_i(ptr,SizeofResource(hMod,res));
		}
		else return 0;
	}
	else return 0;
}

puResource SHARED_EXPORT LoadResourceEx(HMODULE hMod,const TCHAR * name,const TCHAR * type,WORD wLang)
{
	HRSRC res = wLang ? FindResourceEx(hMod,type,name,wLang) : FindResource(hMod,name,type);
	if (res==0) return 0;
	HGLOBAL hglob = LoadResource(hMod,res);
	if (hglob)
	{
		void * ptr = LockResource(hglob);
		if (ptr)
		{
			return new uResource_i(ptr,SizeofResource(hMod,res));
		}
		else return 0;
	}
	else return 0;
}

HRSRC SHARED_EXPORT uFindResource(HMODULE hMod,const char * name,const char * type,WORD wLang)
{
	return wLang ? FindResourceEx(hMod,param_os_from_utf8(type),param_os_from_utf8(name),wLang) : FindResource(hMod,param_os_from_utf8(name),param_os_from_utf8(type));
}

BOOL SHARED_EXPORT uLoadString(HINSTANCE ins,UINT id,string_base & out)
{
	BOOL rv = FALSE;
	uResource * res = uLoadResource(ins,uMAKEINTRESOURCE(id),(const char*)(RT_STRING));
	if (res)
	{
		unsigned size = res->GetSize();
		const WCHAR * ptr = (const WCHAR*)res->GetPointer();
		if (size>=4)
		{
			unsigned len = *(const WORD*)(ptr+1);
			if (len * 2 + 4 <= size)
			{
				out = stringcvt::string_utf8_from_wide(ptr+2,len);
			}
		}
		
		delete res;
		rv = TRUE;
	}
	return rv;
}

BOOL SHARED_EXPORT uGetMenuString(HMENU menu,UINT id,string_base & out,UINT flag)
{
	unsigned len = GetMenuString(menu,id,0,0,flag);
	if (len==0)
	{
		out.reset();
		return FALSE;
	}
	array_t<TCHAR> temp;
	temp.set_size(len+1);
	if (GetMenuString(menu,id,temp.get_ptr(),len+1,flag)==0) {
		out.reset();
		return FALSE;
	}
	out = stringcvt::string_utf8_from_os(temp.get_ptr());
	return TRUE;
}

BOOL SHARED_EXPORT uModifyMenu(HMENU menu,UINT id,UINT flags,UINT newitem,const char * data)
{
	return ModifyMenu(menu,id,flags,newitem,param_os_from_utf8(data));
}

UINT SHARED_EXPORT uGetMenuItemType(HMENU menu,UINT position)
{
	MENUITEMINFO info = {};
	info.cbSize = sizeof(info);
	info.fMask = MIIM_TYPE;
	if (!GetMenuItemInfo(menu,position,TRUE,&info))
		return 0;
	return info.fType;
}

static inline bool i_is_path_separator(unsigned c)
{
	return c=='\\' || c=='/' || c=='|' || c==':';
}

int SHARED_EXPORT uSortPathCompare(HANDLE string1,HANDLE string2)
{
	const TCHAR * s1 = reinterpret_cast<const TCHAR*>(string1);
	const TCHAR * s2 = reinterpret_cast<const TCHAR*>(string2);
	const TCHAR * p1, * p2;

	while (*s1 || *s2)
	{
		if (*s1 == *s2)
		{
			s1++;
			s2++;
			continue;
		}

		p1 = s1; while (*p1 && !i_is_path_separator(*p1)) p1++;
		p2 = s2; while (*p2 && !i_is_path_separator(*p2)) p2++;

		if ((!*p1 && !*p2) || (*p1 && *p2))
		{
			int test = CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH, s1, pfc::downcast_guarded<int>(p1 - s1), s2, pfc::downcast_guarded<int>(p2 - s2));
			if (test && test != 2) return test - 2;
			if (!*p1) return 0;
		}
		else
		{
			if (*p1) return -1;
			else return 1;
		}

		s1 = p1 + 1;
		s2 = p2 + 1;
	}
	
	return 0;
}

UINT SHARED_EXPORT uRegisterClipboardFormat(const char * name)
{
	return RegisterClipboardFormat(stringcvt::string_os_from_utf8(name));
}

BOOL SHARED_EXPORT uGetClipboardFormatName(UINT format,string_base & out)
{
	TCHAR temp[1024];
	if (!GetClipboardFormatName(format,temp,_countof(temp))) return FALSE;
	out = stringcvt::string_utf8_from_os(temp,_countof(temp));
	return TRUE;
}

}//extern "C"

BOOL SHARED_EXPORT uSearchPath(const char * path, const char * filename, const char * extension, string_base & p_out)
{
	enum {temp_size = 1024};
	param_os_from_utf8 path_os(path), filename_os(filename), extension_os(extension);
	array_t<TCHAR> temp; temp.set_size(temp_size);
	LPTSTR dummy;
	unsigned len;

	len = SearchPath(path_os,filename_os,extension_os,temp_size,temp.get_ptr(),&dummy);
	if (len == 0) return FALSE;
	if (len >= temp_size)
	{
		unsigned len2;
		temp.set_size(len + 1);
		len2 = SearchPath(path_os,filename_os,extension_os,len+1,temp.get_ptr(),&dummy);
		if (len2 == 0 || len2 > len) return FALSE;
		len = len2;
	}

	p_out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);

	return TRUE;

}

static bool is_ascii_alpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static char ascii_upper(char c)
{
	if (c >= 'a' && c <= 'z') c += 'A' - 'a';
	return c;
}

static BOOL uFixPathCaps_Internal(const char * path,string_base & p_out, bool bQuick) {
	pfc::string8_fastalloc temp, prependbuffer;
	if (path[0] == '\\' && path[1] == '\\')
	{
		unsigned index = 2;
		while(path[index] != '\\')
		{
			if (path[index] == 0) return FALSE;
			index++;
		}
		index++;
		if (path[index] == '\\' || path[index] == 0) return FALSE;
		while(path[index] != '\\')
		{
			if (path[index] == 0) {
				// \\host\share
				uStringLower(p_out,path);
				return TRUE;
			}
			index++;
		}
		index++;
		if (path[index] == '\\') return FALSE;
		uAddStringLower(temp,path,index);
		path += index;
	}
	else if (is_ascii_alpha(path[0]) && path[1] == ':' && path[2] == '\\')
	{
		temp.add_char(ascii_upper(path[0]));
		temp.add_string(":\\");
		path += 3;
	}
	else return FALSE;

	for(;;)
	{
		t_size truncat = temp.length();
		t_size delta = 0;
		while(path[delta]!=0 && path[delta]!='\\') delta++;
		if (delta == 0) break;
		temp.add_string_nc(path,delta);
		
		bool found = false;
		if (!bQuick) {
#ifdef UNICODE
			pfc::winPrefixPath( prependbuffer, temp );
			pfc::ptrholder_t<uFindFile> ff = uFindFirstFile(prependbuffer);
#else
			pfc::ptrholder_t<uFindFile> ff = uFindFirstFile(temp);
#endif
			if (ff.is_valid()) {
				do {
					const char * fn = ff->GetFileName();
					if (!stricmp_utf8_ex(path,delta,fn,strlen(fn)))
					{
						found = true;
						temp.truncate(truncat);
						temp.add_string(fn);
						break;
					}
				} while(ff->FindNext());
			}
		}
		if (!found)
		{
			temp.add_string(path + delta);
			break;
		}
		path += delta;
		if (*path == 0) break;
		path ++;
		temp.add_char('\\');
	}


	p_out = temp;

	return TRUE;
}
/*BOOL SHARED_EXPORT uFixPathCapsQuick(const char * path,string_base & p_out) {
	return uFixPathCaps_Internal(path, p_out, true);
}*/
BOOL SHARED_EXPORT uFixPathCaps(const char * path,string_base & p_out) {
	return uFixPathCaps_Internal(path, p_out, false);
}

LPARAM SHARED_EXPORT uTreeView_GetUserData(HWND p_tree,HTREEITEM p_item)
{
	TVITEM item = {};
	item.mask = TVIF_PARAM;
	item.hItem = p_item;
	if (uSendMessage(p_tree,TVM_GETITEM,0,(LPARAM)&item))
		return item.lParam;
	return 0;
}

bool SHARED_EXPORT uTreeView_GetText(HWND p_tree,HTREEITEM p_item,string_base & p_out)
{
	TCHAR temp[1024];//changeme ?
	TVITEM item = {};
	item.mask = TVIF_TEXT;
	item.hItem = p_item;
	item.pszText = temp;
	item.cchTextMax = _countof(temp);
	if (uSendMessage(p_tree,TVM_GETITEM,0,(LPARAM)&item))
	{
		p_out = stringcvt::string_utf8_from_os(temp,_countof(temp));
		return true;
	}
	else return false;
}

BOOL SHARED_EXPORT uSetWindowText(HWND wnd,const char * p_text)
{
	PFC_ASSERT( wnd != NULL );
	return SetWindowText(wnd,stringcvt::string_os_from_utf8(p_text));
}

BOOL SHARED_EXPORT uSetDlgItemText(HWND wnd,UINT id,const char * p_text)
{
	PFC_ASSERT( wnd != NULL );
	return SetDlgItemText(wnd, id, stringcvt::string_os_from_utf8(p_text));
}

BOOL SHARED_EXPORT uFileExists(const char * fn)
{
	DWORD attrib = uGetFileAttributes(fn);
	if (attrib == 0xFFFFFFFF || (attrib & FILE_ATTRIBUTE_DIRECTORY)) return FALSE;
	return TRUE;
}

BOOL SHARED_EXPORT uFormatSystemErrorMessage(string_base & p_out,DWORD p_code) {
	return pfc::winFormatSystemErrorMessage(p_out, p_code);
}

HMODULE SHARED_EXPORT LoadSystemLibrary(const TCHAR * name) {
	pfc::array_t<TCHAR> buffer; buffer.set_size( MAX_PATH + _tcslen(name) + 2 );
	TCHAR * bufptr = buffer.get_ptr();
	if (GetSystemDirectory(bufptr, MAX_PATH) == 0) return NULL;
	bufptr[MAX_PATH] = 0;

	size_t idx = _tcslen(bufptr);
	if (idx > 0 && bufptr[idx-1] != '\\') bufptr[idx++] = '\\';
	
	pfc::strcpy_t(bufptr+idx, name);
	
	return LoadLibrary(bufptr);
}