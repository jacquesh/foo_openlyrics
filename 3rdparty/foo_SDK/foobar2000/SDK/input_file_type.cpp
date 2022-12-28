#include "foobar2000-sdk-pch.h"
#include "input_file_type.h"
#include "componentversion.h"
#include "archive.h"
#include "playlist_loader.h"

#if FOOBAR2000_TARGET_VERSION >= 76

typedef pfc::avltree_t<pfc::string8,pfc::io::path::comparator> t_fnList;

static void formatMaskList(pfc::string_base & out, t_fnList const & in) {
	auto walk = in.cfirst();
	if (walk.is_valid()) {
		out << *walk; ++walk;
		while(walk.is_valid()) {
			out << ";" << *walk; ++walk;
		}
	}
}
static void formatMaskList(pfc::string_base & out, t_fnList const & in, const char * label) {
	if (in.get_count() > 0) {
		out << label << "|";
		formatMaskList(out,in);
		out << "|";
	}
}

void input_file_type::make_filetype_support_fingerprint(pfc::string_base & str) {
	pfc::string_formatter out;
	pfc::avltree_t<pfc::string8, pfc::string::comparatorCaseInsensitive> names;
	
	{
		componentversion::ptr ptr; service_enum_t<componentversion> e;
		pfc::string_formatter name;
		while(e.next(ptr)) {
			name = "";
			ptr->get_component_name(name);
			if (strstr(name, "decoder") != NULL || strstr(name, "Decoder") != NULL) names += name;
		}
	}

	
	make_extension_support_fingerprint(out);
	for(auto walk = names.cfirst(); walk.is_valid(); ++walk) {
		if (!out.is_empty()) str << "|";
		out << *walk;
	}
	str = out;
}
void input_file_type::make_extension_support_fingerprint(pfc::string_base & str) {
	pfc::avltree_t<pfc::string8, pfc::string::comparatorCaseInsensitive> masks;
	{
		pfc::string_formatter mask;
		for( auto ptr : enumerate() ) {
			const unsigned count = ptr->get_count();
			for(unsigned n=0;n<count;n++) {
				mask.reset();
				if (ptr->get_mask(n,mask)) {
					if (strchr(mask,'|') == NULL) masks += mask;
				}
			}
		}
	}
	pfc::string_formatter out;
	for(auto walk = masks.cfirst(); walk.is_valid(); ++walk) {
		if (!out.is_empty()) out << "|";
		out << *walk;
	}
	str = out;
}

void input_file_type::build_openfile_mask(pfc::string_base & out, bool b_include_playlists, bool b_include_archives)
{	
	t_fnList extensionsAll, extensionsPl, extensionsArc;
	
	if (b_include_playlists) {
		for( auto ptr : playlist_loader::enumerate()) {
			if (ptr->is_associatable()) {
				pfc::string_formatter temp; temp << "*." << ptr->get_extension();
				extensionsPl += temp;
				extensionsAll += temp;
			}
		}
	}
	if (b_include_archives) {
		service_enum_t<filesystem> e;
		archive_v3::ptr p;
		pfc::string_formatter temp;
		while (e.next(p)) {
			p->list_extensions(temp);
			pfc::chain_list_v2_t<pfc::string8> lst;
			pfc::splitStringByChar(lst, temp, ',');
			for (auto iter = lst.first(); iter.is_valid(); ++iter) {
				extensionsArc += pfc::format( "*.", *iter );
			}
		}
	}

	typedef pfc::map_t<pfc::string8,t_fnList,pfc::string::comparatorCaseInsensitive> t_masks;
	t_masks masks;
	{
		pfc::string_formatter name, mask;
		for( auto ptr : enumerate() ) {
			const unsigned count = ptr->get_count();
			for(unsigned n=0;n<count;n++) {
				name.reset();
				mask.reset();
				if (ptr->get_name(n,name) && ptr->get_mask(n,mask)) {
					if (!strchr(name,'|') && !strchr(mask,'|')) {
						masks.find_or_add(name) += mask;
						extensionsAll += mask;
					}
				}
			}
		}
	}
	pfc::string_formatter outBuf;
	outBuf << "All files|*.*|";
	formatMaskList(outBuf, extensionsAll, "All supported media types");
	formatMaskList(outBuf, extensionsPl, "Playlists");
	formatMaskList(outBuf, extensionsArc, "Archives");
	

	for(auto walk = masks.cfirst(); walk.is_valid(); ++walk) {
		formatMaskList(outBuf,walk->m_value,walk->m_key);			
	}
	out = outBuf;
}
#endif