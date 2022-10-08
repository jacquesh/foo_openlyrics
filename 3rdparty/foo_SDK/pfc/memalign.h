#pragma once

namespace pfc {
    
    void alignedAlloc( void* & ptr, size_t & ptrSize, size_t newSize, size_t alignBytes);
    void * alignedAlloc( size_t size, size_t align );
    void alignedFree( void * ptr );
    
	template<unsigned alignBytes = 16>
	class mem_block_aligned {
	public:
		typedef mem_block_aligned<alignBytes> self_t;
		mem_block_aligned() : m_ptr(), m_size() {}

		void * ptr() {return m_ptr;}
		const void * ptr() const {return m_ptr;}
		void * get_ptr() {return m_ptr;}
		const void * get_ptr() const {return m_ptr;}
		size_t size() const {return m_size;}
		size_t get_size() const {return m_size;}

		void resize(size_t s) {
            alignedAlloc( m_ptr, m_size, s, alignBytes );
		}
		void set_size(size_t s) {resize(s);}
		
		~mem_block_aligned() {
            alignedFree(m_ptr);
        }

		self_t const & operator=(self_t const & other) {
			assign(other);
			return *this;
		}
		mem_block_aligned(self_t const & other) : m_ptr(), m_size() {
			assign(other);
		}
		void assign(self_t const & other) {
			resize(other.size());
			memcpy(ptr(), other.ptr(), size());
		}
		mem_block_aligned(self_t && other) {
			m_ptr = other.m_ptr;
			m_size = other.m_size;
			other.m_ptr = NULL; other.m_size = 0;
		}
		self_t const & operator=(self_t && other) {
			alignedFree(m_ptr);
			m_ptr = other.m_ptr;
			m_size = other.m_size;
			other.m_ptr = NULL; other.m_size = 0;
			return *this;
		}
        
	private:
		void * m_ptr;
		size_t m_size;
	};

	template<typename obj_t, unsigned alignBytes = 16>
	class mem_block_aligned_t {
	public:
		typedef mem_block_aligned_t<obj_t, alignBytes> self_t;
		void resize(size_t s) { m.resize( multiply_guarded(s, sizeof(obj_t) ) ); }
		void set_size(size_t s) {resize(s);}
		size_t size() const { return m.size() / sizeof(obj_t); }
		size_t get_size() const {return size();}
		obj_t * ptr() { return reinterpret_cast<obj_t*>(m.ptr()); }
		const obj_t * ptr() const { return reinterpret_cast<const obj_t*>(m.ptr()); }
		obj_t * get_ptr() { return reinterpret_cast<obj_t*>(m.ptr()); }
		const obj_t * get_ptr() const { return reinterpret_cast<const obj_t*>(m.ptr()); }
		mem_block_aligned_t() {}
	private:
		mem_block_aligned<alignBytes> m;
	};

	template<typename obj_t, unsigned alignBytes = 16>
	class mem_block_aligned_incremental_t {
	public:
		typedef mem_block_aligned_t<obj_t, alignBytes> self_t;
		
		void resize(size_t s) {
			if (s > m.size()) {
				m.resize( multiply_guarded<size_t>(s, 3) / 2 );				
			}
			m_size = s;
		}
		void set_size(size_t s) {resize(s);}
		
		size_t size() const { return m_size; }
		size_t get_size() const {return m_size; }

		obj_t * ptr() { return m.ptr(); }
		const obj_t * ptr() const { return m.ptr(); }
		obj_t * get_ptr() { return m.ptr(); }
		const obj_t * get_ptr() const { return m.ptr(); }
		mem_block_aligned_incremental_t() : m_size() {}
		mem_block_aligned_incremental_t(self_t const & other) : m(other.m), m_size(other.m_size) {}
		mem_block_aligned_incremental_t(self_t && other) : m(std::move(other.m)), m_size(other.m_size) { other.m_size = 0; }
		self_t const & operator=(self_t const & other) {m = other.m; m_size = other.m_size; return *this;}
		self_t const & operator=(self_t && other) {m = std::move(other.m); m_size = other.m_size; other.m_size = 0; return *this;}
	private:
		mem_block_aligned_t<obj_t, alignBytes> m;
		size_t m_size;
	};
}
