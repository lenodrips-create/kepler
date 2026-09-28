#pragma once

#include <QHash>
#include <QIcon>
#include <QObject>
#include <QSet>

class QNetworkAccessManager;

// Real site logos for bookmarks. Fetched once per site, then kept on disk.
class Favicons : public QObject
{
    Q_OBJECT

public:
    static Favicons &instance();

    // The site's logo if we have it; otherwise a placeholder while it downloads.
    QIcon icon(const QUrl &url);

    // Where logos come from (also used by the start page).
    static QUrl sourceFor(const QString &host);

signals:
    void iconReady(const QString &host);

private:
    Favicons();
    QString cachePath(const QString &host) const;
    void fetch(const QString &host);

    QNetworkAccessManager *m_net;
    QHash<QString, QIcon> m_icons;
    QSet<QString> m_pending;
};
