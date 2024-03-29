#pragma once

//! \since 0.9.5
//! Instance of a search filter object. \n
//! This object contains a preprocessed search query; used to perform filtering similar to Media Library Search or Album List's "filter" box. \n
//! Use search_filter_manager API to instantiate search_filter objects.
class search_filter : public service_base {
public:
protected:
	//! For backwards compatibility with older (0.9.5 alpha) revisions of this API. Do not call.
	virtual bool test_locked(const metadb_handle_ptr & p_item,const file_info * p_info) = 0;
public:

	//! Use this to run this filter on a group of items.
	//! @param data Items to test.
	//! @param out Pointer to a buffer (size at least equal to number of items in the source list) receiving the results.
	virtual void test_multi(metadb_handle_list_cref data, bool * out) = 0;
	
	FB2K_MAKE_SERVICE_INTERFACE(search_filter,service_base);
};

//! \since 0.9.5.3
class search_filter_v2 : public search_filter {
public:
	virtual bool get_sort_pattern(titleformat_object::ptr & out, int & direction) = 0;

	//! Abortable version of test_multi(). If the abort_callback object becomes signaled while the operation is being performed, contents of the output buffer are undefined and the operation will fail with exception_aborted.
	virtual void test_multi_ex(metadb_handle_list_cref data, bool * out, abort_callback & abort) = 0;

	//! Helper; removes non-matching items from the list.
	void test_multi_here(metadb_handle_list & ref, abort_callback & abort);

	FB2K_MAKE_SERVICE_INTERFACE(search_filter_v2, search_filter)
};

//! \since 0.9.5.4
class search_filter_v3 : public search_filter_v2 {
public:
	//! Returns whether the sort pattern returned by get_sort_pattern() was set by the user explicitly using "SORT BY" syntax or whether it was determined implicitly from some other part of the query. It's recommended to use this to determine whether to create a force-sorted autoplaylist or not.
	virtual bool is_sort_explicit() = 0;

	FB2K_MAKE_SERVICE_INTERFACE(search_filter_v3, search_filter_v2)
};

//! \since 2.0
class search_filter_v4 : public search_filter_v3 {
public:
	enum {
		flag_sort = 1 << 0,
	};
	virtual void test_v4(metadb_handle_list_cref items, metadb_io_callback_v2_data* dataIfAvail, bool* out, abort_callback& abort) = 0;
	
	virtual fb2k::arrayRef /* of metadb_handle */ search_v4(metadb_handle_list_cref lst, uint32_t flags, metadb_io_callback_v2_data * dataIfAvail, abort_callback& a) = 0;

	FB2K_MAKE_SERVICE_INTERFACE(search_filter_v4, search_filter_v3);
};

//! \since 0.9.5
//! Entrypoint class to instantiate search_filter objects.
class search_filter_manager : public service_base {
public:
	//! Creates a search_filter object. Throws an exception on failure (such as an error in the query). It's recommended that you relay the exception message to the user if this function fails.
	virtual search_filter::ptr create(const char * p_query) = 0;

	//! OBSOLETE, DO NOT CALL
	virtual void get_manual(pfc::string_base & p_out) = 0;

	FB2K_MAKE_SERVICE_COREAPI(search_filter_manager);
};

//! \since 0.9.5.3.
class search_filter_manager_v2 : public search_filter_manager {
public:
	enum {
		KFlagAllowSort = 1 << 0,
		KFlagSuppressNotify = 1 << 1,
	};
	//! Creates a search_filter object. Throws an exception on failure (such as an error in the query). It's recommended that you relay the exception message to the user if this function fails.
	//! @param changeNotify A completion_notify callback object that will get called each time the query's behavior changes as a result of some external event (such as system time change). The caller must refresh query results each time this callback is triggered. The status parameter of its on_completion() parameter is unused and always set to zero.
	virtual search_filter_v2::ptr create_ex(const char * query, completion_notify::ptr changeNotify, t_uint32 flags) = 0;

	//! Opens the search query syntax reference document, typically an external HTML in user's default web browser.
	virtual void show_manual() = 0;

	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(search_filter_manager_v2, search_filter_manager);
};

//! \since 2.0
class search_filter_manager_v3 : public search_filter_manager_v2 {
    FB2K_MAKE_SERVICE_COREAPI_EXTENSION(search_filter_manager_v3, search_filter_manager_v2);
public:
    enum combine_t {
        combine_and,
        combine_or
    };
	//! Combine multiple search filters into one, using the specified logical operation. \n
	//! This method INVALIDATES passed objects. Do not try to use them afterwards.
    virtual search_filter_v4::ptr combine(pfc::list_base_const_t<search_filter::ptr> const & arg, combine_t how, completion_notify::ptr changeNotify, t_uint32 flags) = 0;
};
