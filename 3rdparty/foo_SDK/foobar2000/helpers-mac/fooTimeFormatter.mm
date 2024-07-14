//
//  fooTimeFormatter.m
//  foo_abx
//
//  Created by P on 06/09/2023.
//

#import "stdafx.h"
#import "fooTimeFormatter.h"

@implementation fooTimeFormatter

- (NSString *)stringForObjectValue:(id)obj {
    double v = 0;
    if ( [obj respondsToSelector: @selector(doubleValue)]) v = [obj doubleValue];
    else if ( [obj respondsToSelector: @selector(longValue)]) v = [obj longValue];
    else if ( [obj respondsToSelector: @selector(intValue)]) v = [obj intValue];

    unsigned digits = 3;
    if ( _digits ) digits = _digits.unsignedIntValue;
    return [NSString stringWithUTF8String: pfc::format_time_ex(v, digits)];
}

- (NSString *)editingStringForObjectValue:(id)obj {
    return [self stringForObjectValue: obj];
}
- (BOOL)getObjectValue:(out id  _Nullable __autoreleasing *)obj forString:(NSString *)string errorDescription:(out NSString * _Nullable __autoreleasing *)error {
    if ( string == nil ) return NO;
    try {
        double v = pfc::parse_timecode( string.UTF8String );
        *obj = [NSNumber numberWithDouble: v];
        return YES;
    } catch(...) {
        return NO;
    }
}
@end
