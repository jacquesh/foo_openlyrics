#import "fooDecibelFormatter.h"

@implementation fooDecibelFormatter

- (NSString*) formatNumber: (id) obj {
    
    double v = [ obj doubleValue ];
    
    if ( self.showSignAlways ) {
        if (v == 0) return [NSString stringWithUTF8String: "\xc2\xb1" "0"];
        if ( v > 0 ) {
            if (v == (double) (int) v) return [NSString stringWithFormat: @"+%i", (int)v];
            else return [NSString stringWithFormat: @"+%.02f", v];
        }
    }
    
    if (v == (double) (int) v) return [NSString stringWithFormat: @"%i", (int)v];
    else return [NSString stringWithFormat: @"%.02f", v];
}

- (NSString *)stringForObjectValue:(id)obj {
    return [NSString stringWithFormat: @"%@ dB", [self formatNumber: obj]];
}

- (NSAttributedString *)attributedStringForObjectValue:(id)obj withDefaultAttributes:(NSDictionary *)attrs {
    return nil;
}

- (NSString *)editingStringForObjectValue:(id)obj {
    return [self stringForObjectValue: obj];
    // return [self formatNumber: obj];
}

- (BOOL)getObjectValue:(out __autoreleasing id *)obj forString:(NSString *)string errorDescription:(out NSString *__autoreleasing *)error {
    if (error) *error = nil;

    {
        NSMutableCharacterSet * trimMe = [NSMutableCharacterSet whitespaceAndNewlineCharacterSet];
        [trimMe addCharactersInString: [NSString stringWithUTF8String: "+\xc2\xb1" ]];
        string = [ string.lowercaseString stringByTrimmingCharactersInSet: trimMe ];
    }
    
    if ( [string hasSuffix: @"db"]) {
        string = [ [string substringToIndex: string.length-2] stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet] ];
    }
    
    double v = string.doubleValue;
    if (self.minValue) {
        if (v < self.minValue.doubleValue) v = self.minValue.doubleValue;
    }
    if (self.maxValue) {
        if (v > self.maxValue.doubleValue) v = self.maxValue.doubleValue;
    }
    *obj = [NSNumber numberWithDouble: v];
    return YES;
}

@end
