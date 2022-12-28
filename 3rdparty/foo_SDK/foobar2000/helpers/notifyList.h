#pragma once

#include <vector>
#include <set>
#include <map>
#include <memory>
#include <pfc/synchro.h>

template<typename obj_t> class notifyList : public std::vector<obj_t*> {
public:
	void operator+=( obj_t * obj ) { this->push_back(obj); }
	void operator-=( obj_t * obj )  {
		for(auto i = this->begin(); i != this->end(); ++i) {
			if (*i == obj) {this->erase(i); return;}
		}
		PFC_ASSERT(!"Should not get here");
	}
    bool contains(obj_t* obj) const {
        for (auto i : *this) {
            if (i == obj) return true;
        }
        return false;
    }
};



// Efficient notifylist. Efficient addremove, ordered operation.
template<typename obj_t> class notifyList2 {
public:
    typedef uint64_t key_t;
    typedef obj_t * val_t;

    void add( obj_t * obj ) {
        auto key = m_increment++;
        m_ordered[key] = obj;
        m_unordered[obj] = key;
    }
    void remove(obj_t * obj) {
        auto i1 = m_unordered.find(obj);
        if (i1 != m_unordered.end()) {
            m_ordered.erase(i1->second);
            m_unordered.erase(i1);
        }
    }
    void operator+=(obj_t * obj) {add(obj);}
    void operator-=(obj_t * obj) {remove(obj);}

	typedef std::vector<obj_t*> state_t;

	state_t get() const {
		state_t ret;
		ret.resize(m_ordered.size());
		size_t walk = 0;
		for( auto i = m_ordered.begin(); i != m_ordered.end(); ++i ) {
			ret[walk++] = i->second;
		}
		return std::move(ret);
	}
	size_t size() const { return m_unordered.size(); }
    bool contains( obj_t * obj ) const { return m_unordered.find( obj ) != m_unordered.end(); }
    
    key_t firstKey() const {
        auto iter = m_ordered.begin();
        if (iter == m_ordered.end()) return keyInvalid();
        return iter->first;
    }
    key_t nextKey(key_t key) const {
        auto iter = m_ordered.upper_bound( key );
        if (iter == m_ordered.end()) return keyInvalid();
        return iter->first;
    }
    obj_t * resolveKey( key_t key ) const {
        auto iter = m_ordered.find( key );
        if (iter != m_ordered.end()) return iter->second;
        return nullptr;
    }
    bool validKey(key_t key) const {
        return m_ordered.find( key ) != m_ordered.end();
    }
    static key_t keyInvalid() { return (key_t)(-1); }
private:

	key_t m_increment;

	std::map<key_t, val_t> m_ordered;
	std::map<val_t, key_t> m_unordered;
};

template<typename obj_t> class notifyListUnordered : public std::set<obj_t*> {
public:
    void operator+=( obj_t * obj ) {
        this->insert( obj );
    }
    void operator-=( obj_t * obj ) {
        this->erase( obj );
    }
};

//! Notify list v3 \n
//! Traits: \n
//! * Ordered \n
//! * Efficient add/remove \n
//! * Handles every possible scenario of add/remove in mid enumeration \n
//! * Use begin()/end() to walk with no further workarounds \n
//! * If you've done anything between ++ and *iter - or are using the notifylist in multiple threads, check *iter for giving you a null. Otherwise, after ++ you always have a valid value, or end. \n
//! Available in two flavours, non thread safe (notifyList3<>) and thread safe (notifyList3MT<>). \n
//! Multi threaded version ensures thread safety internally - though once it gives you a pointer, there's no safeguard against removal attempt while the pointer is being used, hence not adivsed.
template<typename obj_t, typename sync_t> class notifyList3_ {
private:
    typedef notifyList3_<obj_t, sync_t> self_t;
    typedef notifyList2< obj_t > content_t;
public:
    typedef typename content_t::key_t contentKey_t;
    struct data_t {
        content_t content;
        sync_t sync;
    };
    
    typedef std::shared_ptr< data_t > dataRef_t;

    notifyList3_( ) : m_data(std::make_shared< data_t >()) {}
    
    void operator+=( obj_t * obj ) { add(obj); }
    void operator-=( obj_t * obj ) { remove(obj); }

    void add( obj_t * obj ) {
        inWriteSync( m_data->sync );
        m_data->content.add( obj );
    }
    void remove( obj_t * obj ) {
        inWriteSync( m_data->sync );
        m_data->content.remove( obj );
    }
    size_t size() const {
        inReadSync( m_data->sync );
        return m_data->content.size();
    }
    
    class const_iterator {
    public:
        
        const_iterator( dataRef_t ref, contentKey_t key ) : m_data(ref), m_key(key) {
        }
        
        const_iterator const & operator++(int) {
            increment();
            return *this;
        }
        const_iterator operator++() {
            const_iterator ret ( *this );
            increment();
            return std::move(ret);
        }

        void increment() {
            PFC_ASSERT( isValid() );
            inReadSync( m_data->sync );
            m_key = m_data->content.nextKey( m_key );
        }
        
        bool isValid() const {
            inReadSync( m_data->sync );
            return m_data->content.validKey( m_key );
        }
        
        bool operator==( const_iterator const & other ) const {
            return equals( *this, other );
        }
        bool operator!=( const_iterator const & other ) const {
            return !equals( *this, other );
        }

        static bool equals( const_iterator const & i1, const_iterator const & i2 ) {
            return i1.m_key == i2.m_key;
        }
        
        //! Returns the referenced value. Will be null in case of invalidation in mid-enumeration.
        obj_t * operator*() const {
            inReadSync( m_data->sync );
            return m_data->content.resolveKey( m_key );
        }
        
    private:
        const_iterator() = delete;
        
        const dataRef_t m_data;
        contentKey_t m_key;
    };
    
    const_iterator begin() const {
        inReadSync( m_data->sync );
        return const_iterator( m_data, m_data->content.firstKey() );
    }
    const_iterator end() const {
        inReadSync( m_data->sync );
        return const_iterator( m_data, content_t::keyInvalid() );
    }
    
private:
    notifyList3_( const self_t & ) = delete;
    void operator=( const self_t & ) = delete;
    

    const dataRef_t m_data;
};

//! Thread safe notifyList3. \n
//! See: notifyList3_
template<typename obj_t>
class notifyList3MT : public notifyList3_<obj_t, pfc::readWriteLock> {
public:
    
};

//! Non-thread-safe notifyList3. \n
//! See: notifyList3_
template<typename obj_t>
class notifyList3 : public notifyList3_<obj_t, pfc::dummyLock> {
public:
    
};
