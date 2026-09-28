#include "SchemeHandler.h"

#include "Favicons.h"
#include "Shields.h"
#include "Storage.h"

#include <QBuffer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlScheme>

namespace {

QByteArray resource(const QString &path)
{
    QFile f(QStringLiteral(":/kepler/web/") + path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

// Embeds JSON inside a <script> block without letting "</script>" escape it.
QByteArray scriptSafe(const QJsonObject &o)
{
    return QJsonDocument(o).toJson(QJsonDocument::Compact).replace("</", "<\\/");
}

QByteArray mimeFor(const QString &path)
{
    if (path.endsWith(".png"))
        return "image/png";
    if (path.endsWith(".svg"))
        return "image/svg+xml";
    if (path.endsWith(".css"))
        return "text/css";
    if (path.endsWith(".js"))
        return "text/javascript";
    return "text/html";
}

} // namespace

SchemeHandler::SchemeHandler(bool orbit, QObject *parent)
    : QWebEngineUrlSchemeHandler(parent), m_orbit(orbit)
{
}

void SchemeHandler::registerScheme()
{
    QWebEngineUrlScheme scheme("kepler");
    scheme.setSyntax(QWebEngineUrlScheme::Syntax::Host);
    scheme.setFlags(QWebEngineUrlScheme::SecureScheme | QWebEngineUrlScheme::LocalAccessAllowed);
    QWebEngineUrlScheme::registerScheme(scheme);
}

void SchemeHandler::requestStarted(QWebEngineUrlRequestJob *job)
{
    const QUrl url = job->requestUrl();
    const QString host = url.host();

    QByteArray body;
    QByteArray mime = "text/html";
    if (host == QLatin1String("start")) {
        body = startPage();
    } else if (host == QLatin1String("history")) {
        body = historyPage();
    } else if (host == QLatin1String("assets")) {
        const QString path = url.path().mid(1);
        if (path.contains(QLatin1String("..")))
            return job->fail(QWebEngineUrlRequestJob::RequestDenied);
        body = resource(path);
        mime = mimeFor(path);
    }

    if (body.isEmpty())
        return job->fail(QWebEngineUrlRequestJob::UrlNotFound);

    auto *buffer = new QBuffer(job);
    buffer->setData(body);
    buffer->open(QIODevice::ReadOnly);
    job->reply(mime, buffer);
}

QByteArray SchemeHandler::startPage() const
{
    const Storage &s = Storage::instance();
    QJsonArray tiles;
    for (const auto &b : s.bookmarks()) {
        if (tiles.size() >= 8)
            break;
        tiles.append(QJsonObject{{"url", b.url},
                                 {"title", b.title},
                                 {"icon", Favicons::sourceFor(QUrl(b.url).host()).toString()}});
    }
    const QJsonObject data{
        {"search", s.searchTemplate()},
        {"engine", s.searchEngine()},
        {"tiles", tiles},
        {"blocked", Shields::instance()->totalBlocked()},
        {"shields", s.shieldsEnabled()},
        {"orbit", m_orbit},
    };
    return resource("start.html").replace("{{DATA}}", scriptSafe(data));
}

QByteArray SchemeHandler::historyPage() const
{
    QJsonArray items;
    const auto &history = Storage::instance().history();
    for (auto it = history.crbegin(); it != history.crend() && items.size() < 1500; ++it)
        items.append(QJsonObject{{"u", it->url}, {"t", it->title}, {"d", double(it->time)}});
    const QJsonObject data{{"items", items}, {"orbit", m_orbit}};
    return resource("history.html").replace("{{DATA}}", scriptSafe(data));
}
