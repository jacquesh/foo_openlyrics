#pragma once
#include "string-lite.h"
#include <functional>

#ifdef __ANDROID__
#define PFC_SET_THREAD_DESCRIPTION_EXTERNAL
#endif

namespace pfc {
    [[noreturn]] void crash();
    [[noreturn]] void crashWithMessageOnStack( const char * msg );
    void outputDebugLine(const char * msg);
    
	// Debug logger service.
	// It is up to the caller to ensure thread safety. You want to create debugLineReceiver instances on app startup and never destroy them.
	class debugLineReceiver {
	public:
		debugLineReceiver();
		virtual void onLine( const char *) {}


		static void dispatch( const char * msg );
	private:
		debugLineReceiver * m_chain;

		void operator=( const debugLineReceiver & ) = delete;
		debugLineReceiver( const debugLineReceiver & ) = delete;
	};

    class debugLog : public string_formatter {
    public:
        ~debugLog() { outputDebugLine(this->get_ptr()); }
    };
#define PFC_DEBUGLOG ::pfc::debugLog()._formatter()

    void setCurrentThreadDescription( const char * );

#ifdef PFC_SET_THREAD_DESCRIPTION_EXTERNAL
	void initSetCurrentThreadDescription( std::function<void (const char*)> );
#endif	
}

#define PFC_SET_THREAD_DESCRIPTION(X) { ::pfc::setCurrentThreadDescription(X); }
#define PFC_SET_THREAD_DESCRIPTION_SUPPORTED
