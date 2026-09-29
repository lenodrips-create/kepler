#pragma once

#include <QWebEngineUrlSchemeHandler>

// Serves Kepler's built-in pages:
//   kepler://start    - the new tab page
//   kepler://history  - browsing history
//   kepler://assets/* - images used by those pages
//   kepler://mode/*   - the Normal / Secure browsing switch
class SchemeHandler : public QWebEngineUrlSchemeHandler
{
    Q_OBJECT

public:
    explicit SchemeHandler(bool orbit, QObject *parent = nullptr);

    // Must run before QApplication is constructed.
    static void registerScheme();

    void requestStarted(QWebEngineUrlRequestJob *job) override;

signals:
    // From the start page's switch: kepler://mode/secure or kepler://mode/normal
    void modeRequested(bool secure);

private:
    QByteArray startPage() const;
    QByteArray historyPage() const;

    bool m_orbit;
};
