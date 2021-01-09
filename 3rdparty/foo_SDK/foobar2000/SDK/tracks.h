#pragma once

// Special header with fb2k mobile metadb<=>trackList interop wrappers 

typedef metadb_handle_ptr trackRef;
typedef metadb_handle_list_cref trackListRef;
typedef metadb_handle_list trackListStore;
typedef metadb_info_container trackInfoContainer;

inline size_t trackCount(trackListRef l) {return l.get_size();}
inline trackRef trackListGetTrack(trackListRef l, size_t n) { return l[n]; }

// Returns blank info if no info is known, never null.
inline trackInfoContainer::ptr trackGetInfoRef(trackRef t) { return t->get_info_ref(); }
inline trackInfoContainer::ptr trackGetInfoRef(trackListRef l, size_t n) { return trackGetInfoRef(l[n]); }

// Returns null info if no info is known, contrary to trackGetInfoRef
inline trackInfoContainer::ptr trackGetInfo(trackRef t) { trackInfoContainer::ptr ret; if(!t->get_info_ref(ret)) ret = nullptr; return ret; }
inline trackInfoContainer::ptr trackGetInfo(trackListRef l, size_t n) { return trackGetInfo(l[n]); }

// Returns const char* or pfc::string8 depending on which fb2k!
inline const char * trackGetPath(trackListRef l, size_t n) { return l[n]->get_path(); }
inline const char * trackGetPath(trackRef t) { return t->get_path(); }

// Returns const playable_location& or playable_location_impl depending on which fb2k!
inline playable_location const & trackGetLocation(trackListRef l, size_t n) { return l[n]->get_location(); }
inline playable_location const & trackGetLocation(trackRef t) { return t->get_location(); }

inline double trackGetLength(trackListRef l, size_t n) { return l[n]->get_length(); }
inline double trackGetLength(trackRef t) { return t->get_length(); }

inline void trackFormatTitle(trackRef trk, titleformat_hook * hook, pfc::string_base & out, service_ptr_t<titleformat_object > script, titleformat_text_filter * filter) {
	trk->format_title(hook, out, script, filter);
}