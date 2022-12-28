#include "foobar2000-sdk-pch.h"

#include <pfc/sort.h>
#include <pfc/bit_array_impl.h>
#include <pfc/string_conv.h>
#ifdef FOOBAR2000_MOBILE
#include "browseTree.h"
#endif
#include <mutex>

namespace {
	class arrayImpl : public fb2k::array {
	public:
		arrayImpl() {}
        arrayImpl(std::initializer_list<fb2k::objRef> const& arg) {
            m_content.set_size_discard(arg.size());
            size_t walk = 0;
            for (auto& obj : arg) m_content[walk++] = obj;
        }
		arrayImpl( array::ptr in ) {
			const size_t count = in->count();
			m_content.set_size_discard( count );
			for(size_t w = 0; w < count; ++w) m_content[w] = in->itemAt(w);
		}
		arrayImpl( array::ptr in, size_t const * order, size_t count ) {
			if (count != in->count()) uBugCheck();
			m_content.set_size_discard(count);
			for(size_t w = 0; w < count; ++w) m_content[w] = in->itemAt( order[w] );
		}
		arrayImpl( fb2k::objRef in ) {
			m_content.set_size_discard( 1 ) ;
			m_content[0] = in;
		}
        arrayImpl( fb2k::objRef const * in, size_t inCount) {
            m_content.set_data_fromptr( in, inCount );
        }
		size_t count() const { return m_content.get_size(); }
		service_ptr itemAt(size_t idx) const { return m_content[idx]; }

	private:
		pfc::array_staticsize_t< service_ptr > m_content;
	};
	class arrayMutableImpl : public fb2k::arrayMutable {
	public:
		arrayMutableImpl() {}
		arrayMutableImpl( array::ptr in ) {
			const size_t count = in->count();
			m_content.set_size_discard( count );
			for(size_t w = 0; w < count; ++w) {
				m_content[w] = in->itemAt( w );
			}
		}
		arrayMutableImpl( fb2k::objRef in ) {
			m_content.set_size_discard( 1 );
			m_content[0] = in;
		}
		size_t count() const { return m_content.get_size(); }
		fb2k::objRef itemAt( size_t idx ) const { return m_content[idx]; }
		void remove( pfc::bit_array const & mask ) {
			pfc::remove_mask_t( m_content, mask );
		}
		void insert( fb2k::objRef obj, size_t at ) {
			pfc::insert_t( m_content, obj, at );
		}
		void insertFrom( array::ptr objects, size_t at ) {
			if (objects.get_ptr() == this) {
				objects = arrayWithArray( objects );
			}
			pfc::insert_multi_t( m_content, *objects, objects->count(), at );
		}
		void reorder( const size_t * order, size_t count ) {
			if (count != m_content.get_size()) uBugCheck();
			pfc::reorder_t( m_content, order, count );
		}
		
		void resize( size_t newSize ) {
			m_content.set_size( newSize );
		}
		void setItem( fb2k::objRef obj, size_t atIndex ) {
			if (atIndex > m_content.get_size()) uBugCheck();
			m_content[atIndex] = obj;
		}
        void prealloc(size_t capacity) {
            m_content.prealloc(capacity);
        }
	private:
		pfc::array_t< fb2k::objRef, pfc::alloc_fast > m_content;
	};

	class stringImpl : public fb2k::string {
	public:
		stringImpl( const wchar_t * str, size_t len = pfc_infinite ) {
			size_t n = pfc::stringcvt::estimate_wide_to_utf8( str, len );
			m_str = (char*) malloc( n );
			pfc::stringcvt::convert_wide_to_utf8( m_str, n, str, len );
		}
		stringImpl( const char * str ) {
			m_str = pfc::strDup(str);
            if (m_str == nullptr) throw std::bad_alloc();
		}
        stringImpl( const char * str, size_t len ) {
            len = pfc::strlen_max( str, len );
            m_str = (char*)malloc( len + 1 );
            if (m_str == nullptr) throw std::bad_alloc();
            memcpy (m_str, str, len );
            m_str[len] = 0;
        }
        
		~stringImpl() {
			free(m_str);
		}
		const char * c_str() const {return m_str;}
	private:
		char * m_str;
	};

	

