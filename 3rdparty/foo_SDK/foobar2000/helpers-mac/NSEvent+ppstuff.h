#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSEvent (ppstuff)

@property (readonly) BOOL isKeyDown;
@property (readonly) BOOL isCmdKeyDown;
@property (readonly) BOOL isShiftKeyDown;
@property (readonly) NSEventModifierFlags modifierFlagsDI;
@end

NS_ASSUME_NONNULL_END
