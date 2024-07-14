#import <Cocoa/Cocoa.h>

@interface NSMenu (ppaddons)

- (NSMenuItem*) addItemWithTitle: (NSString*) title action: (SEL) action target: (id) target;
- (void) addSeparator;
- (NSMenuItem*) addSubMenu: (NSMenu*) menu withTitle: (NSString*) title;
- (void)popUpForView:(NSView *)view;

- (void) textToSeparators: (NSString*) text;
- (void) textToSeparators;
@end

@interface NSMenuItem (ppaddons)

@property BOOL checked;

@end
