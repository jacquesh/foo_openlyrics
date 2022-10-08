#include "foobar2000.h"

GUID album_art_extractor::get_guid() {
	album_art_extractor_v2::ptr v2;
	if ( v2 &= this ) return v2->get_guid();
	return pfc::guid_null;
}

GUID album_art_editor::get_guid() {
	album_art_editor_v2::ptr v2;
	if ( v2 &= this ) return v2->get_guid();
	return pfc::guid_null;
}

bool album_art_extractor_instance::query(const GUID & what, album_art_data::ptr & out, abort_callback & abort) {
	try { out = query(what, abort); return true; } catch (exception_album_art_not_found) { return false; }
}

bool album_art_extractor_instance::have_entry(const GUID & what, abort_callback & abort) {
	try { query(what, abort); return true; } catch(exception_album_art_not_found) { return false; }
}

void album_art_editor_instance::remove_all_() {
	album_art_editor_instance_v2::ptr v2;
	if ( v2 &= this ) {
		v2->remove_all();
	} else {
		for( size_t walk = 0; walk < album_art_ids::num_types(); ++ walk ) {
			try {
				this->remove( album_art_ids::query_type( walk ) );
			} catch(exception_io_data) {}
			catch(exception_album_art_not_found) {}
		}
	}
}

bool album_art_editor::g_get_interface(service_ptr_t<album_art_editor> & out,const char * path) {
	service_enum_t<album_art_editor> e; ptr ptr;
	auto ext = pfc::string_extension(path);
	while(e.next(ptr)) {
		if (ptr->is_our_path(path,ext)) { out = ptr; return true; }
	}
	return false;
}

bool album_art_editor::g_is_supported_path(const char * path) {
	ptr ptr; return g_get_interface(ptr,path);
}

album_art_editor_instance_ptr album_art_editor::g_open(file_ptr p_filehint,const char * p_path,abort_callback & p_abort) {
	
#ifdef FOOBAR2000_DESKTOP
	{
		input_manager_v2::ptr m;
		if (fb2k::std_api_try_get(m)) {
			album_art_editor_instance::ptr ret;
			ret ^= m->open_v2(album_art_editor_instance::class_guid, p_filehint, p_path, false, nullptr, p_abort);
			return ret;
		}
	}
#endif

	album_art_editor::ptr obj;
	if (!g_get_interface(obj, p_path)) throw exception_album_art_unsupported_format();
	return obj->open(p_filehint, p_path, p_abort);
}


bool album_art_extractor::g_get_interface(service_ptr_t<album_art_extractor> & out,const char * path) {
	service_enum_t<album_art_extractor> e; ptr ptr;
	auto ext = pfc::string_extension(path);
	while(e.next(ptr)) {
		if (ptr->is_our_path(path,ext)) { out = ptr; return true; }
	}
	return false;
}

bool album_art_extractor::g_is_supported_path(const char * path) {
	ptr ptr; return g_get_interface(ptr,path);
}
album_art_extractor_instance_ptr album_art_extractor::g_open(file_ptr p_filehint,const char * p_path,abort_callback & p_abort) {
#ifdef FOOBAR2000_DESKTOP
	{
		input_manager_v2::ptr m;
		if (fb2k::std_api_try_get(m)) {
			album_art_extractor_instance::ptr ret;
			ret ^= m->open_v2(album_art_extractor_instance::class_guid, p_filehint, p_path, false, nullptr, p_abort);
			return ret;
		}
	}
#endif

	album_art_extractor::ptr obj;
	if (!g_get_interface(obj, p_path)) throw exception_album_art_unsupported_format();
	return obj->open(p_filehint, p_path, p_abort);
}


album_art_extractor_instance_ptr album_art_extractor::g_open_allowempty(file_ptr p_filehint,const char * p_path,abort_callback & p_abort) {
	try {
		return g_open(p_filehint, p_path, p_abort);
	} catch(exception_album_art_not_found) {
		return new service_impl_t<album_art_extractor_instance_simple>();
	}
}

namespace {
	class now_playing_album_art_notify_lambda : public now_playing_album_art_notify {
	public:
		void on_album_art(album_art_data::ptr data) {
			f(data);
		}
		std::function<void(album_art_data::ptr) > f;
	};
}

now_playing_album_art_notify * now_playing_album_art_notify_manager::add(std::function<void (album_art_data::ptr) > f ) {
	PFC_ASSERT ( f != nullptr );
	auto obj = new now_playing_album_art_notify_lambda;
	obj->f = f;
	add(obj);
	return obj;
}

namespace {
	struct aa_t {
		GUID type; const char * name;
	};
	static const GUID guids[] = {
		album_art_ids::cover_front,
		album_art_ids::cover_back,
		album_art_ids::artist,
		album_art_ids::disc,
		album_art_ids::icon,
	};
	static const char * const names[] = {
		"front cover",
		"back cover",
		"artist",
		"disc",
		"icon"
	};
	static const char * const names2[] = {
		"Front Cover",
		"Back Cover",
		"Artist",
		"Disc",
		"Icon"
	};
}

size_t album_art_ids::num_types() {
	PFC_STATIC_ASSERT( PFC_TABSIZE( guids ) == PFC_TABSIZE( names ) );
	PFC_STATIC_ASSERT( PFC_TABSIZE( guids ) == PFC_TABSIZE( names2 ) );
	return PFC_TABSIZE( guids );
}

GUID album_art_ids::query_type(size_t idx) {
	PFC_ASSERT( idx < PFC_TABSIZE( guids ) );
	return guids[idx];
}

const char * album_art_ids::query_name(size_t idx) {
	PFC_ASSERT( idx < PFC_TABSIZE( names ) );
	return names[idx];
}

const char * album_art_ids::name_of(const GUID & id) {
	for( size_t w = 0; w < num_types(); ++w ) {
		if ( query_type(w) == id ) return query_name(w);
	}
	return nullptr;
}

const char * album_art_ids::query_capitalized_name( size_t idx ) {
	PFC_ASSERT( idx < PFC_TABSIZE( names2 ) );
	return names2[idx];
}

const char * album_art_ids::capitalized_name_of( const GUID & id) {
	for( size_t w = 0; w < num_types(); ++w ) {
		if ( query_type(w) == id ) return query_capitalized_name(w);
	}
	return nullptr;
}

bool album_art_path_list::equals(album_art_path_list const& v1, album_art_path_list const& v2) {
	const size_t n = v1.get_count();
	if (n != v2.get_count()) return false;
	for (size_t w = 0; w < n; ++w) {
		if (metadb::path_compare(v1.get_path(w), v2.get_path(w)) != 0) return false;
	}
	return true;
}

bool album_art_path_list::equals(ptr const& v1, ptr const& v2) {
	if (v1.is_valid() != v2.is_valid()) return false;
	if (v1.is_empty() && v2.is_empty()) return true;
	return equals(*v1, *v2);
}