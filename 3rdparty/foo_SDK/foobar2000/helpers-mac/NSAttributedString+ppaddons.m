#import "NSAttributedString+ppaddons.h"
#import <AppKit/AppKit.h>

@implementation NSAttributedString (ppaddons)

+ (NSAttributedString*) attributedStringWithImage: (NSImage*) img {
    if (img == nil) return nil;
    NSTextAttachment * attachment = [[NSTextAttachment alloc] init];
    NSTextAttachmentCell * cell = (NSTextAttachmentCell*) attachment.attachmentCell;
    cell.image = img;
    return [NSAttributedString attributedStringWithAttachment: attachment];
}

+ (NSAttributedString*) attributedStringWithImageNamed: (NSString*) name {
    if (name == nil) return nil;
    return [self attributedStringWithImage: [NSImage imageNamed: name]];
}

+ (NSAttributedString*) attributedStringWithImageNamed:(NSString *)name size: (NSSize) size {
    NSImage * img = [NSImage imageNamed: name];
    if (img == nil) return nil;
    img.size = size;
    return [self attributedStringWithImage: img];
}
+ (NSAttributedString*) attributedStringWithImageNamed:(NSString *)name squareSize: (CGFloat) size {
    return [self attributedStringWithImageNamed: name size: NSMakeSize(size, size)];
}

+(NSAttributedString*)hyperlinkFromString:(NSString*)inString withURL:(NSURL*)aURL
{
    NSMutableAttributedString* attrString = [[NSMutableAttributedString alloc] initWithString: inString];
    NSRange range = NSMakeRange(0, [attrString length]);
    
    [attrString beginEditing];
    [attrString addAttribute:NSLinkAttributeName value:[aURL absoluteString] range:range];
    
    // make the text appear in blue
    [attrString addAttribute:NSForegroundColorAttributeName value:[NSColor linkColor] range:range];
    
    // next make the text appear with an underline
    [attrString addAttribute:
    NSUnderlineStyleAttributeName value:[NSNumber numberWithInt:NSUnderlineStyleSingle] range:range];
    
    [attrString endEditing];
    
    return attrString;
}

@end
