#pragma once

#include "win32_misc.h"
#include <libPPUI/WTL-PP.h>
#include <utility>

class CMenuSelectionReceiver : public CWindowImpl<CMenuSelectionReceiver> {
public:
	CMenuSelectionReceiver(HWND p_parent) {
		WIN32_OP( Create(p_parent) != NULL );
	}
	~CMenuSelectionReceiver() {
		if (m_hWnd != NULL) DestroyWindow();
	}
	typedef CWindowImpl<CMenuSelectionReceiver> _baseClass;
	DECLARE_WND_CLASS_EX(TEXT("{DF0087DB-E765-4283-BBAB-6AB2E8AB64A1}"),0,0);

	BEGIN_MSG_MAP(CMenuSelectionReceiver)
		MESSAGE_HANDLER(WM_MENUSELECT,OnMenuSelect)
	END_MSG_MAP()
protected:
	virtual bool QueryHint(unsigned p_id,pfc::string_base & p_out) {
		return false;
	}
private:
	LRESULT OnMenuSelect(UINT,WPARAM p_wp,LPARAM p_lp,BOOL&) {
		if (p_lp != 0) {
			if (HIWORD(p_wp) & MF_POPUP) {
				m_status.release();
			} else {
				pfc::string8 msg;
				UINT cmd = LOWORD(p_wp);
				if ( cmd == 0 || !QueryHint(cmd,msg)) {
					m_status.release();
				} else {
					if (m_status.is_empty()) {
						if (!static_api_ptr_t<ui_control>()->override_status_text_create(m_status)) m_status.release();
					}
					if (m_status.is_valid()) {
						m_status->override_text(msg);
					}
				}
			}
		} else {
			m_status.release();
		}
		return 0;
	}

	service_ptr_t<ui_status_text_override> m_status;

	PFC_CLASS_NOT_COPYABLE(CMenuSelectionReceiver,CMenuSelectionReceiver);
};

class CMenuDescriptionMap : public CMenuSelectionReceiver {
public:
	CMenuDescriptionMap(HWND p_parent) : CMenuSelectionReceiver(p_parent) {}
	void Set(unsigned p_id,const char * p_description) {m_content.set(p_id,p_description);}
protected:
	bool QueryHint(unsigned p_id,pfc::string_base & p_out) {
		return m_content.query(p_id,p_out);
	}
private:
	pfc::map_t<unsigned,pfc::string8> m_content;
};

class CMenuDescriptionHybrid : public CMenuSelectionReceiver {
public:
	CMenuDescriptionHybrid(HWND parent) : CMenuSelectionReceiver(parent) {}
	void Set(unsigned id, const char * desc) {m_content.set(id, desc);}

	void SetCM(contextmenu_manager::ptr mgr, unsigned base, unsigned max) {
		m_cmMgr = mgr; m_cmMgr_base = base; m_cmMgr_max = max;
	}
protected:
	bool QueryHint(unsigned p_id,pfc::string_base & p_out) {
		if (m_cmMgr.is_valid() && p_id >= m_cmMgr_base && p_id < m_cmMgr_max) {
			return m_cmMgr->get_description_by_id(p_id - m_cmMgr_base,p_out);
		}
		return m_content.query(p_id,p_out);
	}
private:
	pfc::map_t<unsigned,pfc::string8> m_content;
	contextmenu_manager::ptr m_cmMgr; unsigned m_cmMgr_base, m_cmMgr_max;
};

inline pfc::string_base & operator<<(pfc::string_base & p_fmt,const CPoint & p_point) {
	return p_fmt << "(" << p_point.x << "," << p_point.y << ")";
}

inline pfc::string_base & operator<<(pfc::string_base & p_fmt,const CRect & p_rect) {
	return p_fmt << "(" << p_rect.left << "," << p_rect.top << "," << p_rect.right << "," << p_rect.bottom << ")";
}

template<typename TClass>
class CAddDummyMessageMap : public TClass {
public:
	BEGIN_MSG_MAP(CAddDummyMessageMap<TClass>)
	END_MSG_MAP()
};

