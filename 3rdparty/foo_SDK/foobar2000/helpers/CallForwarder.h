#pragma once

namespace CF {
	template<typename obj_t, typename arg_t> class _inMainThread : public main_thread_callback {
	public:
		_inMainThread(obj_t const & obj, const arg_t & arg) : m_obj(obj), m_arg(arg) {}
		
		void callback_run() {
			if (m_obj.IsValid()) callInMainThread::callThis(&*m_obj, m_arg);
		}
	private:
		obj_t m_obj;
		arg_t m_arg;
	};

	template<typename TWhat> class CallForwarder {
	private:
		CallForwarder() = delete;
	protected:
		CallForwarder(TWhat * ptr) : m_ptr(pfc::rcnew_t<TWhat*>(ptr)) {}
		void Orphan() { 
			*m_ptr = NULL; 
		}
	public:
		bool IsValid() const { 
			PFC_ASSERT( m_ptr.is_valid() );
			return m_ptr.is_valid() && *m_ptr != NULL; 
		}
		bool IsEmpty() const { return !IsValid(); }

		TWhat * operator->() const {
			PFC_ASSERT( IsValid() );
			return *m_ptr;
		}

		TWhat & operator*() const {
			PFC_ASSERT( IsValid() );
			return **m_ptr;
		}

		template<typename arg_t>
		void callInMainThread(const arg_t & arg) {
			main_thread_callback_add( new service_impl_t<_inMainThread< CallForwarder<TWhat>, arg_t> >(*this, arg) );
		}
	private:
		const pfc::rcptr_t<TWhat*> m_ptr;
	};

	template<typename TWhat> class CallForwarderMaster : public CallForwarder<TWhat> {
	public:
		CallForwarderMaster(TWhat * ptr) : CallForwarder<TWhat>(ptr) {PFC_ASSERT(ptr!=NULL);}
		~CallForwarderMaster() { this->Orphan(); }
		using CallForwarder<TWhat>::Orphan;
	private:
		CallForwarderMaster() = delete;
		CallForwarderMaster( const CallForwarderMaster<TWhat> & ) = delete;
		void operator=( const CallForwarderMaster & ) = delete;
	};

}