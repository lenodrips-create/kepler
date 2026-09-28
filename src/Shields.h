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

    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

    void setEnabled(bool on) { m_enabled = on; }
    bool isEnabled() const { return m_enabled; }

    int blockedOn(const QString &siteHost) const;
    int totalBlocked() const { return m_total; }

signals:
    void blocked(const QString &siteHost);

private:
    Shields();
    bool isTracker(QString host) const;

    QSet<QString> m_domains;
    std::atomic_bool m_enabled{true};
    std::atomic_int m_total{0};
    mutable QMutex m_mutex;
    QHash<QString, int> m_perSite;
};
