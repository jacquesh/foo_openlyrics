#pragma once

#include <functional>
#include <initializer_list>
#include <pfc/list.h>
#include "completion_notify.h"

namespace fb2k {
	typedef service_ptr objRef;

    template<typename interface_t> class array_typed;
    template<typename interface_t> class array_soft_typed;

	class NOVTABLE array : public service_base, public pfc::list_base_const_t<service_ptr> {
		FB2K_MAKE_SERVICE_INTERFACE( array, service_base );
	public:
		virtual size_t count() const = 0;
		size_t size() const {return count();}
		virtual objRef itemAt( size_t index ) const = 0;
        
        objRef firstItem( ) const;
        objRef lastItem( ) const;

		objRef operator[] (size_t idx) const {return itemAt(idx);}

		array::ptr arrayReordered( size_t const * order, size_t count );

        array::ptr copy() const;
        //! If this is a mutable array, creates a const copy of it. \n
        //! Otherwise returns this object.
        array::ptr makeConst() const;
        
		static array::ptr empty();
		static array::ptr arrayWithArray( array::ptr source );
		static array::ptr arrayWithObject( objRef source );
        static array::ptr arrayWithObjects( objRef const * source, size_t count );
        static array::ptr arrayWithObjects(std::initializer_list< objRef > const&);
        
        size_t indexOfItem( objRef item );
        
        array::ptr subset( pfc::bit_array const & mask ) const;
        array::ptr subsetExcluding( pfc::bit_array const & mask ) const;
        array::ptr subsetExcludingSingle( size_t index ) const;

        // UNSAFE CAST HELPER
        // Use only when documented as allowed
        template<typename interface_t>
        const pfc::list_base_const_t< service_ptr_t< interface_t > >& as_list_of() const {
            const pfc::list_base_const_t<service_ptr> * temp = this;
            return *reinterpret_cast<const pfc::list_base_const_t< service_ptr_t< interface_t > > * > (temp);
        }

        // TYPED ARRAY HELPER
        // Usage: for( auto obj : myArray->typed<interface>() ) { ... }
        // Expects ALL objects in the array to implement the interface - causes runtime bugcheck if they don't
        template<typename interface_t>
        const array_typed<interface_t>& typed() const { return *reinterpret_cast< const array_typed<interface_t> * > (this); }

        // SOFT TYPED ARRAY HELPER
        // Usage: for( auto obj : myArray->softTyped<interface>() ) { ... }
        // Skips objects not implementing the interface.
        template<typename interface_t>
        const array_soft_typed<interface_t>& softTyped() const { return *reinterpret_cast<const array_soft_typed<interface_t> *> (this); }


        // pfc::list_base_const_t<>
        t_size get_count() const override {return this->count();}
        void get_item_ex(service_ptr& p_out, t_size n) const override { p_out = this->itemAt(n); }
	};

    template<typename interface_t> class array_typed : public array {
    public:
        typedef service_ptr_t<interface_t> ptr_t;
        class iterator {
        public:
            static iterator _make(const array* arr, size_t index) { iterator ret; ret.m_arr = arr; ret.m_index = index; return ret; }
            void operator++() { ++m_index; }
            bool operator==(const iterator& other) const { PFC_ASSERT(m_arr == other.m_arr); return m_index == other.m_index; }
            bool operator!=(const iterator& other) const { PFC_ASSERT(m_arr == other.m_arr); return m_index != other.m_index; }
            
            ptr_t operator*() const {
                ptr_t ret;
                ret ^= m_arr->itemAt(m_index);
                return ret;
            }
        private:
            const array* m_arr;
            size_t m_index;
        };
        typedef iterator const_iterator;
        iterator begin() const { return iterator::_make(this, 0); }
        iterator end() const { return iterator::_make(this, this->count()); }
    };

