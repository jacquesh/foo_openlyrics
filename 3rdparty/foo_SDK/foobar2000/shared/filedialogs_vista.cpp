#include "shared.h"
#include "filedialogs.h"
#include <shlobj.h>
#include <shobjidl.h>
#include <shtypes.h>
#include <atlbase.h>
#include <atlcom.h>

#define dTEXT(X) pfc::stringcvt::string_os_from_utf8(X)

class FilterSpec {
public:
	void clear() {
		m_types.set_size(0); m_strings.remove_all();
	}
	void Sanity() {
		if ( GetCount() > 200 ) SetAllFiles();
	}
	void SetAllFiles() {
		FromString( "All files|*.*" );
	}
	void FromString(const char * in) {
		clear();
		if (in == NULL) return;
		pfc::chain_list_v2_t<COMDLG_FILTERSPEC> types;

		for(t_size inWalk = 0; ; ) {
			
			t_size base1 = inWalk;
			t_size delta1 = ScanForSeparator(in+base1);
			t_size base2 = base1 + delta1;
			if (in[base2] == 0) break;
			++base2;
			t_size delta2 = ScanForSeparator(in+base2);
			if (delta1 > 0 && delta2 > 0) {
				COMDLG_FILTERSPEC spec;
				spec.pszName = MakeString(in+base1,delta1);
				spec.pszSpec = MakeString(in+base2,delta2);
				types.add_item(spec);
			}
			inWalk = base2 + delta2;
			if (in[inWalk] == 0) break;
			++inWalk;
		}

		pfc::list_to_array(m_types,types);
	}

	t_size GetCount() const {return m_types.get_count();}
	const COMDLG_FILTERSPEC * GetPtr() const {return m_types.get_ptr();}
private:
	static t_size ScanForSeparator(const char * in) {
		for(t_size walk = 0; ; ++walk) {
			if (in[walk] == 0 || in[walk] == '|') return walk;
		}
	}
	WCHAR * MakeString(const char * in, t_size inLen) {
		t_size len = pfc::stringcvt::estimate_utf8_to_wide(in,inLen);
		WCHAR* str = AllocString(len);
		pfc::stringcvt::convert_utf8_to_wide(str,len,in,inLen);
		return str;
	}
	WCHAR * AllocString(t_size size) {
		auto iter = m_strings.insert_last();
		iter->set_size(size);
		return iter->get_ptr();
	}
	pfc::chain_list_v2_t<pfc::array_t<WCHAR> > m_strings;
	pfc::array_t<COMDLG_FILTERSPEC> m_types;
};

static HRESULT AddOptionsHelper(pfc::com_ptr_t<IFileDialog> dlg, DWORD opts) {
	DWORD options;
	HRESULT state;
	if (FAILED(state = dlg->GetOptions(&options))) return state;
	options |= opts;
	if (FAILED(state = dlg->SetOptions( options ))) return state;
	return S_OK;
}

namespace {

	// SPECIAL
	// Deal with slow or nonworking net shares, do not lockup the calling thread in such cases, just split away
	// Particularly relevant to net shares referred by raw IP, these get us stuck for a long time
	class PDNArg_t : public pfc::refcounted_object_root {
	public:
		CoTaskMemObject<LPITEMIDLIST> m_idList;
		pfc::string8 m_path;
		HRESULT m_result;
	};

	static unsigned CALLBACK PDNProc(void * arg) {
		pfc::refcounted_object_ptr_t<PDNArg_t> ptr; ptr.attach( reinterpret_cast<PDNArg_t*>( arg ) );
		CoInitialize(0);

		SFGAOF dummy;
		ptr->m_result = SHParseDisplayName(dTEXT(ptr->m_path),NULL,&ptr->m_idList.m_ptr,0,&dummy);

		CoUninitialize();

		return 0;
	}
}

