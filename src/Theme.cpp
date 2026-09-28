#include "Theme.h"

#include <QApplication>
#include <QPalette>

namespace Theme {

QString styleSheet()
{
    return QStringLiteral(R"(
QWidget {
    color: #e6e6f5;
    font-size: 13px;
}
QMainWindow, #Central, #TopBar {
    background: #07070c;
}

/* ---------- tabs ---------- */
#Brand {
    padding: 0 6px 0 10px;
}
QTabBar {
    background: transparent;
    qproperty-drawBase: 0;
}
QTabBar::tab {
    background: transparent;
    color: #8b8ba7;
    padding: 8px 8px 8px 12px;
    margin: 6px 1px 0 1px;
    border-top-left-radius: 11px;
    border-top-right-radius: 11px;
    min-width: 60px;
    max-width: 220px;
}
QTabBar::tab:hover {
    background: #12121d;
    color: #cfcfe6;
}
QTabBar::tab:selected {
    background: #11111b;
    color: #f5f5ff;
}
QTabBar::close-button {
    image: url(:/kepler/icons/close-small.svg);
    subcontrol-position: right;
    border-radius: 5px;
    padding: 1px;
}
QTabBar::close-button:hover {
    image: url(:/kepler/icons/close-hover.svg);
    background: #2a2a42;
}
QTabBar QToolButton {
    background: #07070c;
    border: none;
    border-radius: 6px;
}
#NewTabButton {
    margin-top: 6px;
}

/* ---------- nav bar ---------- */
#NavBar {
    background: #11111b;
}
QToolButton {
    background: transparent;
    border: none;
    border-radius: 9px;
    padding: 6px;
}
QToolButton:hover {
    background: #1f1f31;
}
QToolButton:pressed, QToolButton:checked {
    background: #2a2a42;
}
QToolButton::menu-indicator {
    image: none;
    width: 0;
}
#UrlBar {
    background: #07070c;
    border: 1px solid #23233a;
    border-radius: 16px;
    padding: 0 14px 0 6px;
    min-height: 32px;
    max-height: 32px;
    color: #f5f5ff;
    font-size: 14px;
    selection-background-color: #6d4aff;
}
#UrlBar:hover {
    border-color: #33334f;
}
#UrlBar:focus {
    border: 1px solid #8b5cf6;
    background: #0a0a12;
}
#ZoomButton {
    color: #c4b5fd;
    font-size: 12px;
    font-weight: 600;
    padding: 4px 9px;
    border: 1px solid #2d2448;
    border-radius: 12px;
}
#ShieldButton {
    color: #67e8f9;
    font-size: 12px;
    font-weight: 600;
    padding: 5px 8px;
}
#OrbitBadge {
    color: #f5d0fe;
    background: #2a1238;
    border: 1px solid #4a1d63;
    border-radius: 12px;
    padding: 4px 10px;
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 2px;
}
#LoadBar {
    background: #11111b;
    border: none;
    max-height: 2px;
    min-height: 2px;
}
#LoadBar::chunk {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #8b5cf6, stop:1 #22d3ee);
}

/* ---------- bookmarks bar ---------- */
#BookmarksBar {
    background: #11111b;
    border: none;
    border-bottom: 1px solid #1b1b2b;
    padding: 0 8px 4px 8px;
    spacing: 2px;
}
#BookmarksBar QToolButton {
    color: #b8b8d0;
    font-size: 12px;
    padding: 4px 9px;
    border-radius: 7px;
}
#BookmarksBar QToolButton:hover {
    background: #1f1f31;
    color: #f5f5ff;
}

/* ---------- overlays ---------- */
#StatusBubble {
    background: rgba(18, 18, 28, 235);
    color: #b8b8d0;
    border: 1px solid #26263a;
    border-radius: 7px;
    padding: 4px 9px;
    font-size: 12px;
}
#FindBar {
    background: #12121c;
    border: 1px solid #2a2a42;
    border-radius: 12px;
}
#FindBar QLineEdit {
    background: transparent;
    border: none;
    padding: 4px;
    min-width: 200px;
}
#FindCount {
    color: #8b8ba7;
    font-size: 12px;
    padding: 0 6px;
}

