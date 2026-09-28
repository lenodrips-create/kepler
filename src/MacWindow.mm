#include "MacWindow.h"

#include <QWidget>

#import <AppKit/AppKit.h>

namespace {

NSWindow *nsWindowFor(QWidget *widget)
{
    auto *view = reinterpret_cast<NSView *>(widget->window()->winId());
    return view.window;
}

} // namespace

namespace MacWindow {

void setup(QWidget *window)
{
    NSWindow *w = nsWindowFor(window);
    if (!w)
        return;
    // Let Kepler's own top bar fill the space the title bar used to take.
    w.titlebarAppearsTransparent = YES;
    w.titleVisibility = NSWindowTitleHidden;
    w.styleMask |= NSWindowStyleMaskFullSizeContentView;
    w.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    w.backgroundColor = [NSColor colorWithSRGBRed:7 / 255.0 green:7 / 255.0 blue:12 / 255.0 alpha:1.0];
}

void placeTrafficLights(QWidget *window, int x, int barHeight)
{
    NSWindow *w = nsWindowFor(window);
    if (!w || (w.styleMask & NSWindowStyleMaskFullScreen))
        return;

    NSButton *close = [w standardWindowButton:NSWindowCloseButton];
    NSButton *minimize = [w standardWindowButton:NSWindowMiniaturizeButton];
    NSButton *zoom = [w standardWindowButton:NSWindowZoomButton];
    if (!close || !minimize || !zoom)
        return;

    // Stretch the (transparent) title bar to match Kepler's top bar, then
    // center the buttons in it. AppKit resets this on resize, so callers
    // re-run it whenever the window changes.
    NSView *container = close.superview.superview;
    NSRect bar = container.frame;
    bar.size.height = barHeight;
    bar.origin.y = w.frame.size.height - barHeight;
    container.frame = bar;

    const CGFloat spacing = minimize.frame.origin.x - close.frame.origin.x;
    NSArray<NSButton *> *buttons = @[ close, minimize, zoom ];
    for (NSUInteger i = 0; i < buttons.count; ++i) {
        NSButton *b = buttons[i];
        NSPoint origin = b.frame.origin;
        origin.x = x + i * spacing;
        origin.y = (barHeight - b.frame.size.height) / 2.0;
        [b setFrameOrigin:origin];
    }
}

bool isNativeFullScreen(QWidget *window)
{
    NSWindow *w = nsWindowFor(window);
    return w && (w.styleMask & NSWindowStyleMaskFullScreen);
}

} // namespace MacWindow
