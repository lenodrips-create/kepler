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

bool isNativeFullScreen(QWidget *window)
{
    NSWindow *w = nsWindowFor(window);
    return w && (w.styleMask & NSWindowStyleMaskFullScreen);
}

} // namespace MacWindow
