#pragma once

// ptrholder_t<> : std::unique_ptr<> equivalent
// Obsolete, use std::unique_ptr<> instead
// Warning: release() deletes instead of letting go of ownership, contrary to std::unique_ptr<>

namespace pfc {
	class releaser_delete {
	public:
		template<typename T> static void release(T* p_ptr) { delete p_ptr; }
	};
	class releaser_delete_array {
	public:
		template<typename T> static void release(T* p_ptr) { delete[] p_ptr; }
	};
	class releaser_free {
	public:
		static void release(void* p_ptr) { free(p_ptr); }
	};

	//! Assumes t_freefunc to never throw exceptions.
	template<typename T, typename t_releaser = releaser_delete >
	class ptrholder_t {
	private:
		typedef ptrholder_t<T, t_releaser> t_self;
	public:
		inline ptrholder_t(T* p_ptr) : m_ptr(p_ptr) {}
		inline ptrholder_t() : m_ptr(NULL) {}
		inline ~ptrholder_t() { t_releaser::release(m_ptr); }
		inline bool is_valid() const { return m_ptr != NULL; }
		inline bool is_empty() const { return m_ptr == NULL; }
		inline T* operator->() const { return m_ptr; }
		inline T* get_ptr() const { return m_ptr; }
		inline void release() { t_releaser::release(replace_null_t(m_ptr));; }
		inline void attach(T* p_ptr) { release(); m_ptr = p_ptr; }
		inline const t_self& operator=(T* p_ptr) { set(p_ptr); return *this; }
		inline T* detach() { return pfc::replace_null_t(m_ptr); }
		inline T& operator*() const { return *m_ptr; }

		inline t_self& operator<<(t_self& p_source) { attach(p_source.detach()); return *this; }
		inline t_self& operator>>(t_self& p_dest) { p_dest.attach(detach()); return *this; }

		//deprecated
		inline void set(T* p_ptr) { attach(p_ptr); }

		ptrholder_t(t_self&& other) {m_ptr = other.detach(); }
		const t_self& operator=(t_self&& other) { attach(other.detach()); return *this; }
	private:
		ptrholder_t(const t_self&) = delete;
		const t_self& operator=(const t_self&) = delete;

		T* m_ptr;
	};

	//avoid "void&" breakage
	template<typename t_releaser>
	class ptrholder_t<void, t_releaser> {
	private:
		typedef void T;
		typedef ptrholder_t<T, t_releaser> t_self;
	public:
		inline ptrholder_t(T* p_ptr) : m_ptr(p_ptr) {}
		inline ptrholder_t() : m_ptr(NULL) {}
		inline ~ptrholder_t() { t_releaser::release(m_ptr); }
		inline bool is_valid() const { return m_ptr != NULL; }
		inline bool is_empty() const { return m_ptr == NULL; }
		inline T* operator->() const { return m_ptr; }
		inline T* get_ptr() const { return m_ptr; }
		inline void release() { t_releaser::release(replace_null_t(m_ptr));; }
		inline void attach(T* p_ptr) { release(); m_ptr = p_ptr; }
		inline const t_self& operator=(T* p_ptr) { set(p_ptr); return *this; }
		inline T* detach() { return pfc::replace_null_t(m_ptr); }

		inline t_self& operator<<(t_self& p_source) { attach(p_source.detach()); return *this; }
		inline t_self& operator>>(t_self& p_dest) { p_dest.attach(detach()); return *this; }

		//deprecated
		inline void set(T* p_ptr) { attach(p_ptr); }
	private:
		ptrholder_t(const t_self&) = delete;
		const t_self& operator=(const t_self&) = delete;

		T* m_ptr;
	};

}