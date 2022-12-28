#pragma once

namespace pfc {
	//! Manages a malloc()'d memory block. Most methods are self explanatory.
	class mem_block {
	public:
		mem_block( ) noexcept { _clear(); }
		~mem_block() noexcept { clear(); }
		void resize(size_t);
		void clear() noexcept;
		size_t size() const noexcept { return m_size;}
		void * ptr() noexcept { return m_ptr; }
		const void * ptr() const noexcept { return m_ptr; }
		void move( mem_block & other ) noexcept;
		void copy( mem_block const & other );
		mem_block(const mem_block & other) { _clear(); copy(other); }
		mem_block(mem_block && other) noexcept { _clear(); move(other); }
		mem_block const & operator=( const mem_block & other ) { copy(other); return *this; }
		mem_block const & operator=( mem_block && other ) noexcept { move(other); return *this; }

		void set(const void* ptr, size_t size) {set_data_fromptr(ptr, size); }
		void set_data_fromptr(const void* p, size_t size) { resize(size); memcpy(ptr(), p, size); 
		}
		void append_fromptr(const void* p, size_t size) {
			const size_t base = this->size();
			resize(base + size);
			memcpy((uint8_t*)ptr() + base, p, size);
		}

		void* detach() noexcept {
			void* ret = m_ptr; _clear(); return ret;
		}

		void* get_ptr() noexcept { return m_ptr; }
		const void* get_ptr() const noexcept { return m_ptr; }
		size_t get_size() const noexcept { return m_size; }

		//! Attaches an existing memory block, allocated with malloc(), to this object. After this call, the memory becomes managed by this mem_block instance.
		void attach(void* ptr, size_t size) noexcept {
			clear();
			m_ptr = ptr; m_size = size;
		}
		//! Turns existing memory block, allocated with malloc(), to a mem_block object. After this call, the memory becomes managed by this mem_block instance.
		static mem_block takeOwnership(void* ptr, size_t size) {
			mem_block ret(noinit{});
			ret.m_ptr = ptr; ret.m_size = size;
			return ret;
		}
	private:
		struct noinit {}; mem_block(noinit) {}
		void _clear() noexcept { m_ptr = nullptr; m_size = 0; }
		void * m_ptr;
		size_t m_size;
	};
}

