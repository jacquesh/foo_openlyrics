#import "stdafx.h"
#import "fooSampleMacPreferences.h"

namespace foo_sample {
    extern cfg_uint cfg_bogoSetting1, cfg_bogoSetting2;
}

@interface fooSampleMacPreferences ()
@property (nonatomic) NSNumber* bogo1;
@property (nonatomic) NSNumber* bogo2;
@end

@implementation fooSampleMacPreferences

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.
}
- (instancetype)init {
    // IMPORTANT: feed OUR NSBundle, bundleForClass works well for this
    self = [self initWithNibName: @"fooSampleMacPreferences" bundle:[NSBundle bundleForClass: [self class]]];
    [self loadSettings];
    return self;
}
- (void) loadSettings {
    self.bogo1 = [NSNumber numberWithUnsignedLong: foo_sample::cfg_bogoSetting1];
    self.bogo2 = [NSNumber numberWithUnsignedLong: foo_sample::cfg_bogoSetting2];
}
- (IBAction)onBogo1:(id)sender {
    foo_sample::cfg_bogoSetting1 = self.bogo1.unsignedLongValue;
}
- (IBAction)onBogo2:(id)sender {
    foo_sample::cfg_bogoSetting2 = self.bogo2.unsignedLongValue;
}


@end





namespace {
class preferences_page_sample : public preferences_page {
    public:
        service_ptr instantiate() override {
            return fb2k::wrapNSObject( [ fooSampleMacPreferences new ] );
        }
        const char * get_name() override {return "Sample Component";}
        GUID get_guid() override {
            // This is our GUID. Replace with your own when reusing the code.
            return GUID { 0x7702c93e, 0x24dc, 0x48ed, { 0x8d, 0xb1, 0x3f, 0x27, 0xb3, 0x8c, 0x7c, 0xc9 } };
        }
        GUID get_parent_guid() override {return guid_tools;}
    };
    
    FB2K_SERVICE_FACTORY(preferences_page_sample);
}
