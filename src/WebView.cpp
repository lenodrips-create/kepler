#include "WebView.h"

#include "Browser.h"
#include "BrowserWindow.h"

#include <QIcon>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

WebView::WebView(QWebEngineProfile *profile, BrowserWindow *window)
    : QWebEngineView(window), m_window(window)
{
    auto *page = new QWebEnginePage(profile, this);
    setPage(page);
    // Match the start page so new tabs never flash white.
    page->setBackgroundColor(QColor(Qt::black));

    auto *s = page->settings();
    s->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    s->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);
    s->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    s->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    s->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, true);

    connect(this, &QWebEngineView::loadStarted, this, [this] {
        m_loading = true;
        m_progress = 5;
    });
    connect(this, &QWebEngineView::loadProgress, this, [this](int p) { m_progress = p; });
    connect(this, &QWebEngineView::loadFinished, this, [this] {
        m_loading = false;
        m_progress = 100;
    });
    connect(this, &QWebEngineView::urlChanged, this, [this](const QUrl &url) {
        // Built-in pages are dark; everything else gets the web's default white canvas.
        this->page()->setBackgroundColor(url.scheme() == QLatin1String("kepler") || url.isEmpty()
                                             ? QColor(Qt::black)
                                             : QColor(Qt::white));
    });

    connect(page, &QWebEnginePage::renderProcessTerminated, this,
            [this](QWebEnginePage::RenderProcessTerminationStatus status, int) {
                if (status == QWebEnginePage::NormalTerminationStatus)
                    return;
                // The tab crashed: give it a moment, then bring it back.
                QTimer::singleShot(800, this, [this] { reload(); });
            });

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    connect(page, &QWebEnginePage::featurePermissionRequested, this,
            [this](const QUrl &origin, QWebEnginePage::Feature feature) {
                askPermission(origin, int(feature));
            });
QT_WARNING_POP
}

WebView::~WebView()
{
    delete m_devTools;
}

bool WebView::isStartPage(const QUrl &url)
{
    return url.scheme() == QLatin1String("kepler") && url.host() == QLatin1String("start");
}

QWebEngineView *WebView::createWindow(QWebEnginePage::WebWindowType type)
{
    switch (type) {
    case QWebEnginePage::WebBrowserBackgroundTab:
        return m_window->createTab(false, true);
    case QWebEnginePage::WebBrowserWindow: {
        BrowserWindow *w = Browser::instance().createWindow(m_window->isOrbit());
        w->show();
        return w->createTab(true, false);
    }
    case QWebEnginePage::WebBrowserTab:
    case QWebEnginePage::WebDialog:
    default:
        return m_window->createTab(true, true);
    }
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
void WebView::askPermission(const QUrl &origin, int f)
{
    const auto feature = QWebEnginePage::Feature(f);
    QString what;
    switch (feature) {
    case QWebEnginePage::Notifications: what = tr("show notifications"); break;
    case QWebEnginePage::Geolocation: what = tr("know your location"); break;
    case QWebEnginePage::MediaAudioCapture: what = tr("use your microphone"); break;
    case QWebEnginePage::MediaVideoCapture: what = tr("use your camera"); break;
    case QWebEnginePage::MediaAudioVideoCapture: what = tr("use your camera and microphone"); break;
    case QWebEnginePage::MouseLock: what = tr("lock your mouse pointer"); break;
    case QWebEnginePage::DesktopVideoCapture:
    case QWebEnginePage::DesktopAudioVideoCapture: what = tr("share your screen"); break;
    default: what = tr("use an extra feature"); break;
    }

    // Games commonly lock the pointer; allow it without nagging.
    if (feature == QWebEnginePage::MouseLock) {
        page()->setFeaturePermission(origin, feature, QWebEnginePage::PermissionGrantedByUser);
        return;
    }

    QMessageBox box(window());
    box.setWindowTitle(tr("Permission request"));
    box.setIconPixmap(QIcon(":/kepler/icons/shield.svg").pixmap(40, 40));
    box.setText(tr("<b>%1</b> wants to %2.").arg(origin.host().toHtmlEscaped(), what));
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.button(QMessageBox::Yes)->setText(tr("Allow"));
    box.button(QMessageBox::No)->setText(tr("Block"));
    box.setDefaultButton(QMessageBox::No);
    const bool allow = box.exec() == QMessageBox::Yes;
    page()->setFeaturePermission(origin, feature,
                                 allow ? QWebEnginePage::PermissionGrantedByUser
                                       : QWebEnginePage::PermissionDeniedByUser);
}
QT_WARNING_POP

void WebView::openDevTools()
{
    if (!m_devTools) {
        m_devTools = new QWebEngineView;
        m_devTools->setAttribute(Qt::WA_DeleteOnClose);
        auto *devPage = new QWebEnginePage(page()->profile(), m_devTools);
        m_devTools->setPage(devPage);
        page()->setDevToolsPage(devPage);
        m_devTools->setWindowTitle(tr("Kepler DevTools"));
        m_devTools->resize(1000, 700);
    }
    m_devTools->show();
    m_devTools->raise();
    m_devTools->activateWindow();
}