    template<typename interface_t> class array_soft_typed : public array {
    public:
        typedef service_ptr_t<interface_t> ptr_t;
        class iterator {
        public:
            static iterator _make(const array* arr, size_t index) { 
                iterator ret; 
                ret.m_arr = arr; ret.m_index = index; 
                ret.m_max = arr->count();
                ret.fetch(); 
                return ret; 
            }
            void operator++() { ++m_index; fetch(); }
            bool operator==(const iterator& other) const { PFC_ASSERT(m_arr == other.m_arr); return m_index == other.m_index; }
            bool operator!=(const iterator& other) const { PFC_ASSERT(m_arr == other.m_arr); return m_index != other.m_index; }

            ptr_t operator*() const {
                PFC_ASSERT(m_ptr.is_valid());
                return m_ptr;
            }
        private:
            void fetch() {
                m_ptr.reset();
                while (m_index < m_max) {
                    if (m_ptr &= m_arr->itemAt(m_index)) break;
                    ++m_index;
                }
            }
            const array* m_arr;
            size_t m_index, m_max;
            ptr_t m_ptr;
        };
        typedef iterator const_iterator;
        iterator begin() const { return iterator::_make(this, 0); }
        iterator end() const { return iterator::_make(this, this->count()); }
    };

	class NOVTABLE arrayMutable : public array {
		FB2K_MAKE_SERVICE_INTERFACE( arrayMutable, array );
	public:
		virtual void remove( pfc::bit_array const & mask ) = 0;
		virtual void insert( objRef obj, size_t at ) = 0;
		virtual void insertFrom( array::ptr objects, size_t at ) = 0;
		virtual void reorder( const size_t * order, size_t count ) = 0;
		
		virtual void resize( size_t newSize ) = 0;
		virtual void setItem( objRef obj, size_t atIndex ) = 0;

        arrayMutable::ptr copy() const;
        array::ptr copyConst() const;
        
        void removeAt( size_t idx );
        void add( objRef obj ) {insert( obj, (size_t)(-1) ) ; }
		void addFrom( array::ptr obj ) {insertFrom( obj, (size_t)(-1) ) ; }
		static arrayMutable::ptr empty();
		static arrayMutable::ptr arrayWithArray( array::ptr source );
		static arrayMutable::ptr arrayWithObject( objRef source );
        static arrayMutable::ptr arrayWithCapacity(size_t capacity);
	};

	class NOVTABLE string : public service_base {
		FB2K_MAKE_SERVICE_INTERFACE( string, service_base );
	public:
		virtual const char * c_str() const = 0;
        
        size_t length() const { return strlen(c_str()); }
        bool isEmpty() const { return *c_str() == 0; }
        bool equals( const char * other ) const { return strcmp(c_str(), other) == 0; }
        bool equals( string::ptr other ) const { return equals(other->c_str()); }
            
		static string::ptr stringWithString( const char * str );
		static string::ptr stringWithString( string::ptr str );
        static string::ptr stringWithString( const char * str, size_t len );
        static bool equalsNullSafe( string::ptr v1, string::ptr v2 );
	};

	class NOVTABLE memBlock : public service_base {
		FB2K_MAKE_SERVICE_INTERFACE( memBlock, service_base );
	public:
		virtual const void * data() const = 0;
		virtual size_t size() const = 0;

        const uint8_t * begin() const { return (const uint8_t*) data(); }
        const uint8_t * end() const { return begin() + size(); }
        
        //! Album_art_data compatibility
        size_t get_size() { return size(); }
        //! Album_art_data compatibility
        const void * get_ptr() { return data(); }
        
        memBlock::ptr copy() const;
        
		static memBlock::ptr empty();
		static memBlock::ptr blockWithData( const void * inData, size_t inSize);
        static memBlock::ptr blockWithBlock( memBlock::ptr block );
		template<typename vec_t>
		static memBlock::ptr blockWithVector(const vec_t & vec) {
			auto size = vec.size();
			if (size == 0) return empty();
			return blockWithData(&vec[0], size * sizeof(vec[0]));
		}
		
		//! Create an object that takes ownership of this memory, without copying it; will call free() when done.
		static memBlock::ptr blockWithDataTakeOwnership(void * inData, size_t inSize);
        static memBlock::ptr blockWithData(pfc::mem_block const &);
        static memBlock::ptr blockWithData(pfc::mem_block && );
        
