#pragma once

#include <QWebEngineUrlSchemeHandler>

// Serves Kepler's built-in pages:
//   kepler://start    - the new tab page
//   kepler://history  - browsing history
//   kepler://assets/* - images used by those pages
class SchemeHandler : public QWebEngineUrlSchemeHandler
{
    Q_OBJECT

public:
    explicit SchemeHandler(bool orbit, QObject *parent = nullptr);

    // Must run before QApplication is constructed.
    static void registerScheme();

    void requestStarted(QWebEngineUrlRequestJob *job) override;

private:
    QByteArray startPage() const;
    QByteArray historyPage() const;

    bool m_orbit;
};
