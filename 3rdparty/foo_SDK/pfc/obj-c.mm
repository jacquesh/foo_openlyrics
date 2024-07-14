//
//  PFC-ObjC.m
//  pfc-test
//
//  Created by PEPE on 28/07/14.
//  Copyright (c) 2014 PEPE. All rights reserved.
//
#ifdef __APPLE__
#import <Foundation/Foundation.h>


#include <TargetConditionals.h>

#if TARGET_OS_MAC && !TARGET_OS_IPHONE
#import <Cocoa/Cocoa.h>
#endif

#include "pfc.h"


namespace pfc {
    void * thread::g_entry(void * arg) {
        @autoreleasepool {
            reinterpret_cast<thread*>(arg)->entry();
        }
        return NULL;
    }
    void thread::appleStartThreadPrologue() {
        if (![NSThread isMultiThreaded]) [[[NSThread alloc] init] start];
    }
    
    bool isShiftKeyPressed() {
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
        return ( [NSEvent modifierFlags] & NSEventModifierFlagShift ) != 0;
#else
        return false;
#endif
    }
    bool isCtrlKeyPressed() {
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
        return ( [NSEvent modifierFlags] & NSEventModifierFlagControl ) != 0;
#else
        return false;
#endif
    }
    bool isAltKeyPressed() {
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
        return ( [NSEvent modifierFlags] & NSEventModifierFlagOption ) != 0;
#else
        return false;
#endif
    }

	void inAutoReleasePool(std::function<void()> f) {
		@autoreleasepool {		
			f();
		}
	}
    
    void appleDebugLog( const char * str ) {
        NSLog(@"%s\n", str );
    }
    
    bool appleRecycleFile( const char * path ) {
        @autoreleasepool {
            NSFileManager * manager = [NSFileManager defaultManager];
            NSURL * url = [NSURL fileURLWithPath: [NSString stringWithUTF8String: path] ];
            if (@available(iOS 11.0, *)) {
                NSError * error = nil;
                if ([manager trashItemAtURL: url resultingItemURL: nil error: &error]) {
                    return true;
                }
                if ([error.domain isEqualToString: NSCocoaErrorDomain] && error.code == NSFeatureUnsupportedError) {
                    // trashcan not supported, fall thru
                } else {
                    // failed to remove
                    return false;
                }
            }
            return [manager removeItemAtURL: url error: nil];
        }
    }
    void appleSetThreadDescription( const char * str ) {
        @autoreleasepool {
            [NSThread currentThread].name = [NSString stringWithUTF8String: str];
        }
    }

    pfc::string8 unicodeNormalizeD(const char * str) {
        @autoreleasepool {
            pfc::string8 ret;
            NSString * v = [[NSString stringWithUTF8String: str] decomposedStringWithCanonicalMapping];
            if ( v ) ret = v.UTF8String;
            else ret = str;
            return ret;
        }
    }
    pfc::string8 unicodeNormalizeC(const char * str) {
        @autoreleasepool {
            pfc::string8 ret;
            NSString * v = [[NSString stringWithUTF8String: str] precomposedStringWithCanonicalMapping];
            if ( v ) ret = v.UTF8String;
            else ret = str;
            return ret;
        }
    }

    int appleNaturalSortCompare(const char* s1, const char* s2) {
        @autoreleasepool {
            NSString * str1 = [NSString stringWithUTF8String: s1];
            NSString * str2 = [NSString stringWithUTF8String: s2];
            return (int) [str1 localizedCompare: str2];
        }
    }
    int appleNaturalSortCompareI(const char* s1, const char* s2) {
        @autoreleasepool {
            NSString * str1 = [NSString stringWithUTF8String: s1];
            NSString * str2 = [NSString stringWithUTF8String: s2];
            return (int) [str1 localizedCaseInsensitiveCompare: str2];
        }
    }

}
#endif
