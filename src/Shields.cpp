#include "Shields.h"

#include <QMutexLocker>

Shields *Shields::instance()
{
    static Shields *s = new Shields;
    return s;
}

Shields::Shields()
{
    // Well-known ad networks, trackers and analytics beacons.
    static const char *const list[] = {
        // ads
        "doubleclick.net", "googlesyndication.com", "googleadservices.com",
        "googletagservices.com", "adservice.google.com", "adnxs.com", "adsrvr.org",
        "advertising.com", "amazon-adsystem.com", "criteo.com", "criteo.net",
        "taboola.com", "outbrain.com", "rubiconproject.com", "pubmatic.com",
        "openx.net", "casalemedia.com", "bidswitch.net", "smartadserver.com",
        "yieldmo.com", "sharethrough.com", "teads.tv", "3lift.com", "indexww.com",
        "media.net", "zedo.com", "adform.net", "serving-sys.com", "popads.net",
        "popcash.net", "propellerads.com", "adsterra.com", "exoclick.com",
        "juicyads.com", "trafficjunky.net", "onclickads.net", "mgid.com",
        "revcontent.com", "zergnet.com", "adcolony.com", "moatads.com",
        "adsafeprotected.com", "doubleverify.com", "an.yandex.ru",
        // trackers & analytics
        "google-analytics.com", "googletagmanager.com", "scorecardresearch.com",
        "quantserve.com", "hotjar.com", "fullstory.com", "mouseflow.com",
        "crazyegg.com", "clarity.ms", "bat.bing.com", "connect.facebook.net",
        "analytics.tiktok.com", "px.ads.linkedin.com", "analytics.twitter.com",
        "ads-twitter.com", "chartbeat.com", "krxd.net", "bluekai.com",
        "demdex.net", "everesttech.net", "exelator.com", "rlcdn.com", "tapad.com",
        "mathtag.com", "mc.yandex.ru", "branch.io", "app-measurement.com",
    };
    for (const char *d : list)
        m_domains.insert(QString::fromLatin1(d));
}

bool Shields::isTracker(QString host) const
{
    // Match the host and every parent domain: ads.foo.doubleclick.net -> doubleclick.net
    while (!host.isEmpty()) {
        if (m_domains.contains(host))
            return true;
        const int dot = host.indexOf('.');
        if (dot < 0)
            break;
        host = host.mid(dot + 1);
    }
    return false;
}

void Shields::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    if (!m_enabled)
        return;
    // Never block what the user explicitly navigated to.
    if (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame)
        return;

    const QString host = info.requestUrl().host().toLower();
    if (!isTracker(host))
        return;

    const QString site = info.firstPartyUrl().host().toLower();
    if (isTracker(site))
        return; // visiting the tracker's own site on purpose

    info.block(true);
    ++m_total;
    {
        QMutexLocker lock(&m_mutex);
        ++m_perSite[site];
    }
    emit blocked(site);
}

int Shields::blockedOn(const QString &siteHost) const
{
    QMutexLocker lock(&m_mutex);
    return m_perSite.value(siteHost.toLower());
}
