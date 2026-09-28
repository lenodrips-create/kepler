#pragma once

#include <QFrame>

class BrowserWindow;
class QPropertyAnimation;
class QWebEngineProfile;
class WebView;

// Claude, slid in over the current tab. The page underneath stays put, and
// the conversation is kept while the panel is hidden.
class ClaudePanel : public QFrame
{
    Q_OBJECT

public:
    ClaudePanel(QWebEngineProfile *profile, BrowserWindow *window, QWidget *parent);

    bool isOpen() const { return m_open; }
    void toggle();
    void showPanel();
    void hidePanel();
    void reposition();

signals:
    void openChanged(bool open);

private:
    QRect targetGeometry() const;

    BrowserWindow *m_window;
    WebView *m_view;
    QPropertyAnimation *m_anim;
    bool m_open = false;
    bool m_loaded = false;
};
