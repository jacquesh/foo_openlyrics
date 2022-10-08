#include "StdAfx.h"
#include "readers.h"
#include "fullFileBuffer.h"
#include "fileReadAhead.h"

#include <list>

t_size reader_membuffer_base::read(void * p_buffer, t_size p_bytes, abort_callback & p_abort) {
	p_abort.check_e();
	t_size max = get_buffer_size();
	if (max < m_offset) uBugCheck();
	max -= m_offset;
	t_size delta = p_bytes;
	if (delta > max) delta = max;
	memcpy(p_buffer, (char*)get_buffer() + m_offset, delta);
	m_offset += delta;
	return delta;
}

void reader_membuffer_base::seek(t_filesize position, abort_callback & p_abort) {
	p_abort.check_e();
	t_filesize max = get_buffer_size();
	if (position == filesize_invalid || position > max) throw exception_io_seek_out_of_range();
	m_offset = (t_size)position;
}




file::ptr fullFileBuffer::open(const char * path, abort_callback & abort, file::ptr hint, t_filesize sizeMax) {
	//mutexScope scope(hMutex, abort);

	file::ptr f;
	if (hint.is_valid()) f = hint;
	else filesystem::g_open_read(f, path, abort);

	if (sizeMax != filesize_invalid) {
		t_filesize fs = f->get_size(abort);
		if (fs > sizeMax) return f;
	}
	try {
		service_ptr_t<reader_bigmem_mirror> r = new service_impl_t<reader_bigmem_mirror>();
		r->init(f, abort);
		f = r;
	}
	catch (std::bad_alloc) {}
	return f;
}










#include <memory>
#include "rethrow.h"
#include <pfc/synchro.h>
#include <pfc/threads.h>

namespace {
	struct dynInfoEntry_t {
		file_info_impl m_info;
		t_filesize m_offset;
	};
	struct readAheadInstance_t {
		file::ptr m_file;
		size_t m_readAhead, m_wakeUpThreschold;

		pfc::array_t<uint8_t> m_buffer;
		size_t m_bufferBegin, m_bufferEnd;
		pfc::event m_canRead, m_canWrite;
		pfc::mutex m_guard;
		ThreadUtils::CRethrow m_error;
		t_filesize m_seekto;
		abort_callback_impl m_abort;
		bool m_remote;
        bool m_atEOF = false;

		bool m_haveDynamicInfo;
		std::list<dynInfoEntry_t> m_dynamicInfo;
	};
	typedef std::shared_ptr<readAheadInstance_t> readAheadInstanceRef;
	static const t_filesize seek_reopen = (filesize_invalid-1);
	class fileReadAhead : public file_readonly_t< service_multi_inherit<file_get_metadata, file_dynamicinfo_v2 > > {
		service_ptr m_metadata;
	public:
		readAheadInstanceRef m_instance;
		~fileReadAhead() {
			if ( m_instance ) {
				auto & i = *m_instance;
				pfc::mutexScope guard( i.m_guard );
				i.m_abort.set();
				i.m_canWrite.set_state(true);
			}
		}
		service_ptr get_metadata(abort_callback&) override { return m_metadata; }
		void initialize( file::ptr chain, size_t readAhead, abort_callback & aborter ) {
			m_metadata = chain->get_metadata_(aborter);
			m_stats = chain->get_stats( aborter );
			if (!chain->get_content_type(m_contentType)) m_contentType = "";
			m_canSeek = chain->can_seek();
			m_position = chain->get_position( aborter );


			auto i = std::make_shared<readAheadInstance_t>();;
			i->m_file = chain;
			i->m_remote = chain->is_remote();
			i->m_readAhead = readAhead;
            i->m_wakeUpThreschold = readAhead * 3 / 4;
			i->m_buffer.set_size_discard( readAhead * 2 );
			i->m_bufferBegin = 0; i->m_bufferEnd = 0;
			i->m_canWrite.set_state(true);
			i->m_seekto = filesize_invalid;
			m_instance = i;

			{
				file_dynamicinfo::ptr dyn;
				if (dyn &= chain) {
					m_haveStaticInfo = dyn->get_static_info(m_staticInfo);
					i->m_haveDynamicInfo = dyn->is_dynamic_info_enabled();
				}
			}

			fb2k::splitTask( [i] {
#ifdef PFC_SET_THREAD_DESCRIPTION
				PFC_SET_THREAD_DESCRIPTION("Fb2k Read-Ahead Thread");
#endif
				worker(*i);
			} );
		}
		t_size read(void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
			auto & i = * m_instance;
			size_t done = 0;
            bool initial = true;
			while( done < p_bytes ) {
                if ( !initial ) {
                    // Do not invoke waiting with common case read with lots of data in the buffer
                    pfc::event::g_twoEventWait( i.m_canRead.get_handle(), p_abort.get_abort_event(), -1);
                }
                p_abort.check();
				pfc::mutexScope guard ( i.m_guard );
				size_t got = i.m_bufferEnd - i.m_bufferBegin;
				if (got == 0) {
					i.m_error.rethrow();
                    if ( initial && ! i.m_atEOF ) {
                        initial = false; continue; // proceed to wait for more data
                    }
					break; // EOF
				}

				size_t delta = pfc::min_t<size_t>( p_bytes - done, got );

                const bool wakeUpBefore = got < i.m_wakeUpThreschold;
                
				auto bufptr = i.m_buffer.get_ptr();
				if ( p_buffer != nullptr ) memcpy( (uint8_t*) p_buffer + done, bufptr + i.m_bufferBegin, delta );
				done += delta;
				i.m_bufferBegin += delta;
				got -= delta;
				m_position += delta;

				if (!i.m_error.didFail() && !i.m_atEOF) {
					if ( got == 0 ) i.m_canRead.set_state( false );
                    const bool wakeUpNow = got < i.m_wakeUpThreschold;
                    // Only set the event when *crossing* the boundary
                    // we will get a lot of wakeUpNow when nearing EOF
                    if ( wakeUpNow && ! wakeUpBefore ) i.m_canWrite.set_state( true );
				}
                initial = false;
                if ( i.m_atEOF ) break; // go no further
			}
            // FB2K_console_formatter() << "ReadAhead read: " << p_bytes << " => " << done;
			return done;
		}
		t_filesize get_size(abort_callback & p_abort) {
			p_abort.check();
			return m_stats.m_size;
		}
		t_filesize get_position(abort_callback & p_abort) {
			p_abort.check();
			return m_position;
		}

