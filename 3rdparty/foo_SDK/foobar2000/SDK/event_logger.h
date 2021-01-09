#pragma once

#if defined(FOOBAR2000_DESKTOP) || PFC_DEBUG
// RATIONALE
// Mobile target doesn't really care about event logging, logger interface exists there only for source compat
// We can use macros to suppress all PFC_string_formatter bloat for targets that do not care about any of this
#define FB2K_HAVE_EVENT_LOGGER
#endif

class NOVTABLE event_logger : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(event_logger, service_base);
public:
	enum {
		severity_status,
		severity_warning,
		severity_error
	};
	void log_status(const char * line) {log_entry(line, severity_status);}
	void log_warning(const char * line) {log_entry(line, severity_warning);}
	void log_error(const char * line) {log_entry(line, severity_error);}
	
	virtual void log_entry(const char * line, unsigned severity) = 0;
};

class event_logger_fallback : public event_logger {
public:
	void log_entry(const char * line, unsigned) {console::print(line);}
};

class NOVTABLE event_logger_recorder : public event_logger {
	FB2K_MAKE_SERVICE_INTERFACE( event_logger_recorder , event_logger );
public:
	virtual void playback( event_logger::ptr playTo ) = 0;
	
	static event_logger_recorder::ptr create();
};

#ifdef FB2K_HAVE_EVENT_LOGGER

#define FB2K_LOG_STATUS(X,Y) (X)->log_status(Y)
#define FB2K_LOG_WARNING(X,Y) (X)->log_warning(Y)
#define FB2K_LOG_ERROR(X,Y) (X)->log_error(Y)

#else

#define FB2K_LOG_STATUS(X,Y) ((void)0)
#define FB2K_LOG_WARNING(X,Y) ((void)0)
#define FB2K_LOG_ERROR(X,Y) ((void)0)

#endif
