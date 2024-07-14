#import "NSComboBox+fb2k.h"
#import "NSMenu+ppaddons.h"

@implementation NSComboBox (fb2k)
- (void) setupHistory: (cfg_dropdown_history&) var {
    [self removeAllItems];
    pfc::string8 temp; var.get_state( temp );
    NSString * str = [NSString stringWithUTF8String: temp.c_str()];
    if ( str.length == 0 ) return;
    NSArray * arr = [str componentsSeparatedByCharactersInSet: [NSCharacterSet newlineCharacterSet]];
    if ( arr.count == 0 ) return;
    for( NSString * str in arr ) {
        if ( str.length > 0 ) [self addItemWithObjectValue: str];
    }
    self.stringValue = arr.firstObject;
    
    [self.menu addSeparator];
}

- (void)addToHistory:(cfg_dropdown_history &)var {
    NSString * str = self.stringValue;
    if ( str.length > 0 ) var.add_item( str.UTF8String );
}
@end
