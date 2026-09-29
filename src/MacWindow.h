#pragma once

#include <QtGlobal>

class QWidget;

// macOS: hide the title bar so Kepler's dark top bar fills it, with the
// red/yellow/green buttons left in their usual spot. No-ops elsewhere.
namespace MacWindow {

#ifdef Q_OS_MACOS
void setup(QWidget *window);
bool isNativeFullScreen(QWidget *window);
#else
inline void setup(QWidget *) {}
inline bool isNativeFullScreen(QWidget *) { return false; }
#endif

// Space the window buttons take up at the left of the top bar.
constexpr int TrafficLightsInset = 78;

} // namespace MacWindow