	class memBlockImpl : public fb2k::memBlock {
	public:
		memBlockImpl( const void * inData, size_t inDataSize ) {
			m_data.set_data_fromptr( (const uint8_t*) inData, inDataSize );
		}
		const void * data() const {
			return m_data.get_ptr();
		}
		size_t size() const {
			return m_data.get_size();
		}
	private:
		pfc::array_staticsize_t< uint8_t > m_data;
	};

	class memBlockImpl_takeOwnership : public fb2k::memBlock {
	public:
		memBlockImpl_takeOwnership(void * data, size_t size) : m_data(data), m_size(size) {}
		~memBlockImpl_takeOwnership() { free(m_data); }
		const void * data() const { return m_data;  }
		size_t size() const { return m_size;  }
	private:
		void * const m_data;
		const size_t m_size;
	};
    
    class memBlockMutableImpl : public fb2k::memBlockMutable {
    public:
        memBlockMutableImpl( size_t initSize ) { m_data.set_size_discard( initSize ); }
        memBlockMutableImpl( const void * inData, size_t inSize) {
            m_data.set_data_fromptr( (const uint8_t*) inData, inSize );
        }
        memBlockMutableImpl() {}
        
        const void * data() const { return m_data.get_ptr(); }
        void * dataMutable() { return m_data.get_ptr(); }
        
        size_t size() const { return m_data.get_size(); }
        void resize( size_t size ) { m_data.set_size( size ); }
        void resizeDiscard( size_t size ) { m_data.set_size_discard( size ); }
        
    private:
        pfc::array_t< uint8_t > m_data;
    };
}

namespace fb2k {
	FOOGUIDDECL const GUID array::class_guid = { 0x3f2c5273, 0x6cea, 0x427d, { 0x8c, 0x74, 0x34, 0x79, 0xc7, 0x22, 0x6a, 0x1 } };
	FOOGUIDDECL const GUID arrayMutable::class_guid = { 0x142709d1, 0x9cef, 0x42c4, { 0xb5, 0x63, 0xbc, 0x3a, 0xee, 0x8, 0xdc, 0xe3 } };
	FOOGUIDDECL const GUID string::class_guid = { 0xb3572de3, 0xdc77, 0x4494, { 0xa3, 0x90, 0xb1, 0xff, 0xd9, 0x17, 0x38, 0x77 } };
	// ! Old GUID of album_art_data !
	FOOGUIDDECL const GUID memBlock::class_guid = { 0x9ddce05c, 0xaa3f, 0x4565, { 0xb3, 0x3a, 0xbd, 0x6a, 0xdc, 0xdd, 0x90, 0x37 } };
    FOOGUIDDECL const GUID memBlockMutable::class_guid =  { 0x481ab64c, 0xa28, 0x435b, { 0x85, 0xdc, 0x9e, 0x20, 0xfe, 0x92, 0x15, 0x61 } };
    FOOGUIDDECL const GUID objReceiver::class_guid = { 0x1b60d9fa, 0xcb9d, 0x45a4, { 0x90, 0x4e, 0xa, 0xa8, 0xaa, 0xe7, 0x99, 0x7c } };

	array::ptr array::arrayReordered( size_t const * order, size_t count ) {
		return new service_impl_t< arrayImpl > ( this, order, count );
	}
    array::ptr array::copy() const {
        return arrayWithArray(  const_cast<array*>( this ) );
    }

	array::ptr array::empty() {
		return new service_impl_t< arrayImpl >();
	}

