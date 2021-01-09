#include "foobar2000.h"

static filesystem::ptr defaultFS() { 
	return filesystem::get( core_api::get_profile_path() );
}

void config_io_callback_v3::on_quicksave() {
	this->on_quicksave_v3(defaultFS());
}
void config_io_callback_v3::on_write(bool bReset) {
	auto fs = defaultFS();
	if (bReset) this->on_reset_v3(fs);
	else this->on_write_v3(fs);
}
