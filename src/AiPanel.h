#pragma once

#include <QFrame>
#include <QList>
#include <QUrl>

class BrowserWindow;
class QButtonGroup;
class QPropertyAnimation;
class QStackedWidget;
class QToolButton;
class QWebEngineProfile;
class WebView;

// AI assistants, slid in over the current tab. Pick one from the row of
// pills; each keeps its own conversation while you switch or hide the panel.
class AiPanel : public QFrame
{
    Q_OBJECT

public:
    AiPanel(QWebEngineProfile *profile, BrowserWindow *window, QWidget *parent);

    bool isOpen() const { return m_open; }
    void toggle();
    void showPanel();
    void hidePanel();
    void reposition();

signals:
    void openChanged(bool open);

private:
    struct Assistant {
        QString name;
        QUrl home;
        QColor color;
        QToolButton *pill = nullptr;
        WebView *view = nullptr;
    };

    void select(int index);
    WebView *viewFor(int index);
    void refreshIcons();
    QRect targetGeometry() const;

    QWebEngineProfile *m_profile;
    BrowserWindow *m_window;
    QList<Assistant> m_assistants;
    QButtonGroup *m_group;
    QStackedWidget *m_stack;
    QPropertyAnimation *m_anim;
    int m_current = -1;
    bool m_open = false;
};
