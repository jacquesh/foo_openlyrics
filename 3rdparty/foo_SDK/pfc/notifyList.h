#pragma once

#include <functional>
#include <map>
#include <memory>

namespace pfc {
	class notifyList {
	public:

		typedef size_t token_t;

		typedef std::function<void () > notify_t;
		token_t add( notify_t f ) {
			auto token = ++ m_increment;
			m_notify[token] = f;
			return token;
		}

		void remove( token_t t ) {
			m_notify.erase(t);
		}

		void dispatch() {
			// Safeguard against someone altering our state in mid-dispatch
			auto temp = m_notify;
			for( auto walk = temp.begin(); walk != temp.end(); ++ walk ) {
				if ( m_notify.count( walk->first ) > 0 ) { // still there?
					walk->second();
				}
			}
		}


		static std::shared_ptr<notifyList> make() {
			return std::make_shared<notifyList>();
		}
	private:
		token_t m_increment = 0;
		
		std::map<token_t, notify_t> m_notify;
	};

	typedef std::shared_ptr< notifyList > notifyListRef_t;

	class notifyEntry {
	public:
		notifyEntry() {}

		notifyEntry & operator<<( notifyList & l ) {
			PFC_ASSERT( m_list == nullptr );
			m_list = &l; 
			return *this;
		}
		notifyEntry & operator<<( notifyListRef_t l ) {
			PFC_ASSERT( m_list == nullptr );
			m_listShared = l;
			m_list = &*l;
			return *this;
		}
		notifyEntry & operator<<( notifyList::notify_t f ) {
			PFC_ASSERT( m_list != nullptr );
			PFC_ASSERT( m_token == 0 );
			m_token = m_list->add( f ); 
			return *this;
		}
		void clear() {
			if ( m_list != nullptr && m_token != 0 ) {
				m_list->remove(m_token);
				m_token = 0;
			}
		}
		~notifyEntry() {
			clear();
		}

	private:
		notifyListRef_t m_listShared;
		notifyList * m_list = nullptr;
		notifyList::token_t m_token = 0;

		notifyEntry( const notifyEntry & ) = delete;
		void operator=( const notifyEntry & ) = delete;
	};
}