static HRESULT SetFolderHelper(pfc::com_ptr_t<IFileDialog> dlg, const char * folderPath) {
	CoTaskMemObject<LPITEMIDLIST> idList;

	// Do SHParseDisplayName() off-thread as it is known to lock up on bad net share references
	pfc::refcounted_object_ptr_t<PDNArg_t> ptr = new PDNArg_t();
	ptr->m_path = folderPath;
	ptr->m_result = E_FAIL;
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, PDNProc, reinterpret_cast<void*>(ptr._duplicate_ptr()), 0, NULL);
	DWORD status = WaitForSingleObject( hThread, 3000 );
	CloseHandle(hThread);
	if (status != WAIT_OBJECT_0) return E_FAIL;
	if (FAILED(ptr->m_result)) return ptr->m_result;

	pfc::com_ptr_t<IShellItem> item;
	HRESULT state;
	if (FAILED(state = SHCreateShellItem(NULL,NULL,ptr->m_idList.m_ptr,item.receive_ptr()))) return state;
	return dlg->SetFolder(item.get_ptr());
}

namespace {
	class _EH {
	public:
		void operator<<(HRESULT hr) {
			if (FAILED(hr)) throw exception_com(hr);
		}
	};
	static _EH EH;
};

BOOL Vista_GetOpenFileName(HWND parent,const char * p_ext_mask,unsigned def_ext_mask,const char * p_def_ext,const char * p_title,const char * p_directory,pfc::string_base & p_filename,BOOL b_save) {
	modal_dialog_scope modalScope(parent);

	pfc::com_ptr_t<IFileDialog> dlg;

	if (b_save) {
		if (FAILED(CoCreateInstance(__uuidof(FileSaveDialog), NULL, CLSCTX_ALL, IID_IFileSaveDialog, (void**) dlg.receive_ptr()))) throw pfc::exception_not_implemented();
	} else {
		if (FAILED(CoCreateInstance(__uuidof(FileOpenDialog), NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**) dlg.receive_ptr()))) throw pfc::exception_not_implemented();
	}

	{
		FilterSpec spec; spec.FromString(p_ext_mask);
		spec.Sanity();
		if (FAILED(dlg->SetFileTypes((UINT)spec.GetCount(),spec.GetPtr()))) return FALSE;
		if (def_ext_mask < spec.GetCount()) {
			if (FAILED(dlg->SetFileTypeIndex(def_ext_mask + 1))) return FALSE;
		}
	}
	if (p_def_ext != NULL) {
		if (FAILED(dlg->SetDefaultExtension(dTEXT(p_def_ext)))) return FALSE;
	}
	if (p_title != NULL) {
		if (FAILED(dlg->SetTitle(dTEXT(p_title)))) return FALSE;
	}

	if (!p_filename.is_empty()) {
		pfc::string path(p_filename);
		pfc::string fn = pfc::io::path::getFileName(path);
		pfc::string parent = pfc::io::path::getParent(path);
		if (!parent.isEmpty()) SetFolderHelper(dlg,parent.ptr());
		dlg->SetFileName(dTEXT(fn.ptr()));
	} else if (p_directory != NULL) {
		SetFolderHelper(dlg,p_directory);
	}

	if (FAILED(AddOptionsHelper(dlg, FOS_FORCEFILESYSTEM))) return FALSE;

	if (FAILED( dlg->Show(parent) ) ) return FALSE;

	{
		pfc::com_ptr_t<IShellItem> result;
		if (FAILED(dlg->GetResult(result.receive_ptr()))) return FALSE;

		CoTaskMemObject<WCHAR*> nameBuf;
		if (FAILED(result->GetDisplayName(SIGDN_FILESYSPATH,&nameBuf.m_ptr))) return FALSE;
		
		p_filename = pfc::stringcvt::string_utf8_from_os_ex( nameBuf.m_ptr );
	}
	return TRUE;
}

