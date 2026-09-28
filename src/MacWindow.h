#pragma once

#include <QtGlobal>

class QWidget;

// macOS: blend the title bar into Kepler's top bar and move the
// red/yellow/green window buttons next to the logo. No-ops elsewhere.
namespace MacWindow {

#ifdef Q_OS_MACOS
void setup(QWidget *window);
void placeTrafficLights(QWidget *window, int x, int barHeight);
bool isNativeFullScreen(QWidget *window);
#else
inline void setup(QWidget *) {}
inline void placeTrafficLights(QWidget *, int, int) {}
inline bool isNativeFullScreen(QWidget *) { return false; }
#endif

// Space the window buttons take up at the left of the top bar.
constexpr int TrafficLightsInset = 80;

} // namespace MacWindow
