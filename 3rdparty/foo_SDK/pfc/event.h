namespace pfc {
#ifdef _WIN32
    
    typedef HANDLE eventHandle_t;

	static const eventHandle_t eventInvalid = NULL;
    
    class event : public win32_event {
    public:
        event() { create(true, false); }
        
        HANDLE get_handle() const {return win32_event::get();}
    };
#else
 
    typedef int eventHandle_t;

	static const eventHandle_t eventInvalid = -1;

    typedef nix_event event;
    
#endif
}
