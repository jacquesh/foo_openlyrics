#pragma once
#include <pthread.h>

namespace pfc {
    class mutexAttr {
	public:
		mutexAttr() {pthread_mutexattr_init(&attr);}
		~mutexAttr() {pthread_mutexattr_destroy(&attr);}
		void setType(int type) {pthread_mutexattr_settype(&attr, type);}
		int getType() const {int rv = 0; pthread_mutexattr_gettype(&attr, &rv); return rv; }
		void setRecursive() {setType(PTHREAD_MUTEX_RECURSIVE);}
		bool isRecursive() {return getType() == PTHREAD_MUTEX_RECURSIVE;}
		void setProcessShared() {pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);}
		operator const pthread_mutexattr_t & () const {return attr;}
		operator pthread_mutexattr_t & () {return attr;}
		pthread_mutexattr_t attr;
	private:
		mutexAttr(const mutexAttr&) = delete; void operator=(const mutexAttr&) = delete;
	};
    
    class mutexBase {
    public:
        void lock() throw() {pthread_mutex_lock(&obj);}
        void unlock() throw() {pthread_mutex_unlock(&obj);}
        
        void enter() throw() {lock();}
        void leave() throw() {unlock();}
        bool tryEnter() throw() {return pthread_mutex_trylock(&obj) == 0; }

        void create( const pthread_mutexattr_t * attr );
        void create( const mutexAttr & );
        void createRecur();
        void destroy();
    protected:
        mutexBase() {}
        ~mutexBase() {}
    private:
        pthread_mutex_t obj;
        
        void operator=( const mutexBase & ) = delete;
        mutexBase( const mutexBase & ) = delete;
    };
    


    class mutex : public mutexBase {
    public:
        mutex() {create(NULL);}
        ~mutex() {destroy();}
    };
    
    class mutexRecur : public mutexBase {
    public:
        mutexRecur() {createRecur();}
        ~mutexRecur() {destroy();}
    };
    
    
    class mutexRecurStatic : public mutexBase {
    public:
        mutexRecurStatic() {createRecur();}
    };
    
    
    
    typedef mutexBase mutexBase_t;
    
    
    class readWriteLockAttr {
    public:
        readWriteLockAttr() {pthread_rwlockattr_init( & attr ); }
        ~readWriteLockAttr() {pthread_rwlockattr_destroy( & attr ) ;}
        
        void setProcessShared( bool val = true ) {
            pthread_rwlockattr_setpshared( &attr, val ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE );
        }
        
        pthread_rwlockattr_t attr;
        
    private:
        readWriteLockAttr(const readWriteLockAttr&) = delete;
        void operator=(const readWriteLockAttr & ) = delete;
    };

    class readWriteLockBase {
    public:
        void create( const pthread_rwlockattr_t * attr );
        void create( const readWriteLockAttr & );
        void destroy() {pthread_rwlock_destroy( & obj ); }
        
        void enterRead() {pthread_rwlock_rdlock( &obj ); }
        void enterWrite() {pthread_rwlock_wrlock( &obj ); }
        void leaveRead() {pthread_rwlock_unlock( &obj ); }
        void leaveWrite() {pthread_rwlock_unlock( &obj ); }
    protected:
        readWriteLockBase() {}
        ~readWriteLockBase() {}
    private:
        pthread_rwlock_t obj;
        
        void operator=( const readWriteLockBase & ) = delete;
        readWriteLockBase( const readWriteLockBase & ) = delete;
    };
    
    
    class readWriteLock : public readWriteLockBase {
    public:
        readWriteLock() {create(NULL);}
        ~readWriteLock() {destroy();}
    };



}



typedef pfc::mutexRecur critical_section;
typedef pfc::mutexRecurStatic critical_section_static;
