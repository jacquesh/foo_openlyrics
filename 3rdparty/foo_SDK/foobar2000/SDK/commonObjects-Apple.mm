#include "foobar2000-sdk-pch.h"
#include "commonObjects-Apple.h"
#include <Cocoa/Cocoa.h>

namespace {
    class NSObjectWrapperImpl : public fb2k::NSObjectWrapper {
    public:
        id obj;
        void * get_() override {
            return (__bridge void*) obj;
        }
    };
}
namespace fb2k {
    service_ptr wrapNSObject(id arg) {
        if (!arg) return nullptr;
        auto ret = fb2k::service_new<NSObjectWrapperImpl>();
        ret->obj = arg;
        return ret;
    }
    id unwrapNSObject(service_ptr arg) {
        id ret = nil;
        fb2k::NSObjectWrapper::ptr obj;
        if ( obj &= arg ) {
            ret = obj->get();
        }
        return ret;
    }
}
