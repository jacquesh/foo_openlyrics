#import "NSButton+checked.h"

@implementation NSButton (checked)

- (BOOL)checked {
    return self.state == NSOnState;
}

- (void)setChecked:(BOOL)checked {
    self.state = checked ? NSOnState : NSOffState;
}

@end
