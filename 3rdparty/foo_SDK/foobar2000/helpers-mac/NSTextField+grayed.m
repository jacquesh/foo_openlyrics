#import "NSTextField+grayed.h"

@implementation NSTextField (grayed)

- (void)setGrayed:(BOOL)grayed {
    if (grayed) self.textColor = [NSColor disabledControlTextColor];
    else self.textColor = [NSColor labelColor];
}

- (BOOL)grayed {
    return [self.textColor isEqualTo: [NSColor disabledControlTextColor]];
}

@end
