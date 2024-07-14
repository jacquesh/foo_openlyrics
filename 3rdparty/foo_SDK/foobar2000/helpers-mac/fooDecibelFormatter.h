#import <Foundation/Foundation.h>

@interface fooDecibelFormatter : NSFormatter

@property (nonatomic) NSNumber * minValue;
@property (nonatomic) NSNumber * maxValue;
@property (nonatomic) BOOL showSignAlways;
@end
