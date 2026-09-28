#pragma once

#include <QColor>
#include <QString>

// Kepler's look: deep-space black, starlight white, and a violet -> cyan nebula accent.
namespace Theme {

inline const QColor Void{0x07, 0x07, 0x0c};
inline const QColor Surface{0x11, 0x11, 0x1b};
inline const QColor Raised{0x1a, 0x1a, 0x28};
inline const QColor Border{0x23, 0x23, 0x3a};
inline const QColor Text{0xe6, 0xe6, 0xf5};
inline const QColor Muted{0x8b, 0x8b, 0xa7};
inline const QColor Violet{0x8b, 0x5c, 0xf6};
inline const QColor Cyan{0x22, 0xd3, 0xee};

QString styleSheet();
QString orbitStyleSheet(); // extra layer for private ("Orbit") windows
void applyPalette();

} // namespace Theme
