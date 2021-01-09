#include "foobar2000.h"

void popup_message::g_show_ex(const char * p_msg,unsigned p_msg_length,const char * p_title,unsigned p_title_length,t_icon p_icon)
{
    // Do not force instantiate, not all platforms have this
    service_enum_t< popup_message > e;
    service_ptr_t< popup_message > m;
    if (e.first( m ) ) {
        m->show_ex( p_msg, p_msg_length, p_title, p_title_length, p_icon );
    }
}


void popup_message::g_complain(const char * what) {
    g_show(what, "Information", icon_error);
}

void popup_message::g_complain(const char * p_whatFailed, const std::exception & p_exception) {
    g_complain(p_whatFailed,p_exception.what());
}
void popup_message::g_complain(const char * p_whatFailed, const char * msg) {
    g_complain( PFC_string_formatter() << p_whatFailed << ": " << msg );
}

void popup_message_v3::show_query( const char * title, const char * msg, unsigned buttons, completion_notify::ptr reply) {
    query_t q;
    q.title = title; q.msg = msg; q.buttons = buttons; q.reply = reply;
    this->show_query( q );
}


#ifdef FOOBAR2000_DESKTOP_WINDOWS
void popup_message_v2::g_show(HWND parent, const char * msg, const char * title) {
    service_enum_t< popup_message_v2 > e;
    service_ptr_t< popup_message_v2 > m;
    if (e.first( m )) {
        m->show(parent, msg, title);
    } else {
        popup_message::g_show( msg, title );
    }
}
void popup_message_v2::g_complain(HWND parent, const char * whatFailed, const char * msg) {
    g_show(parent, PFC_string_formatter() << whatFailed << ": " << msg);
}
void popup_message_v2::g_complain(HWND parent, const char * whatFailed, const std::exception & e) {
    g_complain(parent, whatFailed, e.what());
}
#endif // FOOBAR2000_DESKTOP_WINDOWS

#ifdef FOOBAR2000_MODERN
void fb2k::showToast( const char * msg ) {
    fb2k::popup_toast::arg_t arg;
    fb2k::popup_toast::get()->show_toast(msg, arg);
}

void fb2k::showToastLongDuration( const char * msg ) {
    fb2k::popup_toast::arg_t arg;
    arg.longDuration = true;
    fb2k::popup_toast::get()->show_toast(msg, arg);
}
void popup_message::g_showToast(const char * msg) {
    fb2k::showToast( msg );
}
void popup_message::g_showToastLongDuration(const char * msg) {
    fb2k::showToastLongDuration( msg );
}

#endif // FOOBAR2000_MODERN

#ifdef FOOBAR2000_DESKTOP_WINDOWS
int popup_message_v3::messageBox(HWND parent, const char* msg, const char* title, UINT flags) {
	query_t q = {};
	q.title = title;
	q.msg = msg;
	q.wndParent = parent;

	switch (flags & 0xF) {
	default:
	case MB_OK:
		q.buttons = buttonOK;
		q.defButton = buttonOK;
		break;
	case MB_OKCANCEL:
		q.buttons = buttonOK | buttonCancel;
		q.defButton = (flags & MB_DEFBUTTON2) ? buttonCancel : buttonOK;
		break;
	case MB_ABORTRETRYIGNORE:
		q.buttons = buttonAbort | buttonRetry | buttonIgnore;
		if (flags & MB_DEFBUTTON3) q.defButton = buttonIgnore;
		else if (flags & MB_DEFBUTTON2) q.defButton = buttonRetry;
		else q.defButton = buttonAbort;
		break;
	case MB_YESNOCANCEL:
		q.buttons = buttonYes | buttonNo | buttonCancel;
		if (flags & MB_DEFBUTTON3) q.defButton = buttonCancel;
		else if (flags & MB_DEFBUTTON2) q.defButton = buttonNo;
		else q.defButton = buttonYes;
		break;
	case MB_YESNO:
		q.buttons = buttonYes | buttonNo;
		q.defButton = (flags & MB_DEFBUTTON2) ? buttonNo : buttonYes;
		break;
	case MB_RETRYCANCEL:
		q.buttons = buttonRetry | buttonCancel;
		q.defButton = (flags & MB_DEFBUTTON2) ? buttonCancel : buttonRetry;
		break;
	}
	switch (flags & 0xF0) {
	case MB_ICONHAND:
		q.icon = iconWarning;
		break;
	case MB_ICONQUESTION:
		q.icon = iconQuestion;
		break;
	case MB_ICONEXCLAMATION:
		q.icon = iconError;
		break;
	case MB_ICONASTERISK:
		q.icon = iconInformation;
		break;
	}

	uint32_t status = this->show_query_modal(q);
	
	if (status & buttonOK) return IDOK;
	if (status & buttonCancel) return IDCANCEL;
	if (status & buttonYes) return IDYES;
	if (status & buttonNo) return IDNO;
	if (status & buttonRetry) return IDRETRY;
	if (status & buttonAbort) return IDABORT;
	if (status & buttonIgnore) return IDIGNORE;

	return -1;
}
#endif
