#import <Foundation/Foundation.h>

@interface NSAttributedString (ppaddons)
+ (NSAttributedString*) attributedStringWithImage: (NSImage*) img;
+ (NSAttributedString*) attributedStringWithImageNamed: (NSString*) name;
+ (NSAttributedString*) attributedStringWithImageNamed:(NSString *)name size: (NSSize) size;
+ (NSAttributedString*) attributedStringWithImageNamed:(NSString *)name squareSize: (CGFloat) size;
+ (NSAttributedString*) hyperlinkFromString:(NSString*)inString withURL:(NSURL*)aURL;
@end
