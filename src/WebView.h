#pragma once

#include <QPointer>
#include <QWebEngineView>

class BrowserWindow;
class QWebEngineProfile;

// One tab's worth of web content.
class WebView : public QWebEngineView
{
    Q_OBJECT

public:
    WebView(QWebEngineProfile *profile, BrowserWindow *window);
    ~WebView() override;

    bool isLoading() const { return m_loading; }
    int progress() const { return m_progress; }

    void openDevTools();

    static bool isStartPage(const QUrl &url);

protected:
    QWebEngineView *createWindow(QWebEnginePage::WebWindowType type) override;

private:
    void askPermission(const QUrl &origin, int feature);

    BrowserWindow *m_window;
    bool m_loading = false;
    int m_progress = 0;
    QPointer<QWebEngineView> m_devTools;
};
