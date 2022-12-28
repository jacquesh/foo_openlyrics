#include "foobar2000-sdk-pch.h"
#include "completion_notify.h"
#include "main_thread_callback.h"

void completion_notify::g_signal_completion_async(completion_notify_ptr p_notify,unsigned p_code) {
	if (p_notify.is_valid()) {
        fb2k::inMainThread([p_notify, p_code] { p_notify->on_completion(p_code); });
	}
}

void completion_notify::on_completion_async(unsigned p_code) {
    g_signal_completion_async(this, p_code);
}


completion_notify::ptr completion_notify_receiver::create_or_get_task(unsigned p_id) {
    completion_notify_orphanable_ptr ptr;
    if (!m_tasks.query(p_id,ptr)) {
        ptr = completion_notify_create(this,p_id);
        m_tasks.set(p_id,ptr);
    }
    return ptr;
}

completion_notify_ptr completion_notify_receiver::create_task(unsigned p_id) {
    completion_notify_orphanable_ptr ptr;
    if (m_tasks.query(p_id,ptr)) ptr->orphan();
    ptr = completion_notify_create(this,p_id);
    m_tasks.set(p_id,ptr);
    return ptr;
}

bool completion_notify_receiver::have_task(unsigned p_id) const {
    return m_tasks.have_item(p_id);
}

void completion_notify_receiver::orphan_task(unsigned p_id) {
    completion_notify_orphanable_ptr ptr;
    if (m_tasks.query(p_id,ptr)) {
        ptr->orphan();
        m_tasks.remove(p_id);
    }
}

completion_notify_receiver::~completion_notify_receiver() {
    orphan_all_tasks();
}

void completion_notify_receiver::orphan_all_tasks() {
    m_tasks.enumerate(orphanfunc);
    m_tasks.remove_all();
}

namespace {
    using namespace fb2k;
    
    class completion_notify_func : public completion_notify {
    public:
        void on_completion(unsigned p_code) {
            m_func(p_code);
        }

        completionNotifyFunc_t m_func;
    };
}

namespace fb2k {
    
    completion_notify::ptr makeCompletionNotify( completionNotifyFunc_t func ) {
        service_ptr_t<completion_notify_func> n = new service_impl_t< completion_notify_func >;
        n->m_func = func;
        return n;
    }

}