		void seek(t_filesize p_position,abort_callback & p_abort) {
			p_abort.check();
			if (!m_canSeek) throw exception_io_object_not_seekable();
			if ( m_stats.m_size != filesize_invalid && p_position > m_stats.m_size ) throw exception_io_seek_out_of_range();

			auto posNow = get_position(p_abort);
            
			if ( p_position >= posNow && p_position < posNow + m_instance->m_readAhead ) {
                // FB2K_console_formatter() << "ReadAhead skip: " << posNow << " => " << p_position;
				auto toSkip = p_position - posNow;
				if ( toSkip > 0 ) read(nullptr, (size_t) toSkip, p_abort);
				return;
			}
            // FB2K_console_formatter() << "ReadAhead seek: " << posNow << " => " << p_position;

			seekInternal( p_position );
		}
		bool can_seek() {
			return m_canSeek;
		}
		bool get_content_type(pfc::string_base & p_out) {
			if (m_contentType.length() == 0) return false;
			p_out = m_contentType; return true;
		}
		t_filestats get_stats( abort_callback & p_abort ) {
			p_abort.check();
			return m_stats;
				
		}
		t_filetimestamp get_timestamp(abort_callback & p_abort) {
			p_abort.check();
			return m_stats.m_timestamp;
		}
		bool is_remote() {
			return m_instance->m_remote;
		}

		void reopen( abort_callback & p_abort ) {
			if ( get_position( p_abort ) == 0 ) return;
			seekInternal( seek_reopen );
		}

