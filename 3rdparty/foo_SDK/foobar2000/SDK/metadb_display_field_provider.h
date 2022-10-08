#pragma once

class titleformat_text_out;
class titleformat_hook_function_params;


/*!
	Implementing this service lets you provide your own title-formatting fields that are parsed globally with each call to metadb_handle::format_title methods. \n
	Note that this API is meant to allow you to add your own component-specific fields - not to overlay your data over standard fields or over fields provided by other components. Any attempts to interfere with standard fields will have severe ill effects. \n
	This should be implemented only where absolutely necessary, for safety and performance reasons. Any expensive operations inside the process_field() method may severely damage performance of affected title-formatting calls. \n
	You must NEVER make any other foobar2000 API calls from inside process_field, other than possibly querying information from the passed metadb_handle pointer; you should read your own implementation-specific private data and return as soon as possible. You must not make any assumptions about calling context (threading etc). \n
	It is guaranteed that process_field() is called only inside a metadb lock scope so you can safely call "locked" metadb_handle methods on the metadb_handle pointer you get. You must not lock metadb by yourself inside process_field() - while it is always called from inside a metadb lock scope, it may be called from another thread than the one maintaining the lock because of multi-CPU optimizations active. \n
	If there are multiple metadb_display_field_provider services registered providing fields of the same name, the behavior is undefined. You must pick unique names for provided fields to ensure safe coexistence with other people's components. \n
	IMPORTANT: Any components implementing metadb_display_field_provider MUST call metadb_io::dispatch_refresh() with affected metadb_handles whenever info that they present changes. Otherwise, anything rendering title-formatting strings that reference your data will not update properly, resulting in unreliable/broken output, repaint glitches, etc. \n
	Do not expect a process_field() call each time somebody uses title formatting, calling code might perform its own caching of strings that you return, getting new ones only after metadb_io::dispatch_refresh() with relevant items. \n
	If you can't reliably notify other components about changes of content of fields that you provide (such as when your fields provide some kind of global information and not information specific to item identified by passed metadb_handle), you should not be providing those fields in first place. You must not change returned values of your fields without dispatching appropriate notifications. \n
	Use static service_factory_single_t<myclass> to register your metadb_display_field_provider implementations. Do not call other people's metadb_display_field_provider services directly, they're meant to be called by backend only. \n
	List of fields that you provide is expected to be fixed at run-time. The backend will enumerate your fields only once and refer to them by indexes later. \n
*/

class NOVTABLE metadb_display_field_provider : public service_base {
public:
	//! Returns number of fields provided by this metadb_display_field_provider implementation.
	virtual t_uint32 get_field_count() = 0;
	//! Returns name of specified field provided by this metadb_display_field_provider implementation. Names are not case sensitive. It's strongly recommended that you keep your field names plain English / ASCII only.
	virtual void get_field_name(t_uint32 index, pfc::string_base& out) = 0;
	//! Evaluates the specified field.
	//! @param index Index of field being processed : 0 <= index < get_field_count().
	//! @param handle Handle to item being processed. You can safely call "locked" methods on this handle to retrieve track information and such.
	//! @param out Interface receiving your text output.
	//! @returns Return true to indicate that the field is present so if it's enclosed in square brackets, contents of those brackets should not be skipped, false otherwise.
	virtual bool process_field(t_uint32 index, metadb_handle* handle, titleformat_text_out* out) = 0;

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(metadb_display_field_provider);
};


//! \since 2.0
//! metadb_display_field_provider with caller-supplied metadb record to reduce the number of database queries.
class metadb_display_field_provider_v2 : public metadb_display_field_provider {
	FB2K_MAKE_SERVICE_INTERFACE(metadb_display_field_provider_v2, metadb_display_field_provider)
public:
	virtual bool process_field_v2(t_uint32 index, metadb_handle* handle, metadb_v2::rec_t const& rec, titleformat_text_out* out) = 0;
};
