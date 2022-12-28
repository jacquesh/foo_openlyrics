#include "foobar2000-sdk-pch.h"
#include "console_manager.h"
#include "console.h"
#include "metadb_handle.h"
#include "event_logger.h"

void console::info(const char * p_message) {print(p_message);}
void console::error(const char * p_message) {complain("Error", p_message);}
void console::warning(const char * p_message) {complain("Warning", p_message);}

void console::info_location(const playable_location & src) {print_location(src);}
void console::info_location(const metadb_handle_ptr & src) {print_location(src);}

void console::print_location(const metadb_handle_ptr & src)
{
	print_location(src->get_location());
}

void console::print_location(const playable_location & src)
{
    FB2K_console_formatter() << src;
}

void console::complain(const char * what, const char * msg) {
	FB2K_console_formatter() << what << ": " << msg;
}
void console::complain(const char * what, std::exception const & e) {
	complain(what, e.what());
}

void console::print(const char* p_message)
{
	if (core_api::are_services_available()) {
		for (auto p : console_receiver::enumerate()) {
			p->print(p_message, SIZE_MAX);
		}
	}
}

void console::printf(const char* p_format,...)
{
	va_list list;
	va_start(list,p_format);
	printfv(p_format,list);
	va_end(list);
}

void console::printfv(const char* p_format,va_list p_arglist)
{
	pfc::string8_fastalloc temp;
	uPrintfV(temp,p_format,p_arglist);
	print(temp);
}



namespace {

	class event_logger_recorder_impl : public event_logger_recorder {
	public:
		void playback( event_logger::ptr playTo ) {
			for(auto i = m_entries.first(); i.is_valid(); ++i ) {
				playTo->log_entry( i->line.get_ptr(), i->severity );
			}
		}

		void log_entry( const char * line, unsigned severity ) {
			auto rec = m_entries.insert_last();
			rec->line = line;
			rec->severity = severity;
		}
	private:

		struct entry_t {
			pfc::string_simple line;
			unsigned severity;
		};
		pfc::chain_list_v2_t< entry_t > m_entries;
	};

}

event_logger_recorder::ptr event_logger_recorder::create() {
	return new service_impl_t<event_logger_recorder_impl>();
}




namespace console {
	void addNotify(fb2k::console_notify* n) {
		static_api_ptr_t<fb2k::console_manager>()->addNotify(n);
	}
	void removeNotify(fb2k::console_notify* n) {
		static_api_ptr_t<fb2k::console_manager>()->removeNotify(n);
	}
	fb2k::arrayRef getLines() {
		return static_api_ptr_t<fb2k::console_manager>()->getLines();
	}
	void clearBacklog() {
		static_api_ptr_t<fb2k::console_manager>()->clearBacklog();
	}

	bool isVerbose() {
		return static_api_ptr_t<fb2k::console_manager>()->isVerbose();
	}
}
