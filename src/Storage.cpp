#include "Storage.h"

#include <algorithm>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QUrlQuery>

namespace {

constexpr int MaxHistory = 5000;

struct Engine {
    const char *name;
    const char *pattern; // %1 = encoded query
};

const Engine Engines[] = {
    {"Google", "https://www.google.com/search?q=%1"},
    {"DuckDuckGo", "https://duckduckgo.com/?q=%1"},
    {"Bing", "https://www.bing.com/search?q=%1"},
    {"Brave", "https://search.brave.com/search?q=%1"},
};

QJsonDocument readJson(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(f.readAll());
}

void writeJson(const QString &path, const QJsonDocument &doc)
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return;
    f.write(doc.toJson(QJsonDocument::Compact));
    f.commit();
}

} // namespace

Storage &Storage::instance()
{
    static Storage s;
    return s;
}

Storage::Storage()
{
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    m_historySaveTimer.setSingleShot(true);
    m_historySaveTimer.setInterval(1500);
    connect(&m_historySaveTimer, &QTimer::timeout, this, &Storage::saveHistory);
    load();
}

QString Storage::dataFile(const QString &name) const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + '/' + name;
}

void Storage::load()
{
    const auto history = readJson(dataFile("history.json")).array();
    for (const auto &v : history) {
        const auto o = v.toObject();
        m_history.append({o["u"].toString(), o["t"].toString(), qint64(o["d"].toDouble())});
    }

    const QString bookmarksPath = dataFile("bookmarks.json");
    if (QFile::exists(bookmarksPath)) {
        const auto bookmarks = readJson(bookmarksPath).array();
        for (const auto &v : bookmarks) {
            const auto o = v.toObject();
            m_bookmarks.append({o["url"].toString(), o["title"].toString()});
        }
    } else {
        // First launch: a few places worth visiting.
        m_bookmarks = {
            {"https://edudeck.tech/", "EduDeck"},
            {"https://www.youtube.com/", "YouTube"},
            {"https://www.tiktok.com/", "TikTok"},
            {"https://www.instagram.com/", "Instagram"},
            {"https://github.com/", "GitHub"},
            {"https://en.wikipedia.org/", "Wikipedia"},
            {"https://www.nasa.gov/", "NASA"},
            {"https://science.nasa.gov/mission/kepler/", "Kepler Mission"},
        };
        saveBookmarks();
    }

    // Kepler 1.1 added TikTok and Instagram to the defaults; give existing users them too.
    QSettings settings;
    if (settings.value("bookmarks/defaultsVersion", 1).toInt() < 2) {
        const QList<Bookmark> added{{"https://www.tiktok.com/", "TikTok"},
                                    {"https://www.instagram.com/", "Instagram"}};
        int at = 0;
        for (int i = 0; i < m_bookmarks.size(); ++i)
            if (m_bookmarks[i].url == QLatin1String("https://www.youtube.com/"))
                at = i + 1;
        for (const Bookmark &b : added) {
            if (!isBookmarked(QUrl(b.url)))
                m_bookmarks.insert(at++, b);
        }
        saveBookmarks();
        settings.setValue("bookmarks/defaultsVersion", 2);
    }
}

void Storage::flush()
{
    if (m_historySaveTimer.isActive()) {
        m_historySaveTimer.stop();
        saveHistory();
    }
}

void Storage::saveHistory()
{
    QJsonArray arr;
    for (const auto &e : m_history)
        arr.append(QJsonObject{{"u", e.url}, {"t", e.title}, {"d", double(e.time)}});
    writeJson(dataFile("history.json"), QJsonDocument(arr));
}

void Storage::saveBookmarks()
{
    QJsonArray arr;
    for (const auto &b : m_bookmarks)
        arr.append(QJsonObject{{"url", b.url}, {"title", b.title}});
    writeJson(dataFile("bookmarks.json"), QJsonDocument(arr));
}

