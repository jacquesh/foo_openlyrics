#pragma once
#include <mutex>
#include <atomic>

namespace fb2k {
	// cache of config values
	// STATIC USE ONLY
	class configBoolCache {
	public:
		configBoolCache(const char* var, bool def = false) : m_var(var), m_def(def) {}
		bool get();
		operator bool() { return get(); }
		void set(bool);
		void operator=(bool v) { set(v); }

		configBoolCache(const configBoolCache&) = delete;
		void operator=(const configBoolCache&) = delete;
	private:
		const char* const m_var;
		const bool m_def;
		std::atomic_bool m_value = { false };
		std::once_flag m_init;
	};

	class configIntCache {
	public:
		configIntCache(const char* var, int64_t def = 0) : m_var(var), m_def(def) {}
		int64_t get();
		operator int64_t() { return get(); }
		void set(int64_t);
		void operator=(int64_t v) { set(v); }

		configIntCache(const configIntCache&) = delete;
		void operator=(const configIntCache&) = delete;
	private:
		const char* const m_var;
		const int64_t m_def;
		std::atomic_int64_t m_value = { 0 };
		std::once_flag m_init;
	};
}