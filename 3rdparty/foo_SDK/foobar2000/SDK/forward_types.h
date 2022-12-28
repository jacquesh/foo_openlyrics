#pragma once

namespace foobar2000_io
{
	class abort_callback;
}

namespace fb2k {
	class image;
	typedef service_ptr_t<image> imageRef;

	class memBlock;
}

typedef fb2k::memBlock album_art_data;

typedef service_ptr_t<album_art_data> album_art_data_ptr;

class file_info;
class playable_location;

class titleformat_object;
class titleformat_hook;
class titleformat_text_filter;

typedef service_ptr_t<titleformat_object> titleformat_object_ptr;

class mem_block_container;

class file_info_filter;

class metadb_handle;
typedef service_ptr_t<metadb_handle> metadb_handle_ptr;

namespace foobar2000_io {
	class stream_reader; class stream_writer;
}

class metadb_io_callback_v2_data;
