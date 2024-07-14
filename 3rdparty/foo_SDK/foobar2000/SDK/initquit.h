#pragma once
//! Basic callback startup/shutdown callback, on_init is called after the main window has been created, on_quit is called before the main window is destroyed. \n
//! To register: static initquit_factory_t<myclass> myclass_factory; \n
//! Note that you should be careful with calling other components during on_init/on_quit or \n
//! initializing services that are possibly used by other components by on_init/on_quit - \n
//! initialization order of components is undefined.
//! If some other service that you publish is not properly functional before you receive an on_init() call, \n
//! someone else might call this service before >your< on_init is invoked.
class NOVTABLE initquit : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(initquit); 
public:
	virtual void on_init() {}
	virtual void on_quit() {}
};

template<typename T>
class initquit_factory_t : public service_factory_single_t<T> {};


//! \since 1.1
namespace init_stages {
	enum {
		before_config_read = 10,
		after_config_read = 20,
		before_library_init = 30,
		// Since foobar2000 v2.0, after_library_init is fired OUT OF ORDER with the rest, after ASYNCHRONOUS library init has completed.
		after_library_init = 40,
		before_ui_init = 50,
		after_ui_init = 60,
	};
};

//! \since 1.1
class NOVTABLE init_stage_callback : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(init_stage_callback)
public:
	virtual void on_init_stage(t_uint32 stage) = 0;

	static void dispatch(t_uint32 stage) {FB2K_FOR_EACH_SERVICE(init_stage_callback, on_init_stage(stage));}
};

//! Helper for FB2K_RUN_ON_INIT_QUIT()
class initquit_lambda : public initquit {
public:
	initquit_lambda(std::function<void()> i, std::function<void()> q) : m_init(i), m_quit(q) {}
	void on_init() override { if (m_init) m_init(); }
	void on_quit() override { if (m_quit) m_quit(); }
private:
	const std::function<void()> m_init, m_quit;
};

//! Helper macros to skip implementing initquit.\n
//! Usage: \n
//! void myfunc() { ... } \n
//! FB2K_RUN_ON_INIT(myFunc);
#define FB2K_RUN_ON_INIT_QUIT(funcInit, funcQuit) FB2K_SERVICE_FACTORY_PARAMS(initquit_lambda, funcInit, funcQuit)
#define FB2K_RUN_ON_INIT(func) FB2K_RUN_ON_INIT_QUIT(func, nullptr)
#define FB2K_RUN_ON_QUIT(func) FB2K_RUN_ON_INIT_QUIT(nullptr, func)

//! Helper for FB2K_ON_INIT_STAGE
class init_stage_callback_lambda : public init_stage_callback {
public:
	init_stage_callback_lambda(std::function<void()> f, uint32_t stage) : m_func(f), m_stage(stage) {}

	void on_init_stage(t_uint32 stage) override {
		PFC_ASSERT(m_func != nullptr);
		if (stage == m_stage) m_func();
	}

	const std::function<void()> m_func;
	const uint32_t m_stage;
};

//! Helper macro to skip implementing init_stage_callback.\n
//! Usage: \n
//! void myfunc() {...} \n
//! FB2K_ON_INIT_STAGE(myfunc, init_stages::after_ui_init);
#define FB2K_ON_INIT_STAGE(func, stage) FB2K_SERVICE_FACTORY_PARAMS(init_stage_callback_lambda, func, stage)
