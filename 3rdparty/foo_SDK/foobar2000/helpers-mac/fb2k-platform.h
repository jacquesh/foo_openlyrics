#pragma once

#import <SDK/foobar2000.h>
#import <Cocoa/Cocoa.h>

namespace fb2k {
    NSString * strToPlatform( const char * );
    NSString * strToPlatform( const char * , size_t );
    NSString * strToPlatform( stringRef );
    stringRef strFromPlatform( NSString * );
    
    stringRef urlFromPlatform( id url /* can be NSString or NSURL */ );

    typedef NSImage* platformImage_t;
    platformImage_t imageToPlatform( fb2k::objRef );
    

    // These two functions do the same, openWebBrowser() was added for compatiblity with fb2k mobile
    void openWebBrowser(const char * URL);
    void openURL( const char * URL);
}

namespace pfc {
    string8 strFromPlatform(NSString*);
    NSString * strToPlatform( const char * );
    NSString * strToPlatform(string8 const&);
    string8 strFromPlatform(CFStringRef);
}