template<typename _parentClass> class CWindowFixSEH : public _parentClass { public:
	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID = 0) {
		__try {
			return _parentClass::ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, dwMsgMapID);
		} __except(uExceptFilterProc(GetExceptionInformation())) { return FALSE; /* should not get here */ }
	}
	template<typename ... arg_t> CWindowFixSEH( arg_t && ... arg ) : _parentClass( std::forward<arg_t>(arg) ... ) {}
};

template<typename TClass>
class CWindowAutoLifetime : public CWindowFixSEH<TClass> {
public:
	typedef CWindowFixSEH<TClass> TBase;
	template<typename ... arg_t> CWindowAutoLifetime(HWND parent, arg_t && ... arg) : TBase( std::forward<arg_t>(arg) ... ) {Init(parent);}
private:
	void Init(HWND parent) {WIN32_OP(this->Create(parent) != NULL);}
	void OnFinalMessage(HWND wnd) {PFC_ASSERT_NO_EXCEPTION( TBase::OnFinalMessage(wnd) ); PFC_ASSERT_NO_EXCEPTION(delete this);}
};

template<typename TClass>
class ImplementModelessTracking : public TClass {
public:
	template<typename ... arg_t> ImplementModelessTracking(arg_t && ... arg ) : TClass(std::forward<arg_t>(arg) ... ) {}
	
	BEGIN_MSG_MAP_EX(ImplementModelessTracking)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_DESTROY(OnDestroy)
		CHAIN_MSG_MAP(TClass)
	END_MSG_MAP_HOOK()
private:
	BOOL OnInitDialog(CWindow, LPARAM) {m_modeless.Set( this->m_hWnd ); SetMsgHandled(FALSE); return FALSE; }
	void OnDestroy() {m_modeless.Set(NULL); SetMsgHandled(FALSE); }
	CModelessDialogEntry m_modeless;
};

namespace fb2k {
	template<typename dialog_t, typename ... arg_t> dialog_t * newDialogEx( HWND parent, arg_t && ... arg ) {
		return new CWindowAutoLifetime<ImplementModelessTracking< dialog_t > > ( parent, std::forward<arg_t>(arg) ... );
	}
	template<typename dialog_t, typename ... arg_t> dialog_t * newDialog(arg_t && ... arg) {
		return new CWindowAutoLifetime<ImplementModelessTracking< dialog_t > > (core_api::get_main_window(), std::forward<arg_t>(arg) ...);
	}
}

class CMenuSelectionReceiver_UiElement : public CMenuSelectionReceiver {
public:
	CMenuSelectionReceiver_UiElement(service_ptr_t<ui_element_instance> p_owner,unsigned p_id_base) : CMenuSelectionReceiver(p_owner->get_wnd()), m_owner(p_owner), m_id_base(p_id_base) {}
protected:
	bool QueryHint(unsigned p_id,pfc::string_base & p_out) {
		return m_owner->edit_mode_context_menu_get_description(p_id,m_id_base,p_out);
	}
private:
	const unsigned m_id_base;
	const service_ptr_t<ui_element_instance> m_owner;
};

static bool window_service_trait_defer_destruction(const service_base *) {return true;}


//! Special service_impl_t replacement for service classes that also implement ATL/WTL windows.
template<typename _t_base>
class window_service_impl_t : public implement_service_query< CWindowFixSEH<_t_base> > {
private:
	typedef window_service_impl_t<_t_base> t_self;
	typedef implement_service_query< CWindowFixSEH<_t_base> > t_base;
public:
	BEGIN_MSG_MAP_EX(window_service_impl_t)
		MSG_WM_DESTROY(OnDestroyPassThru)
		CHAIN_MSG_MAP(__super)
	END_MSG_MAP_HOOK()

	int FB2KAPI service_release() throw() {
		int ret = --m_counter; 
		if (ret == 0) {
			if (window_service_trait_defer_destruction(this) && !InterlockedExchange(&m_delayedDestroyInProgress,1)) {
				PFC_ASSERT_NO_EXCEPTION( service_impl_helper::release_object_delayed(this); );
			} else if (this->m_hWnd != NULL) {
				if (!m_destroyWindowInProgress) { // don't double-destroy in weird scenarios
					PFC_ASSERT_NO_EXCEPTION( ::DestroyWindow(this->m_hWnd) );
				}
			} else {
				PFC_ASSERT_NO_EXCEPTION( delete this );
			}
		}
		return ret;
	}
	int FB2KAPI service_add_ref() throw() {return ++m_counter;}

