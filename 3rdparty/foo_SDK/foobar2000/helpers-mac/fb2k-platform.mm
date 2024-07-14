#include "fb2k-platform.h"

namespace fb2k {
    NSString * strToPlatform( const char * s, size_t len ) {
        pfc::string8 temp( s, len );
        return strToPlatform( temp );
    }

    NSString * strToPlatform( const char * s ) {
        return [NSString stringWithUTF8String: s];
    }
    NSString * strToPlatform( stringRef s ) {
        if ( s.is_empty( ) ) return nil;
        return strToPlatform( s->c_str() );
    }
    stringRef strFromPlatform( NSString * s ) {
        return makeString( s.UTF8String );
    }

    stringRef urlFromPlatform( id obj ) {
        if ( [obj isKindOfClass: [NSURL class] ] ) {
            NSURL * URL = obj;
            obj = URL.absoluteString;
            if ([obj hasPrefix:@"file://"])
                obj = URL.path;
        }
        if ( [obj respondsToSelector: @selector(UTF8String)]) {
            pfc::string8 temp;
            filesystem::g_get_canonical_path( [obj UTF8String], temp );
            return makeString( temp );
        }
        return nullptr;
    }

    void openURL( const char * URL ) {
        @autoreleasepool {
            [[NSWorkspace sharedWorkspace] openURL: [NSURL URLWithString: [NSString stringWithUTF8String: URL]]];
        }
    }
    void openWebBrowser( const char * URL) {
        openURL(URL);
    }

    NSImage * imageToPlatform( fb2k::objRef obj ) {
        fb2k::image::ptr img;
        if ( img &= obj ) {
            return (__bridge NSImage *) img->getNative();
        }
        return nil;
    }
}

namespace pfc {
    string8 strFromPlatform(NSString* str) {
        string8 ret;
        if ( str ) ret = str.UTF8String;
        return ret;
    }
    NSString * strToPlatform( const char * str) {
        return fb2k::strToPlatform( str );
    }
    NSString * strToPlatform(string8 const& str) {
        return fb2k::strToPlatform( str.c_str() );
    }
    string8 strFromPlatform(CFStringRef str) {
        return strFromPlatform( (__bridge NSString*) str );
    }
}
