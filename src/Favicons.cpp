#include "Favicons.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>

Favicons &Favicons::instance()
{
    static Favicons f;
    return f;
}

Favicons::Favicons() : m_net(new QNetworkAccessManager(this))
{
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/favicons");
}

QUrl Favicons::sourceFor(const QString &host)
{
    return QUrl(QStringLiteral("https://www.google.com/s2/favicons?sz=64&domain=") + host);
}

QString Favicons::cachePath(const QString &host) const
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/favicons/" + host + ".png";
}

QIcon Favicons::icon(const QUrl &url)
{
    const QString host = url.host().toLower();
    if (host.isEmpty())
        return QIcon(":/kepler/icons/globe.svg");
    if (auto it = m_icons.constFind(host); it != m_icons.constEnd())
        return *it;

    const QString path = cachePath(host);
    if (QFile::exists(path)) {
        const QIcon icon(path);
        m_icons.insert(host, icon);
        return icon;
    }
    fetch(host);
    return QIcon(":/kepler/icons/globe.svg");
}

void Favicons::fetch(const QString &host)
{
    if (m_pending.contains(host))
        return;
    m_pending.insert(host);

    QNetworkRequest request(sourceFor(host));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = m_net->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, host] {
        reply->deleteLater();
        m_pending.remove(host);
        const QImage image = QImage::fromData(reply->readAll());
        if (reply->error() != QNetworkReply::NoError || image.isNull())
            return; // try again next launch
        QSaveFile file(cachePath(host));
        if (file.open(QIODevice::WriteOnly) && image.save(&file, "PNG"))
            file.commit();
        m_icons.insert(host, QIcon(QPixmap::fromImage(image)));
        emit iconReady(host);
    });
}
