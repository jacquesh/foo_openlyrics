#import "fooGenericDialog.h"

@interface fooGenericDialog ()

@end

@implementation fooGenericDialog {
    __weak IBOutlet NSStackView *m_stack;
}

- (instancetype)init {
    return [self initWithWindowNibName:@"fooGenericDialog"];
}
- (void)windowDidLoad {
    [super windowDidLoad];

    [m_stack setViews: @[_theContent.view] inGravity:NSStackViewGravityCenter];
}
- (IBAction)onDone:(id)sender {
    [self.window makeFirstResponder: self.window];
    [self endDialog: NSModalResponseOK];
}
- (void)cancelOperation:(id)sender {
    [self endDialog: NSModalResponseCancel];
}

- (void) endDialog: (NSModalResponse) response {
    [self.window.sheetParent endSheet: self.window returnCode: response];
}

@end