	array::ptr array::arrayWithArray( array::ptr source ) {
		return new service_impl_t< arrayImpl > ( source );
	}
	array::ptr array::arrayWithObject( objRef source ) {
		return new service_impl_t< arrayImpl > ( source );
	}
    array::ptr array::arrayWithObjects( objRef const * source, size_t count ) {
        return new service_impl_t<arrayImpl>( source, count );
    }
    array::ptr array::arrayWithObjects(std::initializer_list< objRef > const& arg) {
        return new service_impl_t<arrayImpl>(arg);
    }
    arrayMutable::ptr arrayMutable::arrayWithCapacity(size_t capacity) {
        auto ret = fb2k::service_new< arrayMutableImpl >();
        ret->prealloc(capacity);
        return ret;
    }
	arrayMutable::ptr arrayMutable::empty() {
		return new service_impl_t< arrayMutableImpl > ();
	}
	arrayMutable::ptr arrayMutable::arrayWithArray( array::ptr source ) {
		return new service_impl_t< arrayMutableImpl > ( source );
	}
	arrayMutable::ptr arrayMutable::arrayWithObject( objRef source ) {
		return new service_impl_t< arrayMutableImpl > ( source );
	}
    arrayMutable::ptr arrayMutable::copy() const {
        return arrayWithArray(  const_cast<arrayMutable*>(this) );
    }
    array::ptr array::makeConst() const {
        arrayMutableRef mut;
        if (mut &= const_cast<array*>(this)) return mut->copyConst();
        return const_cast<array*>(this);
    }
    array::ptr arrayMutable::copyConst() const {
        return array::arrayWithArray( const_cast<arrayMutable*>(this) );
    }

	string::ptr string::stringWithString( const char * str ) {
		return new service_impl_t< stringImpl > (str);
	}
    string::ptr string::stringWithString( const char * str, size_t len ) {
        return new service_impl_t< stringImpl > (str, len);
    }
	string::ptr string::stringWithString( string::ptr str ) {
		return new service_impl_t< stringImpl > (str->c_str() );
	}
	bool string::equalsNullSafe( string::ptr v1, string::ptr v2 ) {
		if (v1.is_empty() && v2.is_empty()) return true;
		if (v1.is_empty() || v2.is_empty()) return false;
		return v1->equals( v2 );
	}

	memBlock::ptr memBlock::empty() {
		return blockWithData(NULL, 0);
	}
	memBlock::ptr memBlock::blockWithDataTakeOwnership(void * inData, size_t inSize) {
		return new service_impl_t< memBlockImpl_takeOwnership >(inData, inSize);
	}
	memBlock::ptr memBlock::blockWithData( const void * inData, size_t inSize) {
		return new service_impl_t< memBlockImpl > ( inData, inSize );
	}
    memBlock::ptr memBlock::blockWithData(pfc::mem_block const& b) {
        return blockWithData(b.ptr(), b.size());
    }
    memBlock::ptr memBlock::blockWithData(pfc::mem_block&& b) {
        auto ret = blockWithDataTakeOwnership(b.ptr(), b.size());
        b.detach();
        return ret;
    }
    memBlock::ptr memBlock::copy() const {
        return blockWithBlock( const_cast<memBlock*>( this ) );
    }
    
    memBlockMutable::ptr memBlockMutable::copy() const {
        return blockWithBlock( const_cast<memBlockMutable*>( this ) );
    }
    memBlock::ptr memBlockMutable::copyConst() const {
        return memBlock::blockWithBlock( const_cast<memBlockMutable*>(this) );
    }
    
    memBlockMutableRef memBlockMutable::empty() {
        return new service_impl_t< memBlockMutableImpl >;
    }
    memBlockMutableRef memBlockMutable::blockWithSize(size_t initSize) {
        return new service_impl_t< memBlockMutableImpl >( initSize );
    }
    
    memBlockRef memBlock::blockWithBlock(memBlock::ptr block) {
        return blockWithData( block->data(), block->size() );
    }
    memBlockMutableRef memBlockMutable::blockWithData( const void * inData, size_t inSize ) {
        return new service_impl_t< memBlockMutableImpl >( inData, inSize );
    }
    
    memBlockMutableRef memBlockMutable::blockWithBlock(memBlock::ptr block) {
        return blockWithData( block->data(), block->size() );
    }
    
