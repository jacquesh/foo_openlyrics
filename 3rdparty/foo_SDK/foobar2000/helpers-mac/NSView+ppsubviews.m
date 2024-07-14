#import "NSView+ppsubviews.h"

@implementation NSView (ppsubviews)

- (NSView*) recurFindSubViewOfClass: (Class) cls identifier: (NSString*) identifier {
    @autoreleasepool {
        NSMutableArray * arrAll = [NSMutableArray new];
        
        [arrAll addObjectsFromArray: self.subviews];
        for ( NSUInteger w = 0; w < arrAll.count; ++ w ) {
            NSView * thisView = [arrAll objectAtIndex: w];
            
            if ( (cls == nil || [thisView isKindOfClass: cls]) && ( identifier == nil || [thisView.identifier isEqualToString: identifier] ) ) {
                return thisView;
            }
            [arrAll addObjectsFromArray: thisView.subviews];
            
            
            if ( w >= 200 ) {
                [arrAll removeObjectsInRange: NSMakeRange(0, w) ];
                w = 0;
            }
        }
        return nil;
    }
}


- (NSView*) findSubViewOfClass: (Class) cls identifier: (NSString*) identifier {
    for (NSView * v in self.subviews) {
        if ( (cls == nil || [v isKindOfClass: cls]) && ( identifier == nil || [ v.identifier isEqualToString: identifier ] ) ) {
            return v;
        }
    }
    return nil;
}
- (NSView *)findSubViewOfClass:(Class)cls {
    return [self findSubViewOfClass: cls identifier: nil];
}
- (NSButton *)findButton {
    return (NSButton*) [self findSubViewOfClass: [NSButton class]];
}
- (NSTextView *)findTextView {
    return (NSTextView*) [self findSubViewOfClass: [NSTextView class]];
}
- (NSTextField *) findTextField {
    return (NSTextField*) [self findSubViewOfClass: [NSTextField class]];
}
- (NSImageView*) findImageView {
    return (NSImageView*) [self findSubViewOfClass: [NSImageView class]];
}
@end
