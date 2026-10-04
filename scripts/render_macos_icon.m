// clang scripts/render_macos_icon.m -framework Foundation -framework CoreGraphics -framework ImageIO -o /tmp/ra-icon-render
// /tmp/ra-icon-render INPUT.png OUTPUT.png
#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ImageIO/ImageIO.h>
#include <math.h>
int main(int argc, const char **argv) {
    @autoreleasepool {
        if (argc != 3) return 2;
        CGImageSourceRef source = CGImageSourceCreateWithURL((__bridge CFURLRef)[NSURL fileURLWithPath:@(argv[1])], NULL);
        if (!source) return 1;
        CGImageRef art = CGImageSourceCreateImageAtIndex(source, 0, NULL);
        CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
        CGContextRef ctx = CGBitmapContextCreate(NULL, 1024, 1024, 8, 0, space, kCGImageAlphaPremultipliedLast);
        if (!art || !ctx) return 1;
        // 824px industrial rounded square, 100px transparent margins on a 1024px canvas.
        for (int i = 0; i <= 2048; ++i) {
            double a = i * 2 * M_PI / 2048, c = cos(a), s = sin(a);
            double x = 512 + 412 * copysign(pow(fabs(c), 1.0 / 6), c);
            double y = 512 + 412 * copysign(pow(fabs(s), 1.0 / 6), s);
            if (!i) CGContextMoveToPoint(ctx, x, y); else CGContextAddLineToPoint(ctx, x, y);
        }
        CGContextClosePath(ctx);
        CGContextClip(ctx);
        CGContextSetInterpolationQuality(ctx, kCGInterpolationHigh);
        CGContextDrawImage(ctx, CGRectMake(100, 100, 824, 824), art);
        CGImageRef image = CGBitmapContextCreateImage(ctx);
        CGImageDestinationRef dest = CGImageDestinationCreateWithURL((__bridge CFURLRef)[NSURL fileURLWithPath:@(argv[2])], CFSTR("public.png"), 1, NULL);
        if (!dest) return 1;
        CGImageDestinationAddImage(dest, image, NULL);
        BOOL ok = CGImageDestinationFinalize(dest);
        CFRelease(dest); CGImageRelease(image); CGContextRelease(ctx);
        CGColorSpaceRelease(space); CGImageRelease(art); CFRelease(source);
        return ok ? 0 : 1;
    }
}
