#import <Cocoa/Cocoa.h>

@interface NSView (ppsubviews)
- (NSView*) recurFindSubViewOfClass: (Class) cls identifier: (NSString*) identifier;
- (NSView*) findSubViewOfClass: (Class) cls identifier: (NSString*) identifier;
- (NSView*) findSubViewOfClass: (Class) cls;
- (NSButton*) findButton;
- (NSTextView*) findTextView;
- (NSTextField*) findTextField;
- (NSImageView*) findImageView;
@end
