#include "pfc-lite.h"
#include "pfc.h"
#include "string-conv-lite.h"
#include "SmartStrStr.h"

namespace {
    class foo {};
}

inline pfc::string_base & operator<<(pfc::string_base & p_fmt,foo p_source) {p_fmt.add_string_("FOO"); return p_fmt;}

namespace {
	using namespace pfc;
	class thread_selftest : public thread {
	public:
		void threadProc() {
			pfc::event ev;
			ev.wait_for(1);
			m_event.set_state(true);
			ev.wait_for(1);
		}
		pfc::event m_event;

		void selftest() {
			lores_timer timer; timer.start();
			this->start(arg_t());
			if (!m_event.wait_for(-1)) {
				PFC_ASSERT(!"Should not get here");
				return;
			}
			PFC_ASSERT(fabs(timer.query() - 1.0) < 0.1);
			this->waitTillDone();
			PFC_ASSERT(fabs(timer.query() - 2.0) < 0.1);
		}
	};
}

namespace pfc {

	static pfc::string8 testFormatter() {
		return PFC_string_formatter() << "foo" << 1 << 2 << 3;
	}
	

	// Self test routines that need to be executed to do their payload
	void selftest_runtime() {

		{
			string test = "foo123";
			auto wide = pfc::wideFromUTF8(test);
			auto narrow = pfc::utf8FromWide(wide);
			PFC_ASSERT(test == narrow);
		}
		{
			auto fmt = testFormatter();
			PFC_ASSERT(fmt == "foo123");
		}
		{
			auto fmt = pfc::format("foo", 1, 2, 3);
			PFC_ASSERT(fmt == "foo123");
		}


#if 0 // misfires occassionally, don't run it each time
		{
			thread_selftest t; t.selftest();
		}
#endif

		{
			stringLite s = "foo";
			PFC_ASSERT( (s + 2) == (s.c_str() + 2) );
		}

		{
			stringLite s = "foo";
			s.insert_chars(2, "00");
			PFC_ASSERT( strcmp(s, "fo00o" ) == 0 );
			s.remove_chars(2, 2);
			PFC_ASSERT( strcmp(s, "foo" ) == 0);
		}

		{
			pfc::map_t<pfc::string8, int> map;
			map["1"] = 1;
			map["2"] = 2;
			map["3"] = 3;
			PFC_ASSERT(map.get_count() == 3);
			PFC_ASSERT(map["1"] == 1);
			PFC_ASSERT(map["2"] == 2);
			PFC_ASSERT(map["3"] == 3);
		}

		{
			auto& obj = SmartStrStr::global();
			
			pfc::string8 str;
			str = obj.transformStr(u8"Bück Dich");
			PFC_ASSERT(str == "Buck Dich");
			str = obj.transformStr(u8"AUSLÄNDER");
			PFC_ASSERT(str == "AUSLANDER");
			str = obj.transformStr(u8"Weißes Fleisch");
			PFC_ASSERT(str == "Weisses Fleisch");
		}
	}
	// Self test routines that fail at compile time if there's something seriously wrong
	void selftest_static() {
		PFC_STATIC_ASSERT(sizeof(t_uint8) == 1);
		PFC_STATIC_ASSERT(sizeof(t_uint16) == 2);
		PFC_STATIC_ASSERT(sizeof(t_uint32) == 4);
		PFC_STATIC_ASSERT(sizeof(t_uint64) == 8);

		PFC_STATIC_ASSERT(sizeof(t_int8) == 1);
		PFC_STATIC_ASSERT(sizeof(t_int16) == 2);
		PFC_STATIC_ASSERT(sizeof(t_int32) == 4);
		PFC_STATIC_ASSERT(sizeof(t_int64) == 8);

		PFC_STATIC_ASSERT(sizeof(t_float32) == 4);
		PFC_STATIC_ASSERT(sizeof(t_float64) == 8);

		PFC_STATIC_ASSERT(sizeof(t_size) == sizeof(void*));
		PFC_STATIC_ASSERT(sizeof(t_ssize) == sizeof(void*));

		PFC_STATIC_ASSERT(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4);

		PFC_STATIC_ASSERT(sizeof(GUID) == 16);

		typedef pfc::avltree_t<int> t_asdf;
		t_asdf asdf; asdf.add_item(1);
		t_asdf::iterator iter = asdf._first_var();
		t_asdf::const_iterator iter2 = asdf._first_var();

		PFC_string_formatter() << "foo" << 1337 << foo();

		pfc::list_t<int> l; l.add_item(3);
	}

	void selftest() {
		selftest_static(); selftest_runtime();

		debugLog out; out << "PFC selftest OK";
	}
}
