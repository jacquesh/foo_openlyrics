#pragma once

#ifdef __APPLE__

namespace fb2k {
    class NSObjectWrapper : public service_base {
        FB2K_MAKE_SERVICE_INTERFACE(NSObjectWrapper, service_base);
    public:
        virtual void * get_() = 0;
#ifdef __OBJC__
        id get() { return (__bridge id) get_(); }
#endif
        
    };
#ifdef __OBJC__
    service_ptr wrapNSObject(id);
    id unwrapNSObject(service_ptr);
#endif
}

#endif
