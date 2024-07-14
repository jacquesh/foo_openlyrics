#import "fooWindowWithCancel.h"

@implementation fooWindowWithCancel
- (void) cancelOperation: (id) sender {
    id d = self.delegate;
    if ( [d respondsToSelector: @selector(cancelOperation:)]) {
        [d cancelOperation: sender];
    }
}
@end
