#import "NSEvent+ppstuff.h"

@implementation NSEvent (ppstuff)

- (BOOL)isKeyDown {
    return self.type == NSEventTypeKeyDown;
}
- (BOOL)isCmdKeyDown {
    return self.isKeyDown && self.modifierFlagsDI == NSEventModifierFlagCommand;
}
- (BOOL)isShiftKeyDown {
    return self.isKeyDown && self.modifierFlagsDI == NSEventModifierFlagShift;
}
- (NSEventModifierFlags)modifierFlagsDI {
    return self.modifierFlags & NSEventModifierFlagDeviceIndependentFlagsMask;
}
@end