	template<typename ... arg_t>
	window_service_impl_t( arg_t && ... arg ) : t_base( std::forward<arg_t>(arg) ... ) {};
private:
	void OnDestroyPassThru() {
		SetMsgHandled(FALSE); m_destroyWindowInProgress = true;
	}
	void OnFinalMessage(HWND p_wnd) {
		t_base::OnFinalMessage(p_wnd);
		service_ptr_t<service_base> bump(this);
	}
	volatile bool m_destroyWindowInProgress = false;
	volatile LONG m_delayedDestroyInProgress = 0;
	pfc::refcounter m_counter;
};

namespace fb2k {
	template<typename obj_t, typename ... arg_t> 
	service_ptr_t<obj_t> service_new_window(arg_t && ... arg) {
		return new window_service_impl_t< obj_t > ( std::forward<arg_t> (arg) ... );
	}
}

static void AppendMenuPopup(HMENU menu, UINT flags, CMenu & popup, const TCHAR * label) {
	PFC_ASSERT( flags & MF_POPUP );
	WIN32_OP( CMenuHandle(menu).AppendMenu(flags, popup, label) );
	popup.Detach();
}

class CMessageMapDummy : public CMessageMap { 
public:
	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		LRESULT& lResult, DWORD dwMsgMapID) {return FALSE;}
};







template<typename TDialog> class preferences_page_instance_impl : public TDialog {
public:
	preferences_page_instance_impl(HWND parent, preferences_page_callback::ptr callback) : TDialog(callback) {
		WIN32_OP(this->Create(parent) != NULL);

		// complain early if what we created isn't a child window
		PFC_ASSERT( (this->GetStyle() & (WS_POPUP|WS_CHILD)) == WS_CHILD );
	}
	HWND get_wnd() {return this->m_hWnd;}
};
static bool window_service_trait_defer_destruction(const preferences_page_instance *) {return false;}
template<typename TDialog> class preferences_page_impl : public preferences_page_v3 {
public:
	preferences_page_instance::ptr instantiate(HWND parent, preferences_page_callback::ptr callback) {
		return fb2k::service_new_window<preferences_page_instance_impl<TDialog> >(parent, callback);
	}
};

class CEmbeddedDialog : public CDialogImpl<CEmbeddedDialog> {
public:
	CEmbeddedDialog(CMessageMap * owner, DWORD msgMapID, UINT dialogID) : m_owner(*owner), IDD(dialogID), m_msgMapID(msgMapID) {}
	
	BEGIN_MSG_MAP(CEmbeddedDialog)
		CHAIN_MSG_MAP_ALT_MEMBER(m_owner, m_msgMapID)
	END_MSG_MAP()

	const DWORD m_msgMapID;
	const UINT IDD;
	CMessageMap & m_owner;
};




// here because of window_service_impl_t
template<typename TImpl, typename TInterface = ui_element> class ui_element_impl : public TInterface {
public:
	GUID get_guid() { return TImpl::g_get_guid(); }
	GUID get_subclass() { return TImpl::g_get_subclass(); }
	void get_name(pfc::string_base & out) { TImpl::g_get_name(out); }
	ui_element_instance::ptr instantiate(HWND parent, ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback) {
		PFC_ASSERT(cfg->get_guid() == get_guid());
		service_nnptr_t<ui_element_instance_impl_helper> item = new window_service_impl_t<ui_element_instance_impl_helper>(cfg, callback);
		item->initialize_window(parent);
		return item;
	}
	ui_element_config::ptr get_default_configuration() { return TImpl::g_get_default_configuration(); }
	ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg) { return NULL; }
	bool get_description(pfc::string_base & out) { out = TImpl::g_get_description(); return true; }
private:
	class ui_element_instance_impl_helper : public TImpl {
	public:
		ui_element_instance_impl_helper(ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback) : TImpl(cfg, callback) {}

		GUID get_guid() { return TImpl::g_get_guid(); }
		GUID get_subclass() { return TImpl::g_get_subclass(); }
		HWND get_wnd() { return *this; }
	};
public:
	typedef ui_element_instance_impl_helper TInstance;
	static TInstance const & instanceGlobals() { return *reinterpret_cast<const TInstance*>(NULL); }
};
