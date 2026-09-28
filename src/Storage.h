#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>

struct HistoryEntry {
    QString url;
    QString title;
    qint64 time = 0; // msecs since epoch
};

struct Bookmark {
    QString url;
    QString title;
};

// Everything Kepler remembers between launches: history, bookmarks,
// settings and the last session. Lives on disk as JSON in the app data dir.
class Storage : public QObject
{
    Q_OBJECT

public:
    static Storage &instance();

    // history
    void addHistory(const QUrl &url, const QString &title);
    void updateHistoryTitle(const QUrl &url, const QString &title);
    const QList<HistoryEntry> &history() const { return m_history; }
    void clearHistory();

    // bookmarks
    bool isBookmarked(const QUrl &url) const;
    void toggleBookmark(const QUrl &url, const QString &title);
    void removeBookmark(const QString &url);
    const QList<Bookmark> &bookmarks() const { return m_bookmarks; }

    // omnibox suggestions: bookmarks first, then recent history
    QStringList completions() const;

    // settings
    static QStringList searchEngineNames();
    QString searchEngine() const;
    void setSearchEngine(const QString &name);
    QUrl searchUrl(const QString &query) const;
    QString searchTemplate() const;

    bool shieldsEnabled() const;
    void setShieldsEnabled(bool on);

    bool bookmarksBarVisible() const;
    void setBookmarksBarVisible(bool on);

    QStringList sessionUrls() const;
    void saveSession(const QStringList &urls);

    void flush();

signals:
    void historyChanged();
    void bookmarksChanged();
    void settingsChanged();

private:
    Storage();
    QString dataFile(const QString &name) const;
    void load();
    void saveHistory();
    void saveBookmarks();

    QList<HistoryEntry> m_history; // oldest first
    QList<Bookmark> m_bookmarks;
    QTimer m_historySaveTimer;
};