		bool get_static_info(class file_info & p_out) {
			if ( ! m_haveStaticInfo ) return false;
			mergeInfo(p_out, m_staticInfo);
			return true;
		}
		bool is_dynamic_info_enabled() {
			return m_instance->m_haveDynamicInfo;

		}
		static void mergeInfo( file_info & out, const file_info & in ) {
			out.copy_meta(in);
			out.overwrite_info(in);
		}
		bool get_dynamic_info_v2(class file_info & out, t_filesize & outOffset) {
			auto & i = * m_instance;
			if ( ! i.m_haveDynamicInfo ) return false;
			
			insync( i.m_guard );
			auto ptr = i.m_dynamicInfo.begin();
			for ( ;; ) {
				if ( ptr == i.m_dynamicInfo.end() ) break;
				if ( ptr->m_offset > m_position ) break;
				++ ptr;
			}

			if ( ptr == i.m_dynamicInfo.begin() ) return false;

			auto iter = ptr; --iter;
			mergeInfo(out, iter->m_info);
			outOffset = iter->m_offset;
			i.m_dynamicInfo.erase( i.m_dynamicInfo.begin(), ptr );

			return true;
		}
	private:
		void seekInternal( t_filesize p_position ) {
			auto & i = * m_instance;
			insync( i.m_guard );
			i.m_error.rethrow();
			i.m_bufferBegin = i.m_bufferEnd = 0;
			i.m_canWrite.set_state(true);
			i.m_seekto = p_position;
            i.m_atEOF = false;
			i.m_canRead.set_state(false);

			m_position = ( p_position == seek_reopen ) ? 0 : p_position;
		}
		static void worker( readAheadInstance_t & i ) {
			ThreadUtils::CRethrow err;
			err.exec( [&i] {
                bool atEOF = false;
				uint8_t* bufptr = i.m_buffer.get_ptr();
				const size_t readAtOnceLimit = i.m_remote ? 256 : 4*1024;
				for ( ;; ) {
					i.m_canWrite.wait_for(-1);
					size_t readHowMuch = 0, readOffset = 0;
					{
						pfc::mutexScope guard(i.m_guard);
						i.m_abort.check();
						if ( i.m_seekto != filesize_invalid ) {
							if ( i.m_seekto == seek_reopen ) {
								i.m_file->reopen( i.m_abort );
							} else {
								i.m_file->seek( i.m_seekto, i.m_abort );
							}

							i.m_seekto = filesize_invalid;
                            atEOF = false;
						}
						size_t got = i.m_bufferEnd - i.m_bufferBegin;

						if ( i.m_bufferBegin >= i.m_readAhead ) {
							memmove( bufptr, bufptr + i.m_bufferBegin, got );
							i.m_bufferBegin = 0;
							i.m_bufferEnd = got;
						}

						if ( got < i.m_readAhead ) {
							readHowMuch = i.m_readAhead - got;
							readOffset = i.m_bufferEnd;
						}
					}

					if ( readHowMuch > readAtOnceLimit ) {
						readHowMuch = readAtOnceLimit;
					}

					bool dynInfoGot = false;
					dynInfoEntry_t dynInfo;

					if ( readHowMuch > 0 ) {
						readHowMuch = i.m_file->read( bufptr + readOffset, readHowMuch, i.m_abort );

                        if ( readHowMuch == 0 ) atEOF = true;
                        
						if ( i.m_haveDynamicInfo ) {
							file_dynamicinfo::ptr dyn;
							if ( dyn &= i.m_file ) {
								file_dynamicinfo_v2::ptr dyn2;
								if ( dyn2 &= dyn ) {
									dynInfoGot = dyn2->get_dynamic_info_v2(dynInfo.m_info, dynInfo.m_offset);
								} else {
									dynInfoGot = dyn->get_dynamic_info( dynInfo.m_info );
									if (dynInfoGot) {
										dynInfo.m_offset = dyn->get_position( i.m_abort );
									}
								}
							}
						}
					}

					{
						pfc::mutexScope guard( i.m_guard );
						i.m_abort.check();
						if ( i.m_seekto != filesize_invalid ) {
							// Seek request happened while we were reading - discard and continue
							continue; 
						}
                        i.m_atEOF = atEOF;
						i.m_canRead.set_state( true );
						i.m_bufferEnd += readHowMuch;
						size_t got = i.m_bufferEnd - i.m_bufferBegin;
                        if ( atEOF || got >= i.m_readAhead ) i.m_canWrite.set_state(false);

						if ( dynInfoGot ) {
							i.m_dynamicInfo.push_back( std::move(dynInfo) );
						}

					}
				}
			} );

			if ( err.didFail( ) ) {
				pfc::mutexScope guard( i.m_guard );
				i.m_error = err;
				i.m_canRead.set_state(true);
			}
		}

		bool m_canSeek;
		t_filestats m_stats;
		pfc::string8 m_contentType;
		t_filesize m_position;


		bool m_haveStaticInfo;
		file_info_impl m_staticInfo;
	};

}


file::ptr fileCreateReadAhead(file::ptr chain, size_t readAheadBytes, abort_callback & aborter ) {
	auto obj = fb2k::service_new<fileReadAhead>(); 
	obj->initialize( chain, readAheadBytes, aborter );
	
	// Two paths to cast to file*, pick one explicitly to avoid compiler error
	file_get_metadata::ptr temp = std::move(obj);
	return std::move(temp);
}
