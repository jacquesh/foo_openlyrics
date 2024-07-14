#pragma once
#include "library_manager.h"
#include "metadb_callbacks.h"
#include "callback_merit.h"

//! Callback service receiving notifications about Media Library content changes. Methods called only from main thread.\n
//! Use library_callback_factory_t template to register.
class NOVTABLE library_callback : public service_base {
public:
	//! Called when new items are added to the Media Library.
	virtual void on_items_added(metadb_handle_list_cref items) = 0;
	//! Called when some items have been removed from the Media Library.
	virtual void on_items_removed(metadb_handle_list_cref items) = 0;
	//! Called when some items in the Media Library have been modified. \n
	//! The list is sorted by pointer value for convenient matching by binary search.
	virtual void on_items_modified(metadb_handle_list_cref items) = 0;

	//! Is current on_items_modified() cycle called due to actual tags changed or dispaly hook operations? \n
	//! Supported since foobar2000 v2.0 beta 13
	static bool is_modified_from_hook();

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(library_callback);
};

//! \since 2.0
class NOVTABLE library_callback_v2 : public library_callback {
public:
	virtual void on_items_modified_v2(metadb_handle_list_cref items, metadb_io_callback_v2_data& data) = 0;
    virtual void on_library_initialized() = 0;

	FB2K_MAKE_SERVICE_INTERFACE(library_callback_v2, library_callback);
};

template<typename T>
class library_callback_factory_t : public service_factory_single_t<T> {};

class NOVTABLE library_callback_dynamic {
public:
	//! Called when new items are added to the Media Library.
	virtual void on_items_added(metadb_handle_list_cref items) = 0;
	//! Called when some items have been removed from the Media Library.
	virtual void on_items_removed(metadb_handle_list_cref items) = 0;
	//! Called when some items in the Media Library have been modified.
	//! The list is sorted by pointer value for convenient matching by binary search.
	virtual void on_items_modified(metadb_handle_list_cref items) = 0;

	void register_callback(); void unregister_callback();
};

//! Base class for library_callback_dynamic implementations, manages register/unregister calls for you
class library_callback_dynamic_impl_base : public library_callback_dynamic {
public:
	library_callback_dynamic_impl_base() { register_callback(); }
	~library_callback_dynamic_impl_base() { unregister_callback(); }

	//stub implementations - avoid pure virtual function call issues
	void on_items_added(metadb_handle_list_cref) override {}
	void on_items_removed(metadb_handle_list_cref) override {}
	void on_items_modified(metadb_handle_list_cref) override {}

	PFC_CLASS_NOT_COPYABLE_EX(library_callback_dynamic_impl_base);
};

//! \since 2.0
class NOVTABLE library_callback_v2_dynamic {
public:
	//! Called when new items are added to the Media Library.
	virtual void on_items_added(metadb_handle_list_cref items) = 0;
	//! Called when some items have been removed from the Media Library.
	virtual void on_items_removed(metadb_handle_list_cref items) = 0;
	//! Called when some items in the Media Library have been modified.
	//! The list is sorted by pointer value for convenient matching by binary search.
	virtual void on_items_modified_v2(metadb_handle_list_cref items, metadb_io_callback_v2_data & data) = 0;
    
    virtual void on_library_initialized() = 0;

	void register_callback(); void unregister_callback();
};

//! \since 2.0
//! //! Base class for library_callback_v2_dynamic implementations, manages register/unregister calls for you
class library_callback_v2_dynamic_impl_base : public library_callback_v2_dynamic {
public:
	library_callback_v2_dynamic_impl_base() { register_callback(); }
	~library_callback_v2_dynamic_impl_base() { unregister_callback(); }

	//stub implementations - avoid pure virtual function call issues
	void on_items_added(metadb_handle_list_cref) override {}
	void on_items_removed(metadb_handle_list_cref) override {}
	void on_items_modified_v2(metadb_handle_list_cref, metadb_io_callback_v2_data&) override {}
	void on_library_initialized() override {}

	PFC_CLASS_NOT_COPYABLE_EX(library_callback_v2_dynamic_impl_base);
};
