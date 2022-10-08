#pragma once

class _critical_section_base {
protected:
	CRITICAL_SECTION sec;
public:
	_critical_section_base() {}
	inline void enter() throw() {EnterCriticalSection(&sec);}
	inline void leave() throw() {LeaveCriticalSection(&sec);}
	inline void create() throw() {
#ifdef PFC_WINDOWS_DESKTOP_APP
		InitializeCriticalSection(&sec);
#else
		InitializeCriticalSectionEx(&sec,0,0);
#endif
	}
	inline void destroy() throw() {DeleteCriticalSection(&sec);}
	inline bool tryEnter() throw() { return !!TryEnterCriticalSection(&sec); }
private:
	_critical_section_base(const _critical_section_base&) = delete;
	void operator=(const _critical_section_base&) = delete;
};

// Static-lifetime critical section, no cleanup - valid until process termination
class critical_section_static : public _critical_section_base {
public:
	critical_section_static() {create();}
#if !PFC_LEAK_STATIC_OBJECTS
	~critical_section_static() {destroy();}
#endif
};

// Regular critical section, intended for any lifetime scopes
class critical_section : public _critical_section_base {
public:
	critical_section() {create();}
	~critical_section() {destroy();}
};

namespace pfc {

// Read write lock - Vista-and-newer friendly lock that allows concurrent reads from a resource that permits such
// Warning, non-recursion proof
class readWriteLock {
public:
	readWriteLock() : theLock() {
	}
	
	void enterRead() {
		AcquireSRWLockShared( & theLock );
	}
	void enterWrite() {
		AcquireSRWLockExclusive( & theLock );
	}
	void leaveRead() {
		ReleaseSRWLockShared( & theLock );
	}
	void leaveWrite() {
		ReleaseSRWLockExclusive( &theLock );
	}

private:
	readWriteLock(const readWriteLock&) = delete;
	void operator=(const readWriteLock&) = delete;

	SRWLOCK theLock;
};

typedef ::_critical_section_base mutexBase_t;
typedef ::critical_section mutex;

}
