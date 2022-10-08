#pragma once

#include "avltree.h"

namespace pfc {

	//warning: not multi-thread-safe
	template<typename t_base>
	class instanceTracker : public t_base {
	private:
		typedef instanceTracker<t_base> t_self;
	public:
        template<typename ... args_t> instanceTracker( args_t && ... args) : t_base(std::forward<args_t>(args) ...) {g_list += this; }

		instanceTracker(const t_self & p_other) : t_base( (const t_base &)p_other) {g_list += this;}
		~instanceTracker() {g_list -= this;}

		typedef pfc::avltree_t<t_self*> t_list;
		static const t_list & instanceList() {return g_list;}
		template<typename t_callback> static void forEach(t_callback && p_callback) {instanceList().enumerate(p_callback);}
	private:
		static t_list g_list;
	};

	template<typename t_base>
	typename instanceTracker<t_base>::t_list instanceTracker<t_base>::g_list;


	//warning: not multi-thread-safe
	template<typename TClass>
	class instanceTrackerV2 {
	private:
		typedef instanceTrackerV2<TClass> t_self;
	public:
		instanceTrackerV2(const t_self & p_other) {g_list += static_cast<TClass*>(this);}
		instanceTrackerV2() {g_list += static_cast<TClass*>(this);}
		~instanceTrackerV2() {g_list -= static_cast<TClass*>(this);}

		typedef pfc::avltree_t<TClass*> t_instanceList;
		static const t_instanceList & instanceList() {return g_list;}
		template<typename t_callback> static void forEach(t_callback && p_callback) {instanceList().enumerate(p_callback);}
	private:
		static t_instanceList g_list;
	};

	template<typename TClass>
	typename instanceTrackerV2<TClass>::t_instanceList instanceTrackerV2<TClass>::g_list;


	struct objDestructNotifyData {
		bool m_flag;
		objDestructNotifyData * m_next;

	};
	class objDestructNotify {
	public:
		objDestructNotify() : m_data() {}
		~objDestructNotify() {
			set();
		}
		
		void set() {
			objDestructNotifyData * w = m_data;
			while(w) {
				w->m_flag = true; w = w->m_next;
			}
		}
		objDestructNotifyData * m_data;
	};

	class objDestructNotifyScope : private objDestructNotifyData {
	public:
		objDestructNotifyScope(objDestructNotify &obj) : m_obj(&obj) {
			m_next = m_obj->m_data;
			m_obj->m_data = this;
		}
		~objDestructNotifyScope() {
			if (!m_flag) m_obj->m_data = m_next;
		}
		bool get() const {return m_flag;}
		PFC_CLASS_NOT_COPYABLE_EX(objDestructNotifyScope)
	private:
		objDestructNotify * m_obj;

	};
}
