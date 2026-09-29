#include "Browser.h"

#include "BrowserWindow.h"
#include "DownloadsPanel.h"
#include "SchemeHandler.h"
#include "Shields.h"
#include "Storage.h"

#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QWebEngineCookieStore>
#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QWebEngineDownloadRequest>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

namespace {
Browser *s_instance = nullptr;
}

Browser::Browser()
{
    s_instance = this;

    m_profile = new QWebEngineProfile(QStringLiteral("Kepler"), this);
    m_profile->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);
    setupProfile(m_profile, false);

    m_downloads = new DownloadsPanel;

    Shields::instance()->setEnabled(Storage::instance().shieldsEnabled());
    connect(&Storage::instance(), &Storage::settingsChanged, this,
            [] { Shields::instance()->setEnabled(Storage::instance().shieldsEnabled()); });
}

Browser::~Browser()
{
    // Pages must die before the profile that owns their data.
    const auto windows = m_windows;
    qDeleteAll(windows);
    delete m_downloads;
    delete m_profile;
    Storage::instance().flush();
    s_instance = nullptr;
}

Browser &Browser::instance()
{
    Q_ASSERT(s_instance);
    return *s_instance;
}

QWebEngineProfile *Browser::createOrbitProfile(QObject *owner)
{
    auto *profile = new QWebEngineProfile(owner); // no storage name => off the record
    setupProfile(profile, true);
    return profile;
}

void Browser::setupProfile(QWebEngineProfile *profile, bool orbit)
{
    // Present as a regular Chromium so sites don't serve a degraded experience.
    QString ua = profile->httpUserAgent();
    ua.remove(QRegularExpression(QStringLiteral("QtWebEngine/[\\d.]+ ")));
    profile->setHttpUserAgent(ua + QStringLiteral(" Kepler/" KEPLER_VERSION));

    auto *handler = new SchemeHandler(orbit, profile);
    profile->installUrlSchemeHandler("kepler", handler);
    connect(handler, &SchemeHandler::modeRequested, this, &Browser::switchMode, Qt::QueuedConnection);

    if (orbit) {
        // Secure browsing: every tracker blocked, https only, no cross-site cookies.
        profile->setUrlRequestInterceptor(Shields::secureInstance());
        profile->cookieStore()->setCookieFilter(
            [](const QWebEngineCookieStore::FilterRequest &request) { return !request.thirdParty; });
    } else {
        profile->setUrlRequestInterceptor(Shields::instance());
    }
    profile->setDownloadPath(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    profile->settings()->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, true);

    connect(profile, &QWebEngineProfile::downloadRequested, this, [this](QWebEngineDownloadRequest *d) {
        d->accept();
        m_downloads->addDownload(d);
        if (BrowserWindow *w = activeWindow())
            w->showDownloads();
    });
}

BrowserWindow *Browser::createWindow(bool orbit)
{
    auto *w = new BrowserWindow(orbit);
    m_windows.append(w);
    connect(w, &QObject::destroyed, this, [this, w] { m_windows.removeOne(w); });
    return w;
}

void Browser::switchMode(bool secure)
{
    BrowserWindow *from = activeWindow();
    if (from && from->isOrbit() == secure)
        return;

    BrowserWindow *to = createWindow(secure);
    to->newTab();
    if (from) {
        to->setGeometry(from->geometry());
        if (from->isMaximized())
            to->setWindowState(Qt::WindowMaximized);
    }
    to->show();
    to->activateWindow();

    // Nothing open but start pages? Then this is a straight switch.
    if (from && from->sessionUrls().isEmpty())
        from->close();
}

void Browser::openTerminal(QWidget *parent)
{
    const QString home = QDir::homePath();
    bool ok = false;
#if defined(Q_OS_MACOS)
    ok = QProcess::startDetached(QStringLiteral("open"), {QStringLiteral("-a"), QStringLiteral("Terminal"), home});
#elif defined(Q_OS_WIN)
    const QString wt = QStandardPaths::findExecutable(QStringLiteral("wt.exe"));
    ok = !wt.isEmpty() ? QProcess::startDetached(wt, {QStringLiteral("-d"), home})
                       : QProcess::startDetached(QStringLiteral("cmd.exe"), {}, home);
#else
    static const char *const terminals[] = {"x-terminal-emulator", "gnome-terminal", "konsole",
                                            "xfce4-terminal", "kitty", "alacritty", "xterm"};
    for (const char *t : terminals) {
        const QString path = QStandardPaths::findExecutable(QString::fromLatin1(t));
        if (!path.isEmpty() && QProcess::startDetached(path, {}, home)) {
            ok = true;
            break;
        }
    }
#endif
    if (!ok)
        QMessageBox::warning(parent, tr("Terminal"), tr("Kepler couldn't find a terminal app to open."));
}

BrowserWindow *Browser::activeWindow() const
{
    if (auto *w = qobject_cast<BrowserWindow *>(QApplication::activeWindow()))
        return w;
    return m_windows.isEmpty() ? nullptr : m_windows.last();
}

QUrl Browser::urlFromInput(const QString &input)
{
    const QString text = input.trimmed();
    if (text.isEmpty())
        return {};

    static const QStringList schemes{"http", "https", "file", "ftp", "kepler", "about",
                                     "view-source", "data", "chrome", "qrc"};
    const int colon = text.indexOf(':');
    if (colon > 0 && !text.contains(' ') && schemes.contains(text.left(colon).toLower()))
        return QUrl(text);

    // Looks like an address? (example.com, localhost:3000, 192.168.0.1, ./file.html)
    static const QRegularExpression host(
        QStringLiteral("^(localhost|[\\w-]+(\\.[\\w-]+)+)(:\\d+)?([/?#].*)?$"));
    if (!text.contains(' ') && host.match(text).hasMatch()) {
        const QUrl url = QUrl::fromUserInput(text);
        if (url.isValid())
            return url;
    }
    if (text.startsWith('/') || text.startsWith(QLatin1String("./")) || text.startsWith('~')) {
        const QUrl url = QUrl::fromUserInput(text, QDir::currentPath());
        if (url.isValid())
            return url;
    }
    return Storage::instance().searchUrl(text);
}
