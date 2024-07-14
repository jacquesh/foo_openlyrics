#pragma once

#include <climits> // UINT_MAX
#include <functional>
#include "completion_notify.h"

//! This interface allows you to show generic nonmodal noninteractive dialog with a text message. This should be used instead of MessageBox where possible.\n
//! Usage: use popup_message::g_show / popup_message::g_show_ex static helpers, or popup_message::get() to obtain an instance.\n
//! Thread safety: OK to call from worker threads (will delegate UI ops to main thread automatically). \n
//! Note that all strings are UTF-8.
class NOVTABLE popup_message : public service_base {
public:
	enum t_icon {icon_information, icon_error, icon_query};
	//! Activates the popup dialog; returns immediately (the dialog remains visible).
	//! @param p_msg Message to show (UTF-8 encoded string).
	//! @param p_msg_length Length limit of message string to show, in bytes (actual string may be shorter if null terminator is encountered before). Set this to infinite to use plain null-terminated strings.
	//! @param p_title Title of dialog to show (UTF-8 encoded string).
	//! @param p_title_length Length limit of the title string, in bytes (actual string may be shorter if null terminator is encountered before). Set this to infinite to use plain null-terminated strings.
	//! @param p_icon Icon of the dialog - can be set to icon_information, icon_error or icon_query.
	virtual void show_ex(const char * p_msg,size_t p_msg_length,const char * p_title,size_t p_title_length,t_icon p_icon = icon_information) = 0;

	//! Activates the popup dialog; returns immediately (the dialog remains visible); helper function built around show_ex(), takes null terminated strings with no length limit parameters.
	//! @param p_msg Message to show (UTF-8 encoded string).
	//! @param p_title Title of dialog to show (UTF-8 encoded string).
	//! @param p_icon Icon of the dialog - can be set to icon_information, icon_error or icon_query.
	inline void show(const char * p_msg,const char * p_title,t_icon p_icon = icon_information) {show_ex(p_msg,UINT_MAX,p_title,UINT_MAX,p_icon);}

	//! Static helper function instantiating the service and activating the message dialog. See show_ex() for description of parameters.
	static void g_show_ex(const char * p_msg,size_t p_msg_length,const char * p_title,size_t p_title_length,t_icon p_icon = icon_information);
	//! Static helper function instantiating the service and activating the message dialog. See show() for description of parameters.
	static inline void g_show(const char * p_msg,const char * p_title,t_icon p_icon = icon_information) {g_show_ex(p_msg,UINT_MAX,p_title,UINT_MAX,p_icon);}

	//! Shows generic box with a failure message
	static void g_complain(const char * what);
	//! <whatfailed>: <exception message>
	static void g_complain(const char * p_whatFailed, const std::exception & p_exception);
	//! <whatfailed>: <msg>
	static void g_complain(const char * p_whatFailed, const char * msg);

    static void g_showToast(const char * msg);
    static void g_showToastLongDuration(const char * msg);

    FB2K_MAKE_SERVICE_COREAPI(popup_message);
};

#define EXCEPTION_TO_POPUP_MESSAGE(CODE,LABEL) try { CODE; } catch(std::exception const & e) {popup_message::g_complain(LABEL,e);}

//! \since 1.1
//! Extendsion to popup_message API. \n
//! Thread safety: OK to call from worker threads (will delegate UI ops to main thread automatically). \n
//! Note that all strings are UTF-8.
class NOVTABLE popup_message_v2 : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(popup_message_v2);
public:
	virtual void show(fb2k::hwnd_t parent, const char * msg, t_size msg_length, const char * title, t_size title_length) = 0;
	void show(fb2k::hwnd_t parent, const char * msg, const char * title) {show(parent, msg, SIZE_MAX, title, SIZE_MAX);}

	static void g_show(fb2k::hwnd_t parent, const char * msg, const char * title = "Information");
	static void g_complain(fb2k::hwnd_t parent, const char * whatFailed, const char * msg);
	static void g_complain(fb2k::hwnd_t parent, const char * whatFailed, const std::exception & e);
};

namespace fb2k {
	//! \since 2.0
	//! Not really implemented for Windows yet.
	class popup_toast : public service_base {
		FB2K_MAKE_SERVICE_COREAPI( popup_toast );
	public:
		struct arg_t {
            bool longDuration = false;
        };
		virtual void show_toast(const char * msg, arg_t const & arg) = 0;
	};
	void showToast( const char * msg );
	void showToastLongDuration( const char * msg );

	class toastFormatter : public pfc::string_formatter {
	public:
		~toastFormatter() noexcept {
			try {
				if ( this->length() > 0 ) showToast( c_str() );
			} catch(...) {}
		}
    };
}

#define FB2K_Toast() ::fb2k::toastFormatter()._formatter()


//! \since 1.5
//! MessageBox-like dialog, only non-blocking and with dark mode support under foobar2000 v2.0. \n
//! Call from main thread only (contrary to popup_message / popup_message_v2) !!!
class NOVTABLE popup_message_v3 : public service_base {
    FB2K_MAKE_SERVICE_COREAPI(popup_message_v3);
public:

    //! show_query button codes. \n
    //! Combine one or more of these to create a button mask to pass to show_query().
    enum {
        buttonOK = 1 << 0,
        buttonCancel = 1 << 1,
        buttonYes = 1 << 2,
        buttonNo = 1 << 3,
        buttonRetry = 1 << 4,
        buttonAbort = 1 << 5,
        buttonIgnore = 1 << 6,

        flagDoNotAskAgain = 1 << 16,

        iconNone = 0,
        iconInformation,
        iconQuestion,
        iconWarning,
        iconError,
    };

    struct query_t {
        const char * title = nullptr;
        const char * msg = nullptr;
        uint32_t buttons = 0;
        uint32_t defButton = 0;
        uint32_t icon = iconNone;
        completion_notify::ptr reply; // optional
        fb2k::hwnd_t wndParent = NULL;
        const char * msgDoNotAskAgain = nullptr;

		void show();
    };

    //! Shows an interactive query presenting the user with multiple actions to choose from.
    virtual void show_query(query_t const &) = 0;

    //! Modal version of show_query. Reply part of the argument can be empty; the status code will be returned.
    virtual uint32_t show_query_modal(query_t const &) = 0;

	// Minimalist MessageBox() reimplementation wrapper
	int messageBox(fb2k::hwnd_t, const char*, const char*, unsigned);
	void messageBoxAsync(fb2k::hwnd_t, const char*, const char*, unsigned, std::function<void (int)> reply = nullptr);
	static query_t setupMessageBox(fb2k::hwnd_t parent, const char* msg, const char* title, unsigned flags);
	static int messageBoxReply(uint32_t);
    
    //! Old method wrapper
    void show_query( const char * title, const char * msg, unsigned buttons, completion_notify::ptr reply);
};
