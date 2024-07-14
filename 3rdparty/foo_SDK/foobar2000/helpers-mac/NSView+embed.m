#import "NSView+embed.h"

@implementation NSView (embed)

- (void)embedInView:(NSView *)superview {
    self.autoresizingMask = NSViewHeightSizable | NSViewWidthSizable;
    self.frame = superview.bounds;
    [superview addSubview: self];
}

- (void)embedInViewV2:(NSView *)superview {
    
    self.autoresizingMask = 0;
    [superview addSubview: self];
    NSMutableArray <NSLayoutConstraint * > * constraints = [NSMutableArray arrayWithCapacity: 4];
    static const NSLayoutAttribute params[4] = { NSLayoutAttributeLeft, NSLayoutAttributeRight, NSLayoutAttributeTop, NSLayoutAttributeBottom };
    for( unsigned i = 0; i < 4; ++ i) {
        NSLayoutAttribute a = params[i];
        [constraints addObject: [NSLayoutConstraint constraintWithItem: self attribute:a relatedBy:NSLayoutRelationEqual toItem:superview attribute:a multiplier:1 constant:0]];
    }
    
    [superview addConstraints: constraints];
}

@end
