#pragma once

namespace pfc {
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

		void set_data_fromptr(const void* p, size_t size) {
			resize(size); memcpy(ptr(), p, size);
		}
		void append_fromptr(const void* p, size_t size) {
			const size_t base = this->size();
			resize(base + size);
			memcpy((uint8_t*)ptr() + base, p, size);
		}

		void* detach() {
			void* ret = m_ptr; _clear(); return ret;
		}
	private:
		void _clear() noexcept { m_ptr = nullptr; m_size = 0; }
		void * m_ptr;
		size_t m_size;
	};
}