BOOL Vista_GetOpenFileNameMulti(HWND parent,const char * p_ext_mask,unsigned def_ext_mask,const char * p_def_ext,const char * p_title,const char * p_directory,pfc::ptrholder_t<uGetOpenFileNameMultiResult> & out) {
	modal_dialog_scope modalScope(parent);

	pfc::com_ptr_t<IFileOpenDialog> dlg;
	if (FAILED(CoCreateInstance(__uuidof(FileOpenDialog), NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**) dlg.receive_ptr()))) throw pfc::exception_not_implemented();

	{
		FilterSpec spec; spec.FromString(p_ext_mask);
		spec.Sanity();
		if (FAILED(dlg->SetFileTypes((UINT)spec.GetCount(),spec.GetPtr()))) return FALSE;
		if (def_ext_mask < spec.GetCount()) {
			if (FAILED(dlg->SetFileTypeIndex(def_ext_mask + 1))) return FALSE;
		}
	}
	if (p_def_ext != NULL) {
		if (FAILED(dlg->SetDefaultExtension(dTEXT(p_def_ext)))) return FALSE;
	}
	if (p_title != NULL) {
		if (FAILED(dlg->SetTitle(dTEXT(p_title)))) return FALSE;
	}

	if (p_directory != NULL) {
		SetFolderHelper(dlg,p_directory);
	}

	if (FAILED(AddOptionsHelper(dlg, FOS_ALLOWMULTISELECT | FOS_FORCEFILESYSTEM))) return FALSE;

	if (FAILED( dlg->Show(parent) ) ) return FALSE;

	{
		pfc::ptrholder_t<uGetOpenFileNameMultiResult_impl> result = new uGetOpenFileNameMultiResult_impl;
		pfc::com_ptr_t<IShellItemArray> results;
		if (FAILED(dlg->GetResults(results.receive_ptr()))) return FALSE;
		DWORD total;
		if (FAILED(results->GetCount(&total))) return FALSE;
		for(DWORD itemWalk = 0; itemWalk < total; ++itemWalk) {
			pfc::com_ptr_t<IShellItem> item;
			if (SUCCEEDED(results->GetItemAt(itemWalk,item.receive_ptr()))) {
				CoTaskMemObject<WCHAR*> nameBuf;
				if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH,&nameBuf.m_ptr))) return FALSE;
				
				result->AddItem(pfc::stringcvt::string_utf8_from_os_ex( nameBuf.m_ptr ) );
				
			}
		}

		if (result->get_count() == 0) return FALSE;

		out = result.detach();
	}

	return TRUE;
}
#if 0
namespace {
	class CFileDialogEvents_LocateFile : public IFileDialogEvents {
	public:
		CFileDialogEvents_LocateFile(pfc::stringp tofind) : m_tofind(tofind) {}
		HRESULT STDMETHODCALLTYPE OnFileOk(IFileDialog *pfd) {return S_OK;}
        
		HRESULT STDMETHODCALLTYPE OnFolderChanging( IFileDialog *pfd, IShellItem *psiFolder) {return S_OK;}
        
		HRESULT STDMETHODCALLTYPE OnFolderChange( IFileDialog *pfd ) {
			//pfd->GetFolder();
			return S_OK;
		}
        
		HRESULT STDMETHODCALLTYPE OnSelectionChange( IFileDialog *pfd ) {return S_OK;}
        
		HRESULT STDMETHODCALLTYPE OnShareViolation(  IFileDialog *pfd, IShellItem *psi, FDE_SHAREVIOLATION_RESPONSE *pResponse) { return S_OK; }
        
		HRESULT STDMETHODCALLTYPE OnTypeChange( IFileDialog *pfd ) {return S_OK; }
        
		HRESULT STDMETHODCALLTYPE OnOverwrite( IFileDialog *pfd, IShellItem *psi, FDE_OVERWRITE_RESPONSE *pResponse) {return S_OK; }
		
	private:
		const pfc::string m_tofind;
	};
	
}
#endif
BOOL Vista_BrowseForFolder(HWND parent, const char * p_title, pfc::string_base & path) {
	modal_dialog_scope modalScope(parent);
	pfc::com_ptr_t<IFileOpenDialog> dlg;
	if (FAILED(CoCreateInstance(__uuidof(FileOpenDialog), NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**) dlg.receive_ptr()))) throw pfc::exception_not_implemented();
	
	if (p_title != NULL) {
		if (FAILED(dlg->SetTitle(dTEXT(p_title)))) return FALSE;
	}

	if (FAILED(AddOptionsHelper(dlg, FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM))) return FALSE;

	if (!path.is_empty()) {
		SetFolderHelper(dlg,path);
	}

	if (FAILED( dlg->Show(parent) ) ) return FALSE;

	{
		pfc::com_ptr_t<IShellItem> result;
		if (FAILED(dlg->GetResult(result.receive_ptr()))) return FALSE;

		CoTaskMemObject<WCHAR*> nameBuf;
		if (FAILED(result->GetDisplayName(SIGDN_FILESYSPATH,&nameBuf.m_ptr))) return FALSE;
		
		path = pfc::stringcvt::string_utf8_from_os_ex( nameBuf.m_ptr );
	}
	return TRUE;
}


