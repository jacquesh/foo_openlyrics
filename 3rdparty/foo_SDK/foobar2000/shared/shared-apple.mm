#include "shared.h"
#include "shared-apple.h"

#import <Cocoa/Cocoa.h>

bool uSetClipboardString(const char * str) {
    @autoreleasepool {
        @try {
            NSPasteboard * pb = [NSPasteboard generalPasteboard];
            [pb clearContents];
            [pb setString: [NSString stringWithUTF8String: str] forType:NSPasteboardTypeString];
            return true;
        } @catch (NSException *) {
            
        }
    }
    return false;
}

bool uGetClipboardString(pfc::string_base & out) {
    bool rv = false;
    @autoreleasepool {
        NSPasteboard * pb = [NSPasteboard generalPasteboard];
        NSString * str = [pb stringForType: NSPasteboardTypeString];
        if ( str != nil ) {
            out = str.UTF8String;
            rv = true;
        }
    }
    return rv;
}
