#include "foobar2000.h"
#include <exception>

void packet_decoder::g_open(service_ptr_t<packet_decoder> & p_out,bool p_decode,const GUID & p_owner,t_size p_param1,const void * p_param2,t_size p_param2size,abort_callback & p_abort)
{
	std::exception_ptr rethrow;
	bool havePartial = false, tryingPartial = false;
	for ( ;; ) {
		service_enum_t<packet_decoder_entry> e;
		service_ptr_t<packet_decoder_entry> ptr;
		
		while (e.next(ptr)) {
			p_abort.check();
			if (ptr->is_our_setup(p_owner, p_param1, p_param2, p_param2size)) {
				if (!tryingPartial && ptr->is_supported_partially_(p_owner, p_param1, p_param2, p_param2size)) {
					havePartial = true;
				} else {
					try {
						ptr->open(p_out, p_decode, p_owner, p_param1, p_param2, p_param2size, p_abort);
						return;
					} catch (exception_io_data) {
						rethrow = std::current_exception();
					}
				}
			}
		}

		if (!havePartial || tryingPartial) break;
		tryingPartial = true;
	}

	if (rethrow) std::rethrow_exception(rethrow);
	throw exception_io_data();
}

size_t packet_decoder::initPadding() {
	size_t v = this->set_stream_property(property_bufferpadding, 0, NULL, 0);
	if (v > 0) {
		this->set_stream_property(property_bufferpadding, v, NULL, 0);
	}
	return v;
}

void packet_decoder::setEventLogger(event_logger::ptr logger) {
	this->set_stream_property(property_eventlogger, 0, logger.get_ptr(), 0);
}

void packet_decoder::setCheckingIntegrity(bool checkingIntegrity) {
	this->set_stream_property(property_checkingintegrity, checkingIntegrity ? 1 : 0, NULL, 0);
}

void packet_decoder::setAllowDelayed( bool bAllow ) {
    this->set_stream_property( property_allow_delayed_output, bAllow ? 1 : 0, NULL, 0);
}

bool packet_decoder_entry::is_supported_partially_(const GUID& p_owner, t_size p_param1, const void* p_param2, t_size p_param2size) {
	bool ret = false;
	packet_decoder_entry_v2::ptr v2;
	if (v2 &= this) {
		ret = v2->is_supported_partially(p_owner, p_param1, p_param2, p_param2size);
	}
	return ret;
}