__inline HRESULT mySHLoadLibraryFromItem(
    __in IShellItem *psiLibrary,
    __in DWORD grfMode,
    __in REFIID riid,
    __deref_out void **ppv
)
{
    *ppv = NULL;
    IShellLibrary *plib;

    HRESULT hr = CoCreateInstance(
      CLSID_ShellLibrary, 
      NULL, 
      CLSCTX_INPROC_SERVER, 
      IID_PPV_ARGS(&plib));

    if (SUCCEEDED(hr))
    {
        hr = plib->LoadLibraryFromItem (psiLibrary, grfMode);
        if (SUCCEEDED(hr))
        {
            hr = plib->QueryInterface (riid, ppv);
        }
        plib->Release();
    }
    return hr;
}

//
// from shobjidl.h
//
__inline HRESULT mySHLoadLibraryFromKnownFolder(
    __in REFKNOWNFOLDERID kfidLibrary, 
    __in DWORD grfMode, 
    __in REFIID riid, 
    __deref_out void **ppv)
{
    *ppv = NULL;
    IShellLibrary *plib;
    HRESULT hr = CoCreateInstance( 
        CLSID_ShellLibrary,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&plib));
    if (SUCCEEDED(hr))
    {
        hr = plib->LoadLibraryFromKnownFolder(kfidLibrary, grfMode);
        if (SUCCEEDED(hr))
        {
            hr = plib->QueryInterface(riid, ppv);
        }
        plib->Release();
    }
    return hr;
}

puGetOpenFileNameMultiResult Vista_BrowseForFolderEx(HWND parent,const char * p_title, const char * initPath) {
	try {
		modal_dialog_scope modalScope(parent);
		pfc::com_ptr_t<IFileOpenDialog> dlg;
		if (FAILED(CoCreateInstance(__uuidof(FileOpenDialog), NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**) dlg.receive_ptr()))) throw pfc::exception_not_implemented();
	
		if (p_title != NULL) {
			EH << dlg->SetTitle(dTEXT(p_title));
		}

		EH << AddOptionsHelper(dlg, FOS_ALLOWMULTISELECT | FOS_PICKFOLDERS);

		if (initPath && *initPath) {
			SetFolderHelper(dlg,initPath);
		}

		EH << dlg->Show(parent);

		pfc::ptrholder_t<uGetOpenFileNameMultiResult_impl> result = new uGetOpenFileNameMultiResult_impl;
		pfc::com_ptr_t<IShellItemArray> results;
		EH << dlg->GetResults(results.receive_ptr());
		DWORD total;
		EH << results->GetCount(&total);
		CoTaskMemObject<WCHAR*> nameBuf;
		for(DWORD itemWalk = 0; itemWalk < total; ++itemWalk) {
			pfc::com_ptr_t<IShellItem> item;
			EH << results->GetItemAt(itemWalk,item.receive_ptr());
			if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,nameBuf.Receive()))) {
				result->AddItem(pfc::stringcvt::string_utf8_from_os_ex( nameBuf.m_ptr ) );
			} else {
				pfc::com_ptr_t<IShellLibrary> library;
				if (SUCCEEDED(mySHLoadLibraryFromItem(item.get_ptr(), STGM_READ, IID_IShellLibrary, (void**)library.receive_ptr()))) {
					pfc::com_ptr_t<IShellItemArray> subFolders;
					EH << library->GetFolders(LFF_FORCEFILESYSTEM, IID_IShellItemArray, (void**)subFolders.receive_ptr());
					DWORD subTotal;
					EH << subFolders->GetCount(&subTotal);
					for(DWORD subWalk = 0; subWalk < subTotal; ++subWalk) {
						pfc::com_ptr_t<IShellItem> subItem;
						EH << subFolders->GetItemAt(subWalk,subItem.receive_ptr());
						EH << subItem->GetDisplayName(SIGDN_FILESYSPATH,nameBuf.Receive());
						result->AddItem(pfc::stringcvt::string_utf8_from_os_ex( nameBuf.m_ptr ) );
					}
				}
			}
		}
		if (result->GetCount() == 0) return NULL;
		return result.detach();
	} catch(exception_com) {
		return NULL;
	}
}