/* ---------- downloads ---------- */
#DownloadsCard {
    background: #12121c;
    border: 1px solid #2a2a42;
    border-radius: 14px;
}
#DownloadsTitle {
    font-size: 15px;
    font-weight: 700;
    color: #f5f5ff;
}
#DownloadItem {
    background: #181826;
    border-radius: 10px;
}
#DownloadName {
    color: #f0f0ff;
    font-weight: 600;
}
#DownloadStatus, #DownloadsEmpty {
    color: #8b8ba7;
    font-size: 12px;
}
#DownloadItem QProgressBar {
    background: #07070c;
    border: none;
    border-radius: 2px;
    max-height: 4px;
    min-height: 4px;
}
#DownloadItem QProgressBar::chunk {
    border-radius: 2px;
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #8b5cf6, stop:1 #22d3ee);
}
QPushButton {
    background: #1f1f31;
    border: 1px solid #2a2a42;
    border-radius: 8px;
    padding: 6px 14px;
}
QPushButton:hover {
    background: #2a2a42;
}
QPushButton:default {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #7c3aed, stop:1 #0891b2);
    border: none;
    color: white;
}

/* ---------- generic ---------- */
QLineEdit {
    background: #0a0a12;
    border: 1px solid #23233a;
    border-radius: 8px;
    padding: 5px 8px;
    selection-background-color: #6d4aff;
}
QMenu {
    background: #12121c;
    border: 1px solid #2a2a42;
    border-radius: 12px;
    padding: 6px;
}
QMenu::item {
    padding: 7px 32px 7px 12px;
    border-radius: 7px;
    color: #e6e6f5;
}
QMenu::item:selected {
    background: #24243a;
}
QMenu::item:disabled {
    color: #5a5a73;
}
QMenu::separator {
    height: 1px;
    background: #23233a;
    margin: 5px 8px;
}
QMenu::icon {
    padding-left: 8px;
}
QAbstractItemView {
    background: #12121c;
    border: 1px solid #2a2a42;
    color: #e6e6f5;
    selection-background-color: #24243a;
    selection-color: #ffffff;
    outline: 0;
    padding: 4px;
}
QAbstractItemView::item {
    padding: 6px 8px;
    border-radius: 6px;
}
QScrollArea, QScrollArea > QWidget > QWidget {
    background: transparent;
    border: none;
}
QScrollBar:vertical {
    background: transparent;
    width: 10px;
    margin: 2px;
}
QScrollBar::handle:vertical {
    background: #2a2a42;
    border-radius: 3px;
    min-height: 30px;
}
QScrollBar::add-line, QScrollBar::sub-line {
    height: 0;
    width: 0;
}
QToolTip {
    background: #12121c;
    color: #e6e6f5;
    border: 1px solid #2a2a42;
    padding: 5px 9px;
}
QDialog, QMessageBox {
    background: #11111b;
}
)");
}

QString orbitStyleSheet()
{
    // Orbit (private) windows swap the violet/cyan nebula for a magenta one.
    return QStringLiteral(R"(
#NavBar, #BookmarksBar, #LoadBar { background: #150d22; }
QTabBar::tab:selected { background: #150d22; }
QTabBar::tab:hover { background: #120b1c; }
#UrlBar:focus { border-color: #d946ef; }
#LoadBar::chunk {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #d946ef, stop:1 #8b5cf6);
}
)");
}

void applyPalette()
{
    QPalette p;
    p.setColor(QPalette::Window, Surface);
    p.setColor(QPalette::WindowText, Text);
    p.setColor(QPalette::Base, Void);
    p.setColor(QPalette::AlternateBase, Raised);
    p.setColor(QPalette::Text, Text);
    p.setColor(QPalette::Button, Raised);
    p.setColor(QPalette::ButtonText, Text);
    p.setColor(QPalette::ToolTipBase, Surface);
    p.setColor(QPalette::ToolTipText, Text);
    p.setColor(QPalette::PlaceholderText, Muted);
    p.setColor(QPalette::Highlight, QColor(0x6d, 0x4a, 0xff));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Link, Cyan);
    p.setColor(QPalette::Disabled, QPalette::Text, Muted);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, Muted);
    p.setColor(QPalette::Disabled, QPalette::WindowText, Muted);
    QApplication::setPalette(p);
}

} // namespace Theme