        //! Determine whether two memBlock objects store the same content.
        static bool equals(memBlock const & v1, memBlock const & v2) {
            const t_size s = v1.size();
            if (s != v2.size()) return false;
            return memcmp(v1.data(), v2.data(),s) == 0;
        }
        static bool equals(ptr const& v1, ptr const& v2) {
            if (v1.is_valid() != v2.is_valid()) return false;
            if (v1.is_empty() && v2.is_empty()) return true;
            return equals(*v1, *v2);
        }

        bool operator==(const memBlock & other) const {return equals(*this,other);}
        bool operator!=(const memBlock & other) const {return !equals(*this,other);}
        
	};
    
    class NOVTABLE memBlockMutable : public memBlock {
        FB2K_MAKE_SERVICE_INTERFACE( memBlockMutable, memBlock );
    public:
        virtual void * dataMutable() = 0;
        
        //! Resizes preserving content. When expanding, contents of newly allocated area are undefined.
        virtual void resize( size_t size ) = 0;
        //! Resizes without preserving content. Contents undefined afterwards.
        virtual void resizeDiscard( size_t size ) { resize( size ); }
        
        memBlockMutable::ptr copy() const;
        memBlock::ptr copyConst() const;
        static memBlockMutable::ptr empty();
        static memBlockMutable::ptr blockWithSize( size_t initSize );
        static memBlockMutable::ptr blockWithData( const void * inData, size_t inSize );
        static memBlockMutable::ptr blockWithBlock( memBlock::ptr block );
        
        bool operator==(const memBlockMutable & other) const {return equals(*this,other);}
        bool operator!=(const memBlockMutable & other) const {return !equals(*this,other);}
    };
    
    
    //! Asynchronous object return helper. \n
    //! If the operation has failed for some reason, receiveObj() will be called with null object.
    class objReceiver : public service_base {
        FB2K_MAKE_SERVICE_INTERFACE( objReceiver, service_base );
    public:
        virtual void receiveObj( objRef obj ) = 0;
    };
    
    typedef objReceiver::ptr objReceiverRef;
    
    
    typedef ::completion_notify completionNotify;
    typedef completionNotify::ptr completionNotifyRef;
	typedef array::ptr arrayRef;
	typedef arrayMutable::ptr arrayMutableRef;
	typedef string::ptr stringRef;
	typedef memBlock::ptr memBlockRef;
    typedef memBlockMutable::ptr memBlockMutableRef;

	stringRef makeString( const wchar_t * str );
	stringRef makeString( const wchar_t * str, size_t len );

	inline stringRef makeString( const char * str ) {
		return string::stringWithString ( str );
	}
    inline stringRef makeString( const char * str, size_t len) {
        return string::stringWithString ( str, len );
    }
	inline arrayRef makeArray( std::initializer_list<objRef> const & arg ) {
		return array::arrayWithObjects( arg );
	}
    inline memBlockRef makeMemBlock( const void * data, size_t size ) {
        return memBlock::blockWithData( data, size );
    }
    
    void describe( objRef obj, pfc::string_formatter & output, unsigned indent);
    void describeDebug( objRef obj );
    
    
    typedef std::function< void (objRef) > objReceiverFunc_t;
    
    objReceiverRef makeObjReceiver( objReceiverFunc_t );

	template<typename class_t> objReceiverRef makeObjReceiverTyped(std::function< void(service_ptr_t<class_t>) > func) {
		return makeObjReceiver([=](objRef obj) {
			if (obj.is_empty()) func(nullptr);
			else {
				service_ptr_t<class_t> temp; temp ^= obj; func(temp);
			}
		});
	}
    
    objRef callOnRelease( std::function< void () > );
    objRef callOnReleaseInMainThread( std::function< void () > );

	arrayRef makeArrayDeferred( std::function< arrayRef () > f );
}
inline pfc::string_base & operator<<(pfc::string_base & p_fmt, fb2k::stringRef str) { p_fmt.add_string(str->c_str()); return p_fmt; }
