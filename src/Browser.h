#pragma once

#include <QList>
#include <QObject>
#include <QUrl>

class BrowserWindow;
class DownloadsPanel;
class QWebEngineProfile;

// Application-wide state: the browsing profile, open windows and downloads.
class Browser : public QObject
{
    Q_OBJECT

public:
    Browser();
    ~Browser() override;

    static Browser &instance();

    QWebEngineProfile *profile() const { return m_profile; }
    QWebEngineProfile *createOrbitProfile(QObject *owner);

    BrowserWindow *createWindow(bool orbit = false);
    QList<BrowserWindow *> windows() const { return m_windows; }
    BrowserWindow *activeWindow() const;

    DownloadsPanel *downloads() const { return m_downloads; }

    // The start page's Normal / Secure switch.
    void switchMode(bool secure);

    // Opens this computer's own terminal app.
    static void openTerminal(QWidget *parent);

    // Turns whatever was typed in the omnibox into somewhere to go.
    static QUrl urlFromInput(const QString &input);

    static QUrl startUrl() { return QUrl(QStringLiteral("kepler://start")); }
    static QUrl historyUrl() { return QUrl(QStringLiteral("kepler://history")); }

private:
    void setupProfile(QWebEngineProfile *profile, bool orbit);

    QWebEngineProfile *m_profile = nullptr;
    QList<BrowserWindow *> m_windows;
    DownloadsPanel *m_downloads = nullptr;
};
