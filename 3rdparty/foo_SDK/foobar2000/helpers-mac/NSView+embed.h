#import <Cocoa/Cocoa.h>

@interface NSView (embed)
- (void) embedInView: (NSView*) superview;
- (void) embedInViewV2: (NSView*) superview; // adds autolayout constraints
@end