    void describe( objRef obj, pfc::string_formatter & output, unsigned indent) {
        output.add_chars( ' ', indent * 2 );
        {
            stringRef str;
            if (obj->cast(str)) {
                output << "string: " << str->c_str() << "\n";
                return;
            }
        }
        {
            arrayRef arr;
            if (obj->cast(arr)) {
                arrayMutableRef mut;
                if (obj->cast(mut)) {
                    output << "arrayMutable";
                } else {
                    output << "array";
                }
                const size_t count = arr->count();
                output << " (" << count << " items):\n";
                for(size_t w = 0; w < count; ++w) {
                    describe( arr->itemAt(w), output, indent + 1);
                }
                return;
            }
        }
        {
            memBlockRef block;
            if (obj->cast(block)) {
                memBlockMutableRef mut;
                if (obj->cast(mut)) {
                    output << "memBlockMutable";
                } else {
                    output << "memBlock";
                }
                output << " (" << block->size() << " bytes)\n";
                return;
            }
        }
#ifdef FOOBAR2000_MOBILE
        {
            browseTreeItem::ptr item;
            if (obj->cast(item)) {
                output << "browseTreeItem\n";
                return;
            }
        }
#endif
        
        output << "[unknown]\n";
    }
    
    void describeDebug( objRef obj ) {
        console::formatter temp;
        describe( obj, temp, 0 );
    }
    
    void arrayMutable::removeAt( size_t idx ) {
        remove(pfc::bit_array_one (idx) ) ;
    }

	stringRef makeString( const wchar_t * str ) {
		return new service_impl_t<stringImpl>(str);
	}

	stringRef makeString( const wchar_t * str, size_t len ) {
		return new service_impl_t<stringImpl>(str, len);
	}
}

namespace {
    using namespace fb2k;
    class objReceiverImpl : public objReceiver {
    public:
        objReceiverImpl( const objReceiverFunc_t & f ) : m_func(f) {}
        void receiveObj(objRef obj) {
            m_func(obj);
        }
        
        objReceiverFunc_t m_func;
    };
    
    class callOnReleaseImpl : public service_base {
    public:
        callOnReleaseImpl( std::function<void () > f_) : f(f_) {}
        std::function<void ()> f;
        
        ~callOnReleaseImpl () {
            try {
                f();
            } catch(...) {}
        }
    };

	class arrayDeferred : public array {
	public:
		arrayDeferred( std::function<arrayRef () > f ) : m_func(f) {}
		size_t count() const {
			init();
			return m_chain->count();
		}
		objRef itemAt( size_t index ) const {
			init();
			return m_chain->itemAt( index );
		}		
	private:
		mutable std::once_flag m_once;
		mutable std::function< arrayRef () > m_func;
		mutable arrayRef m_chain;
		void init() const {
			std::call_once( m_once, [this] {
				m_chain = m_func();
			} );
		}
	};
}

namespace fb2k {

    objReceiverRef makeObjReceiver( objReceiverFunc_t f ) {
        return new service_impl_t< objReceiverImpl > ( f );
    }
    
    objRef callOnRelease( std::function< void () > f) {
        return new service_impl_t< callOnReleaseImpl > (f);
    }
    objRef callOnReleaseInMainThread( std::function< void () > f) {
        return callOnRelease( [f] {
            fb2k::inMainThread2( f );
        });
    }
	arrayRef makeArrayDeferred( std::function< arrayRef () > f ) {
		return new service_impl_t<arrayDeferred> ( f );
	}
}

namespace fb2k {
    objRef array::firstItem() const {
        size_t n = this->count();
        if ( n == 0 ) return nullptr;
        return this->itemAt( 0 );
    }
    
    objRef array::lastItem() const {
        size_t n = this->count();
        if ( n == 0 ) return nullptr;
        return this->itemAt( n - 1 );
    }
    
    size_t array::indexOfItem(objRef item) {
        const size_t m = this->count();
        for( size_t n = 0; n < m; ++n ) {
            auto obj = this->itemAt( n );
            if ( obj == item ) return n;
        }
        return pfc_infinite;
    }
    
    array::ptr array::subset( pfc::bit_array const & mask ) const {
        auto out = arrayMutable::empty();
        mask.walk( this->size(), [=] ( size_t w ) {
            out->add( this->itemAt( w ) );
        } );
        return out->makeConst();
    }
    
    array::ptr array::subsetExcluding(const pfc::bit_array & mask) const {
        return subset(pfc::bit_array_not(mask) );
    }
    array::ptr array::subsetExcludingSingle( size_t index ) const {
        return subsetExcluding(pfc::bit_array_one(index ) );
    }
}
