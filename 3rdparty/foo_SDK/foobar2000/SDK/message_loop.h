#pragma once

#ifdef _WIN32
//! A message filter class. Allows you to filter incoming messages in main app loop. Use message_loop API to register/unregister, \n
//! or derive from message_filter_impl_base to have your class registered/deregistered automatically with lifetime.
//! //! Use with caution. Will not be executed during modal loops.
class NOVTABLE message_filter {
public:
	//! Notifies you about incoming message. \n
	//! You can alter the message (not generally recommended), let others handle it (return false) \n
	//! or signal that you've handled it (return true) to suppress further processing.
    virtual bool pretranslate_message(MSG * p_msg) = 0;
};
 
//! An idle handler class. Executed when main loop is about to enter idle state. \n
//! Use with caution. Will not be executed during modal loops.
class NOVTABLE idle_handler {
public:
    virtual bool on_idle() = 0;
};

//! Entrypoint API for registering message_filter and idle_handler objects. \n
//! Usage: message_loop::get()->add_message_filter(myfilter);
class NOVTABLE message_loop : public service_base
{
public:
    virtual void add_message_filter(message_filter * ptr) = 0;
    virtual void remove_message_filter(message_filter * ptr) = 0;
 
    virtual void add_idle_handler(idle_handler * ptr) = 0;
    virtual void remove_idle_handler(idle_handler * ptr) = 0;

	FB2K_MAKE_SERVICE_COREAPI(message_loop);
};

class NOVTABLE message_loop_v2 : public message_loop {
public:
	virtual void add_message_filter_ex(message_filter * ptr, t_uint32 lowest, t_uint32 highest) = 0;

	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(message_loop_v2, message_loop);
};

class message_filter_impl_base : public message_filter {
public:
	message_filter_impl_base();
	message_filter_impl_base(t_uint32 lowest, t_uint32 highest);
	~message_filter_impl_base();

	bool pretranslate_message(MSG * p_msg) override {return false;}
	
	PFC_CLASS_NOT_COPYABLE(message_filter_impl_base,message_filter_impl_base);
};

class message_filter_impl_accel : public message_filter_impl_base {
protected:
	bool pretranslate_message(MSG * p_msg) override;
public:
	message_filter_impl_accel(HINSTANCE p_instance,const TCHAR * p_accel);
	void set_wnd(HWND p_wnd) {m_wnd = p_wnd;}
private:
	HWND m_wnd = NULL;
	win32_accelerator m_accel;

	PFC_CLASS_NOT_COPYABLE(message_filter_impl_accel,message_filter_impl_accel);
};

class message_filter_remap_f1 : public message_filter_impl_base {
public:
	bool pretranslate_message(MSG * p_msg) override;
	void set_wnd(HWND wnd) {m_wnd = wnd;}
private:
	static bool IsOurMsg(const MSG * msg) {
		return msg->message == WM_KEYDOWN && msg->wParam == VK_F1;
	}
	HWND m_wnd = NULL;
};

class idle_handler_impl_base : public idle_handler {
public:
	idle_handler_impl_base() {message_loop::get()->add_idle_handler(this);}
	~idle_handler_impl_base() { message_loop::get()->remove_idle_handler(this);}
	bool on_idle() {return true;}

	PFC_CLASS_NOT_COPYABLE_EX(idle_handler_impl_base)
};
#endif // _WIN32
