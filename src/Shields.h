#pragma once

#include <QHash>
#include <QMutex>
#include <QSet>
#include <QWebEngineUrlRequestInterceptor>

#include <atomic>

// Shields: a built-in ad & tracker blocker. Third-party requests to known
// ad/tracking networks are dropped before they ever leave the machine.
class Shields : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT

public:
    static Shields *instance();
    // Secure browsing: always on, and upgrades plain http pages to https.
    static Shields *secureInstance();

    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

    void setEnabled(bool on) { m_enabled = on; }
    bool isEnabled() const { return m_enabled; }

    int blockedOn(const QString &siteHost) const;
    int totalBlocked() const { return s_total; }

signals:
    void blocked(const QString &siteHost);

private:
    explicit Shields(bool secure);
    bool isTracker(QString host) const;

    const bool m_secure;
    QSet<QString> m_domains;
    std::atomic_bool m_enabled{true};

    // Counts are shared by the normal and secure blockers.
    static std::atomic_int s_total;
    static QMutex s_mutex;
    static QHash<QString, int> s_perSite;
};
