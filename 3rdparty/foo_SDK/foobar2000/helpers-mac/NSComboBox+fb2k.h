#import <Cocoa/Cocoa.h>

#import <SDK/foobar2000.h>
#import <helpers/dropdown_helper.h>

@interface NSComboBox (fb2k)
- (void) setupHistory: (cfg_dropdown_history&) var;
- (void) addToHistory: (cfg_dropdown_history&) var;
@end
