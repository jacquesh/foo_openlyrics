#import "NSMenu+ppaddons.h"

@implementation NSMenu (ppaddons)

- (NSMenuItem*) addItemWithTitle: (NSString*) title action: (SEL) action target: (id) target {
    NSMenuItem * item = [[NSMenuItem alloc] initWithTitle: title action: action keyEquivalent: @""];
    item.target = target;
    [self addItem: item];
    return item;
}

- (void) addSeparator {
    [self addItem: [NSMenuItem separatorItem]];
}

- (NSMenuItem*) addSubMenu: (NSMenu*) menu withTitle: (NSString*) title {
    NSMenuItem * item = [[NSMenuItem alloc] init];
    item.title = title;
    item.submenu = menu;
    [self addItem: item];
    return item;
}

- (void)popUpForView:(NSView *)view {
    BOOL pullsDown = YES;
    NSMenu *popMenu = [self copy];
    NSRect frame = [view frame];
    frame.origin.x = 0.0;
    frame.origin.y = 0.0;
    
    if (pullsDown) [popMenu insertItemWithTitle:@"" action:NULL keyEquivalent:@"" atIndex:0];
    
    NSPopUpButtonCell *popUpButtonCell = [[NSPopUpButtonCell alloc] initTextCell:@"" pullsDown:pullsDown];
    [popUpButtonCell setMenu:popMenu];
    if (!pullsDown) [popUpButtonCell selectItem:nil];
    [popUpButtonCell performClickWithFrame:frame inView:view];
}

- (void)textToSeparators {
    [self textToSeparators: @"-"];
}

- (void)textToSeparators:(NSString *)text {
    for( size_t walk = 0; walk < self.numberOfItems; ++ walk ) {
        NSMenuItem * fix = [self itemAtIndex: walk];
        if ( [fix.title isEqualToString: text] ) {
            [self removeItemAtIndex: walk];
            [self insertItem: [NSMenuItem separatorItem] atIndex: walk];
        }
    }
}

@end

@implementation NSMenuItem (ppaddons)

- (void)setChecked:(BOOL)checked {
    self.state = checked ? NSOnState : NSOffState;
}
- (BOOL)checked {
    return self.state == NSOnState;
}
@end