static bool GetLegacyKnownFolder(int & out, REFKNOWNFOLDERID id) {
	if (id == FOLDERID_Music) {
		out = CSIDL_MYMUSIC;
		return true;
	} else if (id == FOLDERID_Pictures) {
		out = CSIDL_MYPICTURES;
		return true;
	} else if (id == FOLDERID_Videos) {
		out = CSIDL_MYVIDEO;
		return true;
	} else if (id == FOLDERID_Documents) {
		out = CSIDL_MYDOCUMENTS;
		return true;
	} else if (id == FOLDERID_Desktop) {
		out = CSIDL_DESKTOP;
		return true;
	} else {
		return false;
	}
}

puGetOpenFileNameMultiResult SHARED_EXPORT uEvalKnownFolder(REFKNOWNFOLDERID id) {
	try {
		pfc::com_ptr_t<IShellLibrary> library;
		EH << mySHLoadLibraryFromKnownFolder(id, STGM_READ, IID_IShellLibrary, (void**)library.receive_ptr());

		pfc::com_ptr_t<IShellItemArray> subFolders;
		EH << library->GetFolders(LFF_FORCEFILESYSTEM, IID_IShellItemArray, (void**)subFolders.receive_ptr());

		pfc::ptrholder_t<uGetOpenFileNameMultiResult_impl> result = new uGetOpenFileNameMultiResult_impl;

		DWORD subTotal;
		EH << subFolders->GetCount(&subTotal);
		CoTaskMemObject<WCHAR*> nameBuf;
		for(DWORD subWalk = 0; subWalk < subTotal; ++subWalk) {
			pfc::com_ptr_t<IShellItem> subItem;
			EH << subFolders->GetItemAt(subWalk,subItem.receive_ptr());
			EH << subItem->GetDisplayName(SIGDN_FILESYSPATH,nameBuf.Receive());
			result->AddItem(pfc::stringcvt::string_utf8_from_os_ex( nameBuf.m_ptr ) );
		}
		if (result->get_count() == 0) return NULL;
		return result.detach();
	} catch(exception_com) {
		//failed
	}


	try {
		CComPtr<IKnownFolderManager> mgr; CComPtr<IKnownFolder> folder; CoTaskMemObject<wchar_t*> path;
		EH << CoCreateInstance(__uuidof(KnownFolderManager), nullptr, CLSCTX_ALL, IID_IKnownFolderManager, (void**)&mgr.p);
		EH << mgr->GetFolder(id, &folder.p);
		EH << folder->GetPath(0, &path.m_ptr);

		pfc::ptrholder_t<uGetOpenFileNameMultiResult_impl> result = new uGetOpenFileNameMultiResult_impl;
		result->AddItem(pfc::stringcvt::string_utf8_from_os(path.m_ptr));
		return result.detach();
	} catch (exception_com) {

	}

	//FALLBACK
	


	{
		int legacyID;
		if (GetLegacyKnownFolder(legacyID, id)) {
			try {
				TCHAR path[MAX_PATH+16] = {};
				EH << SHGetFolderPath(NULL, legacyID, NULL, SHGFP_TYPE_CURRENT, path);
				path[_countof(path)-1] = 0;
				pfc::ptrholder_t<uGetOpenFileNameMultiResult_impl> result = new uGetOpenFileNameMultiResult_impl;
				result->AddItem(pfc::stringcvt::string_utf8_from_os_ex( path ) );
				return result.detach();
			} catch(exception_com) {
				//failed;
			}
		}
	}
#if 0 // Vista code path - uninteresting, XP shit still needs to be supported, SHGetKnownFolderPath() does not exist on XP 
	try {
		pfc::ptrholder_t<uGetOpenFileNameMultiResult_impl> result = new uGetOpenFileNameMultiResult_impl;
		CoTaskMemObject<WCHAR*> nameBuf;
		EH << SHGetKnownFolderPath(id, 0, NULL, nameBuf.Receive());
		result->AddItem(pfc::stringcvt::string_utf8_from_os_ex( nameBuf.m_ptr ) );
		return result.detach();
	} catch(exception_com) {
		//failed
	}
#endif
	return NULL; //failure
}
