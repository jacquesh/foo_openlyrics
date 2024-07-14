#import <Cocoa/Cocoa.h>
#import <SDK/foobar2000.h>

typedef NSViewController fooPreferencesCommon;

template<typename obj_t>
class preferences_mac_common : public preferences_page_v4 {
public:
    service_ptr instantiate() override {
        return fb2k::wrapNSObject( [obj_t new] );
    }
};
