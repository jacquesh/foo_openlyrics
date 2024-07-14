#import "stdafx.h"
#import "fooSampleDSPView.h"
#import "dsp_sample.h"

@interface fooSampleDSPView ()
@property (nonatomic) dsp_preset_edit_callback_v2::ptr callback;
@property (nonatomic) NSNumber * gain;
@end

@implementation fooSampleDSPView

- (instancetype)init {
    // IMPORTANT: feed OUR NSBundle, bundleForClass works well for this
    return [self initWithNibName: @"fooSampleDSPView" bundle:[NSBundle bundleForClass: [self class]]];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.
    
    dsp_preset_impl preset;
    _callback->get_preset(preset);
    self.gain = [NSNumber numberWithFloat: parse_preset(preset)];
}

- (IBAction)onEdit:(id)sender {
    [self apply];
}

- (void) apply {
    dsp_preset_impl preset;
    make_preset( self.gain.floatValue , preset);
    _callback->set_preset( preset );
}
@end

service_ptr ConfigureSampleDSP( fb2k::hwnd_t parent, dsp_preset_edit_callback_v2::ptr callback ) {
    fooSampleDSPView * dialog = [fooSampleDSPView new];
    dialog.callback = callback;
    return fb2k::wrapNSObject( dialog );
}
