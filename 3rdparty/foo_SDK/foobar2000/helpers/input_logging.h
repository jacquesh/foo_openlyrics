#pragma once


class input_logging : public input_stubs {
public:
	input_logging() {
		set_logger(nullptr);
	}

	event_logger_recorder::ptr log_record( std::function<void () > f ) { 
		auto rec = event_logger_recorder::create();
		{
			pfc::vartoggle_t< event_logger::ptr > toggle( m_logger, rec );
			f();
		}
		return rec;
	}
	
	void set_logger( event_logger::ptr logger ) {
		if ( logger.is_valid() ) {
			m_haveCustomLogger = true;
			m_logger = logger;
		} else {
			m_haveCustomLogger = false;
			m_logger = new service_impl_t<event_logger_fallback>();
		}
	}
protected:
	event_logger::ptr m_logger;
	bool m_haveCustomLogger = false;
};

#define FB2K_INPUT_LOG_STATUS(X) FB2K_LOG_STATUS(m_logger, X)
#define FB2K_INPUT_LOG_WARNING(X) FB2K_LOG_WARNING(m_logger, X)
#define FB2K_INPUT_LOG_ERROR(X) FB2K_LOG_ERROR(m_logger, X)