void Storage::addHistory(const QUrl &url, const QString &title)
{
    const QString s = url.toString();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    // Reloads and title updates of the page we just saw refresh the last entry.
    if (!m_history.isEmpty() && m_history.last().url == s) {
        m_history.last().title = title;
        m_history.last().time = now;
    } else {
        m_history.append({s, title, now});
        if (m_history.size() > MaxHistory)
            m_history.remove(0, m_history.size() - MaxHistory);
    }
    m_historySaveTimer.start();
    emit historyChanged();
}

void Storage::updateHistoryTitle(const QUrl &url, const QString &title)
{
    // Pages often set their title after loading; patch the recent visit.
    const QString s = url.toString();
    const int stop = qMax(0, int(m_history.size()) - 20);
    for (int i = int(m_history.size()) - 1; i >= stop; --i) {
        if (m_history[i].url == s) {
            if (m_history[i].title != title) {
                m_history[i].title = title;
                m_historySaveTimer.start();
            }
            return;
        }
    }
}

void Storage::clearHistory()
{
    m_history.clear();
    saveHistory();
    emit historyChanged();
}

bool Storage::isBookmarked(const QUrl &url) const
{
    const QString s = url.toString();
    for (const auto &b : m_bookmarks)
        if (b.url == s)
            return true;
    return false;
}

void Storage::toggleBookmark(const QUrl &url, const QString &title)
{
    const QString s = url.toString();
    if (isBookmarked(url)) {
        removeBookmark(s);
        return;
    }
    m_bookmarks.append({s, title.isEmpty() ? url.host() : title});
    saveBookmarks();
    emit bookmarksChanged();
}

void Storage::removeBookmark(const QString &url)
{
    m_bookmarks.erase(std::remove_if(m_bookmarks.begin(), m_bookmarks.end(),
                                     [&](const Bookmark &b) { return b.url == url; }),
                      m_bookmarks.end());
    saveBookmarks();
    emit bookmarksChanged();
}

QStringList Storage::completions() const
{
    QStringList out;
    QSet<QString> seen;
    for (const auto &b : m_bookmarks) {
        if (!seen.contains(b.url)) {
            seen.insert(b.url);
            out << b.url;
        }
    }
    for (auto it = m_history.crbegin(); it != m_history.crend() && out.size() < 800; ++it) {
        if (!seen.contains(it->url)) {
            seen.insert(it->url);
            out << it->url;
        }
    }
    return out;
}

QStringList Storage::searchEngineNames()
{
    QStringList names;
    for (const auto &e : Engines)
        names << QString::fromLatin1(e.name);
    return names;
}

QString Storage::searchEngine() const
{
    const QString name = QSettings().value("search/engine", "Google").toString();
    return searchEngineNames().contains(name) ? name : QStringLiteral("Google");
}

void Storage::setSearchEngine(const QString &name)
{
    QSettings().setValue("search/engine", name);
    emit settingsChanged();
}

QString Storage::searchTemplate() const
{
    const QString name = searchEngine();
    for (const auto &e : Engines)
        if (name == QLatin1String(e.name))
            return QString::fromLatin1(e.pattern);
    return QString::fromLatin1(Engines[0].pattern);
}

QUrl Storage::searchUrl(const QString &query) const
{
    return QUrl(searchTemplate().arg(QString::fromLatin1(QUrl::toPercentEncoding(query))));
}

bool Storage::shieldsEnabled() const
{
    return QSettings().value("shields/enabled", true).toBool();
}

void Storage::setShieldsEnabled(bool on)
{
    QSettings().setValue("shields/enabled", on);
    emit settingsChanged();
}

bool Storage::bookmarksBarVisible() const
{
    return QSettings().value("ui/bookmarksBar", true).toBool();
}

void Storage::setBookmarksBarVisible(bool on)
{
    QSettings().setValue("ui/bookmarksBar", on);
    emit settingsChanged();
}

QStringList Storage::sessionUrls() const
{
    return QSettings().value("session/urls").toStringList();
}

void Storage::saveSession(const QStringList &urls)
{
    QSettings().setValue("session/urls", urls);
}
