#include "BrowserWindow.h"

#include "Browser.h"
#include "AiPanel.h"
#include "DownloadsPanel.h"
#include "Favicons.h"
#include "FindBar.h"
#include "MacWindow.h"
#include "Shields.h"
#include "Storage.h"
#include "Theme.h"
#include "UrlBar.h"
#include "WebView.h"

#include <iterator>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProgressBar>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTabBar>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>
#include <QWebEngineCookieStore>
#include <QWebEngineDownloadRequest>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QtWebEngineCore/qtwebenginecoreglobal.h>

namespace {

constexpr int MaxClosedTabs = 25;
const qreal ZoomSteps[] = {0.25, 0.33, 0.5, 0.67, 0.75, 0.8, 0.9, 1.0,
                           1.1, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0, 4.0, 5.0};

QIcon icon(const char *name)
{
    return QIcon(QStringLiteral(":/kepler/icons/%1.svg").arg(QLatin1String(name)));
}

QToolButton *toolButton(QWidget *parent, const char *iconName, const QString &tip)
{
    auto *b = new QToolButton(parent);
    b->setIcon(icon(iconName));
    b->setIconSize(QSize(18, 18));
    b->setToolTip(tip);
    b->setAutoRaise(true);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

bool isWebUrl(const QUrl &url)
{
    return url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https");
}

} // namespace

BrowserWindow::BrowserWindow(bool orbit)
    : m_orbit(orbit),
      m_profile(orbit ? Browser::instance().createOrbitProfile(this) : Browser::instance().profile())
{
    setAttribute(Qt::WA_DeleteOnClose);
    resize(1320, 860);
    buildUi();
    buildActions();
    rebuildBookmarksBar();

    if (m_orbit)
        setStyleSheet(Theme::orbitStyleSheet());

    auto &storage = Storage::instance();
    m_urlBar->setCompletions(storage.completions());
    connect(&storage, &Storage::historyChanged, this,
            [this] { m_urlBar->setCompletions(Storage::instance().completions()); });
    connect(&storage, &Storage::bookmarksChanged, this, [this] {
        rebuildBookmarksBar();
        updateBookmarkStar();
        m_urlBar->setCompletions(Storage::instance().completions());
    });
    connect(&storage, &Storage::settingsChanged, this, [this] {
        m_bookmarksBar->setVisible(Storage::instance().bookmarksBarVisible());
        updateShields();
    });
    connect(&Favicons::instance(), &Favicons::iconReady, this, [this] {
        // Several logos can land at once; rebuild once they have.
        QTimer::singleShot(150, this, &BrowserWindow::rebuildBookmarksBar);
    });
    connect(Shields::instance(), &Shields::blocked, this, [this](const QString &site) {
        if (WebView *v = currentView(); v && v->url().host().toLower() == site)
            updateShields();
    }, Qt::QueuedConnection);

    auto *downloads = Browser::instance().downloads();
    connect(downloads, &DownloadsPanel::activeCountChanged, this, &BrowserWindow::updateDownloadsButton);
    updateDownloadsButton(downloads->activeCount());
}

BrowserWindow::~BrowserWindow()
{
    // Every page (including tabs already closed but not yet deleted) must go
    // before an Orbit profile is destroyed.
    const auto views = findChildren<WebView *>();
    qDeleteAll(views);
    if (m_orbit)
        delete m_profile;
}

// ------------------------------------------------------------------------ UI

void BrowserWindow::buildUi()
{
    auto *central = new QWidget(this);
    central->setObjectName("Central");
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // --- top bar: logo, tabs, new-tab button
    m_topBar = new QWidget(central);
    m_topBar->setObjectName("TopBar");
    auto *top = new QHBoxLayout(m_topBar);
    m_topLayout = top;
    top->setContentsMargins(6, 0, 10, 0);
    m_topBar->installEventFilter(this);
    top->setSpacing(2);

    auto *brand = new QLabel(m_topBar);
    brand->setObjectName("Brand");
    const qreal dpr = devicePixelRatioF();
    QPixmap logo = QPixmap(":/kepler/web/logo.png")
                       .scaled(QSize(26, 26) * dpr, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    logo.setDevicePixelRatio(dpr);
    brand->setPixmap(logo);
    brand->setToolTip(QStringLiteral("Kepler"));

    m_tabBar = new QTabBar(m_topBar);
    m_tabBar->setTabsClosable(true);
    m_tabBar->setMovable(true);
    m_tabBar->setDocumentMode(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setElideMode(Qt::ElideRight);
    m_tabBar->setUsesScrollButtons(true);
    m_tabBar->setIconSize(QSize(16, 16));
    m_tabBar->setSelectionBehaviorOnRemove(QTabBar::SelectRightTab);
    m_tabBar->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tabBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_tabBar->installEventFilter(this);

    auto *plus = toolButton(m_topBar, "plus", tr("New tab (Ctrl+T)"));
    plus->setObjectName("NewTabButton");
    connect(plus, &QToolButton::clicked, this, [this] { newTab(); });

    top->addWidget(brand, 0, Qt::AlignVCenter);
    top->addWidget(m_tabBar, 0, Qt::AlignBottom);
    top->addWidget(plus, 0, Qt::AlignVCenter);
    top->addStretch(1);

    if (m_orbit) {
        auto *badge = new QLabel(tr("ORBIT"), m_topBar);
        badge->setObjectName("OrbitBadge");
        badge->setToolTip(tr("Private window: history, cookies and site data are forgotten when it closes"));
        top->addWidget(badge, 0, Qt::AlignVCenter);
    }

    // --- nav bar: navigation, omnibox, tools
    m_navBar = new QWidget(central);
    m_navBar->setObjectName("NavBar");
    auto *navCol = new QVBoxLayout(m_navBar);
    navCol->setContentsMargins(0, 0, 0, 0);
    navCol->setSpacing(0);
    auto *nav = new QHBoxLayout;
    nav->setContentsMargins(8, 6, 8, 6);
    nav->setSpacing(4);

    m_back = toolButton(m_navBar, "back", tr("Back (Alt+Left)"));
    m_forward = toolButton(m_navBar, "forward", tr("Forward (Alt+Right)"));
    m_reload = toolButton(m_navBar, "reload", tr("Reload (Ctrl+R)"));
    auto *home = toolButton(m_navBar, "home", tr("Home"));

    m_urlBar = new UrlBar(m_navBar);
    m_starAction = m_urlBar->addAction(icon("star"), QLineEdit::TrailingPosition);
    m_starAction->setToolTip(tr("Bookmark this page (Ctrl+D)"));

    m_zoom = new QToolButton(m_navBar);
    m_zoom->setObjectName("ZoomButton");
    m_zoom->setToolTip(tr("Reset zoom (Ctrl+0)"));
    m_zoom->hide();

    m_shields = toolButton(m_navBar, "shield", tr("Shields"));
    m_shields->setObjectName("ShieldButton");
    m_shields->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_aiButton = toolButton(m_navBar, "sparkle", tr("AI assistants (Ctrl+Shift+Space)"));
    m_aiButton->setObjectName("AiButton");
    m_aiButton->setText(tr("AI"));
    m_aiButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_aiButton->setCheckable(true);
    m_downloadsButton = toolButton(m_navBar, "download", tr("Downloads (Ctrl+J)"));
    auto *menuButton = toolButton(m_navBar, "menu", tr("Menu"));
    menuButton->setPopupMode(QToolButton::InstantPopup);
    menuButton->setMenu(buildMainMenu());

    nav->addWidget(m_back);
    nav->addWidget(m_forward);
    nav->addWidget(m_reload);
    nav->addWidget(home);
    nav->addSpacing(4);
    nav->addWidget(m_urlBar, 1);
    nav->addWidget(m_zoom);
    nav->addSpacing(4);
    nav->addWidget(m_aiButton);
    nav->addWidget(m_shields);
    nav->addWidget(m_downloadsButton);
    nav->addWidget(menuButton);
    navCol->addLayout(nav);

    m_bookmarksBar = new QToolBar(m_navBar);
    m_bookmarksBar->setObjectName("BookmarksBar");
    m_bookmarksBar->setIconSize(QSize(14, 14));
    m_bookmarksBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_bookmarksBar->setContextMenuPolicy(Qt::PreventContextMenu);
    m_bookmarksBar->setVisible(Storage::instance().bookmarksBarVisible());
    navCol->addWidget(m_bookmarksBar);

    m_progress = new QProgressBar(central);
    m_progress->setObjectName("LoadBar");
    m_progress->setTextVisible(false);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);

    // --- content: stacked web views with floating overlays
    m_content = new QWidget(central);
    auto *contentLayout = new QVBoxLayout(m_content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    m_stack = new QStackedWidget(m_content);
    contentLayout->addWidget(m_stack);
    m_content->installEventFilter(this);

    m_statusBubble = new QLabel(m_content);
    m_statusBubble->setObjectName("StatusBubble");
    m_statusBubble->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_statusBubble->hide();

    m_findBar = new FindBar(m_content);
    m_ai = new AiPanel(m_profile, this, m_content);

    root->addWidget(m_topBar);
    root->addWidget(m_navBar);
    root->addWidget(m_progress);
    root->addWidget(m_content, 1);
    setCentralWidget(central);

    // --- wiring
    connect(m_tabBar, &QTabBar::currentChanged, this, &BrowserWindow::onCurrentTabChanged);
    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &BrowserWindow::closeTab);
    connect(m_tabBar, &QTabBar::tabMoved, this, &BrowserWindow::onTabMoved);
    connect(m_tabBar, &QTabBar::customContextMenuRequested, this, &BrowserWindow::showTabMenu);

    connect(m_back, &QToolButton::clicked, this, [this] { if (auto *v = currentView()) v->back(); });
    connect(m_forward, &QToolButton::clicked, this, [this] { if (auto *v = currentView()) v->forward(); });
    connect(m_reload, &QToolButton::clicked, this, [this] {
        if (auto *v = currentView())
            v->isLoading() ? v->stop() : v->reload();
    });
    connect(home, &QToolButton::clicked, this, [this] {
        if (auto *v = currentView())
            v->load(Browser::startUrl());
    });
    connect(m_urlBar, &UrlBar::navigateRequested, this, &BrowserWindow::navigate);
    connect(m_urlBar, &UrlBar::escapePressed, this, [this] {
        updateUrlBar();
        if (auto *v = currentView())
            v->setFocus();
    });
    connect(m_starAction, &QAction::triggered, this, [this] {
        if (auto *v = currentView(); v && !WebView::isStartPage(v->url()))
            Storage::instance().toggleBookmark(v->url(), v->title());
    });
    connect(m_zoom, &QToolButton::clicked, this, [this] { setZoom(1.0); });
    connect(m_shields, &QToolButton::clicked, this, &BrowserWindow::showShieldsMenu);
    connect(m_downloadsButton, &QToolButton::clicked, this, &BrowserWindow::showDownloads);
    connect(m_aiButton, &QToolButton::clicked, m_ai, &AiPanel::toggle);
    connect(m_ai, &AiPanel::openChanged, m_aiButton, &QToolButton::setChecked);
}

void BrowserWindow::buildActions()
{
    auto add = [this](QList<QKeySequence> keys, auto &&fn) {
        auto *a = new QAction(this);
        a->setShortcuts(keys);
        a->setShortcutContext(Qt::WindowShortcut);
        connect(a, &QAction::triggered, this, fn);
        addAction(a);
        return a;
    };

    add({QKeySequence::AddTab}, [this] { newTab(); });
    add({QKeySequence(Qt::CTRL | Qt::Key_W), QKeySequence(Qt::CTRL | Qt::Key_F4)},
        [this] { closeTab(m_tabBar->currentIndex()); });
    add({QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T)}, [this] { reopenClosedTab(); });
    add({QKeySequence(Qt::CTRL | Qt::Key_Tab), QKeySequence(Qt::CTRL | Qt::Key_PageDown)}, [this] {
        m_tabBar->setCurrentIndex((m_tabBar->currentIndex() + 1) % m_tabBar->count());
    });
    add({QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab), QKeySequence(Qt::CTRL | Qt::Key_PageUp)}, [this] {
        m_tabBar->setCurrentIndex((m_tabBar->currentIndex() - 1 + m_tabBar->count()) % m_tabBar->count());
    });
    for (int i = 1; i <= 9; ++i) {
        add({QKeySequence(Qt::CTRL | Qt::Key(Qt::Key_0 + i))}, [this, i] {
            m_tabBar->setCurrentIndex(i == 9 ? m_tabBar->count() - 1 : qMin(i - 1, m_tabBar->count() - 1));
        });
    }
    add({QKeySequence(Qt::CTRL | Qt::Key_L), QKeySequence(Qt::ALT | Qt::Key_D), QKeySequence(Qt::Key_F6)}, [this] {
        m_urlBar->setFocus(Qt::ShortcutFocusReason);
        m_urlBar->selectAll();
    });
    add({QKeySequence::Back}, [this] { if (auto *v = currentView()) v->back(); });
    add({QKeySequence::Forward}, [this] { if (auto *v = currentView()) v->forward(); });
    add({QKeySequence(Qt::CTRL | Qt::Key_R), QKeySequence(Qt::Key_F5)},
        [this] { if (auto *v = currentView()) v->reload(); });
    add({QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R), QKeySequence(Qt::SHIFT | Qt::Key_F5)}, [this] {
        if (auto *v = currentView())
            v->triggerPageAction(QWebEnginePage::ReloadAndBypassCache);
    });
    add({QKeySequence(Qt::ALT | Qt::Key_Home)}, [this] { if (auto *v = currentView()) v->load(Browser::startUrl()); });
    add({QKeySequence(Qt::CTRL | Qt::Key_D)}, [this] { m_starAction->trigger(); });
    add({QKeySequence::FindNext}, [this] { m_findBar->findNext(); });
    add({QKeySequence::FindPrevious}, [this] { m_findBar->findPrevious(); });
    add({QKeySequence(Qt::Key_F11)}, [this] {
        if (isFullScreen()) {
            m_wasMaximized ? showMaximized() : showNormal();
        } else {
            m_wasMaximized = isMaximized();
            showFullScreen();
        }
    });
    add({QKeySequence(Qt::CTRL | Qt::Key_Plus), QKeySequence(Qt::CTRL | Qt::Key_Equal)}, [this] {
        if (auto *v = currentView())
            for (qreal z : ZoomSteps)
                if (z > v->zoomFactor() + 0.001) { setZoom(z); break; }
    });
    add({QKeySequence(Qt::CTRL | Qt::Key_Minus)}, [this] {
        if (auto *v = currentView())
            for (int i = int(std::size(ZoomSteps)) - 1; i >= 0; --i)
                if (ZoomSteps[i] < v->zoomFactor() - 0.001) { setZoom(ZoomSteps[i]); break; }
    });
    add({QKeySequence(Qt::CTRL | Qt::Key_0)}, [this] { setZoom(1.0); });
    add({QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Space)}, [this] { m_ai->toggle(); });
}

QMenu *BrowserWindow::buildMainMenu()
{
    auto *menu = new QMenu(this);

    auto item = [this, menu](const char *iconName, const QString &text, const QKeySequence &key, auto &&fn) {
        QAction *a = iconName ? menu->addAction(icon(iconName), text) : menu->addAction(text);
        if (!key.isEmpty()) {
            a->setShortcut(key);
            a->setShortcutContext(Qt::WindowShortcut);
            addAction(a); // keep the shortcut alive when the menu is closed
        }
        connect(a, &QAction::triggered, this, fn);
        return a;
    };

    item("plus", tr("New tab"), {}, [this] { newTab(); });
    item(nullptr, tr("New window"), QKeySequence(Qt::CTRL | Qt::Key_N), [] {
        auto *w = Browser::instance().createWindow(false);
        w->newTab();
        w->show();
    });
    item("orbit", tr("New Orbit window (private)"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), [] {
        auto *w = Browser::instance().createWindow(true);
        w->newTab();
        w->show();
    });
    menu->addSeparator();
    item("clock", tr("History"), QKeySequence(Qt::CTRL | Qt::Key_H), [this] { newTab(Browser::historyUrl()); });
    item("download", tr("Downloads"), QKeySequence(Qt::CTRL | Qt::Key_J), [this] { showDownloads(); });
    auto *barToggle = item("bookmark", tr("Show bookmarks bar"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B), [] {
        auto &s = Storage::instance();
        s.setBookmarksBarVisible(!s.bookmarksBarVisible());
    });
    barToggle->setCheckable(true);
    barToggle->setChecked(Storage::instance().bookmarksBarVisible());
    connect(&Storage::instance(), &Storage::settingsChanged, barToggle,
            [barToggle] { barToggle->setChecked(Storage::instance().bookmarksBarVisible()); });
    menu->addSeparator();
    item("search", tr("Find in page"), QKeySequence::Find, [this] { m_findBar->open(); positionOverlays(); });
    item(nullptr, tr("Save page as..."), QKeySequence::Save, [this] { savePage(); });
    item(nullptr, tr("Save as PDF..."), QKeySequence::Print, [this] { savePdf(); });
    item(nullptr, tr("View page source"), QKeySequence(Qt::CTRL | Qt::Key_U), [this] {
        if (auto *v = currentView(); v && isWebUrl(v->url()))
            newTab(QUrl(QStringLiteral("view-source:") + v->url().toString()));
    });
    item(nullptr, tr("Developer tools"), QKeySequence(Qt::Key_F12), [this] {
        if (auto *v = currentView())
            v->openDevTools();
    });
    menu->addSeparator();

    auto *engines = menu->addMenu(icon("search"), tr("Search engine"));
    auto *group = new QActionGroup(engines);
    for (const QString &name : Storage::searchEngineNames()) {
        auto *a = engines->addAction(name);
        a->setCheckable(true);
        a->setChecked(name == Storage::instance().searchEngine());
        group->addAction(a);
        connect(a, &QAction::triggered, this, [name] { Storage::instance().setSearchEngine(name); });
    }
    connect(engines, &QMenu::aboutToShow, engines, [engines] {
        for (QAction *a : engines->actions())
            a->setChecked(a->text() == Storage::instance().searchEngine());
    });

    item(nullptr, tr("Clear browsing data..."), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Delete),
         [this] { clearBrowsingData(); });
    menu->addSeparator();
    item("planet", tr("About Kepler"), {}, [this] { showAbout(); });
    item(nullptr, tr("Quit"), QKeySequence(Qt::CTRL | Qt::Key_Q), [] { QApplication::closeAllWindows(); });
    return menu;
}

void BrowserWindow::rebuildBookmarksBar()
{
    m_bookmarksBar->clear();
    const auto &bookmarks = Storage::instance().bookmarks();
    if (bookmarks.isEmpty()) {
        auto *hint = m_bookmarksBar->addAction(tr("Press Ctrl+D to bookmark a page"));
        hint->setEnabled(false);
        return;
    }
    for (const Bookmark &b : bookmarks) {
        const QString title = b.title.isEmpty() ? QUrl(b.url).host() : b.title;
        auto *a = m_bookmarksBar->addAction(Favicons::instance().icon(QUrl(b.url)),
                                            fontMetrics().elidedText(title, Qt::ElideRight, 160));
        a->setToolTip(b.url);
        const QUrl url(b.url);
        connect(a, &QAction::triggered, this, [this, url] {
            if (QApplication::keyboardModifiers() & Qt::ControlModifier)
                newTab(url, false);
            else if (auto *v = currentView())
                v->load(url);
        });
        if (auto *button = m_bookmarksBar->widgetForAction(a)) {
            button->setContextMenuPolicy(Qt::CustomContextMenu);
            button->setCursor(Qt::PointingHandCursor);
            const QString raw = b.url;
            connect(button, &QWidget::customContextMenuRequested, this, [this, button, url, raw](const QPoint &p) {
                QMenu m(this);
                m.addAction(tr("Open in new tab"), this, [this, url] { newTab(url); });
                m.addAction(tr("Open in Orbit window"), this, [url] {
                    auto *w = Browser::instance().createWindow(true);
                    w->newTab(url);
                    w->show();
                });
                m.addSeparator();
                m.addAction(tr("Remove bookmark"), this, [raw] { Storage::instance().removeBookmark(raw); });
                m.exec(button->mapToGlobal(p));
            });
        }
    }
}

// ---------------------------------------------------------------------- tabs

WebView *BrowserWindow::newTab(const QUrl &url, bool makeCurrent)
{
    WebView *view = createTab(makeCurrent, false);
    view->load(url.isEmpty() ? Browser::startUrl() : url);
    if (makeCurrent && url.isEmpty()) {
        m_urlBar->setFocus(Qt::ShortcutFocusReason);
        m_urlBar->clear();
    }
    return view;
}

WebView *BrowserWindow::createTab(bool makeCurrent, bool nextToCurrent)
{
    auto *view = new WebView(m_profile, this);
    const int index = nextToCurrent && m_tabBar->count() ? m_tabBar->currentIndex() + 1 : m_tabBar->count();

    m_stack->insertWidget(index, view);
    m_tabBar->insertTab(index, icon("globe"), tr("New Tab"));

    connect(view, &QWebEngineView::titleChanged, this, [this, view](const QString &title) {
        const int i = indexOf(view);
        if (i < 0)
            return;
        const QString text = title.isEmpty() || title == view->url().toString() ? tr("New Tab") : title;
        m_tabBar->setTabText(i, text);
        m_tabBar->setTabToolTip(i, title);
        if (view == currentView())
            updateTitle();
        if (!m_orbit && isWebUrl(view->url()) && !title.isEmpty())
            Storage::instance().updateHistoryTitle(view->url(), title);
    });
    connect(view, &QWebEngineView::iconChanged, this, [this, view] { updateTabIcon(view); });
    connect(view, &QWebEngineView::urlChanged, this, [this, view] {
        updateTabIcon(view);
        if (view == currentView()) {
            updateUrlBar();
            updateNavigation();
            updateBookmarkStar();
            updateShields();
        }
    });
    connect(view, &QWebEngineView::loadStarted, this, [this, view] {
        if (view == currentView()) {
            m_progress->setValue(5);
            updateNavigation();
        }
    });
    connect(view, &QWebEngineView::loadProgress, this, [this, view](int p) {
        if (view == currentView())
            m_progress->setValue(p);
    });
    connect(view, &QWebEngineView::loadFinished, this, [this, view](bool ok) {
        if (view == currentView()) {
            m_progress->setValue(0);
            updateNavigation();
            updateZoom();
        }
        if (ok && !m_orbit && isWebUrl(view->url()))
            Storage::instance().addHistory(view->url(), view->title());
    });

    QWebEnginePage *page = view->page();
    connect(page, &QWebEnginePage::linkHovered, this, [this, view](const QString &url) {
        if (view != currentView())
            return;
        if (url.isEmpty()) {
            m_statusBubble->hide();
            return;
        }
        m_statusBubble->setText(m_statusBubble->fontMetrics().elidedText(url, Qt::ElideMiddle, 520));
        m_statusBubble->adjustSize();
        positionOverlays();
        m_statusBubble->show();
        m_statusBubble->raise();
    });
    connect(page, &QWebEnginePage::fullScreenRequested, this, &BrowserWindow::handleFullScreen);
    connect(page, &QWebEnginePage::windowCloseRequested, this, [this, view] { closeTab(indexOf(view)); });
    connect(page, &QWebEnginePage::recentlyAudibleChanged, this, [this, view] { updateTabIcon(view); });
    connect(page, &QWebEnginePage::audioMutedChanged, this, [this, view] { updateTabIcon(view); });

    if (makeCurrent)
        m_tabBar->setCurrentIndex(index);
    return view;
}

WebView *BrowserWindow::currentView() const
{
    return qobject_cast<WebView *>(m_stack->currentWidget());
}

WebView *BrowserWindow::viewAt(int index) const
{
    return qobject_cast<WebView *>(m_stack->widget(index));
}

int BrowserWindow::indexOf(WebView *view) const
{
    return m_stack->indexOf(view);
}

void BrowserWindow::closeTab(int index)
{
    WebView *view = viewAt(index);
    if (!view)
        return;

    const QUrl url = view->url();
    if (!url.isEmpty() && !WebView::isStartPage(url)) {
        m_closedTabs.append(url);
        if (m_closedTabs.size() > MaxClosedTabs)
            m_closedTabs.removeFirst();
    }

    if (m_tabBar->count() == 1) {
        close();
        return;
    }
    // Remove from the stack first so the tab bar's currentChanged sees a consistent state.
    m_stack->removeWidget(view);
    m_tabBar->removeTab(index);
    view->deleteLater();
}

void BrowserWindow::reopenClosedTab()
{
    if (!m_closedTabs.isEmpty())
        newTab(m_closedTabs.takeLast());
}

void BrowserWindow::onCurrentTabChanged(int index)
{
    if (index < 0 || index >= m_stack->count())
        return;
    m_stack->setCurrentIndex(index);
    WebView *view = currentView();

    updateUrlBar();
    updateNavigation();
    updateTitle();
    updateBookmarkStar();
    updateZoom();
    updateShields();
    m_statusBubble->hide();
    m_findBar->setView(view);
    m_progress->setValue(view->isLoading() ? view->progress() : 0);

    if (view->url().isEmpty() || WebView::isStartPage(view->url()))
        m_urlBar->setFocus(Qt::OtherFocusReason);
    else
        view->setFocus();
}

void BrowserWindow::onTabMoved(int from, int to)
{
    QWidget *w = m_stack->widget(from);
    m_stack->removeWidget(w);
    m_stack->insertWidget(to, w);
    m_stack->setCurrentIndex(m_tabBar->currentIndex());
}

void BrowserWindow::showTabMenu(const QPoint &pos)
{
    const int index = m_tabBar->tabAt(pos);
    QMenu menu(this);
    menu.addAction(icon("plus"), tr("New tab"), this, [this] { newTab(); });
    if (WebView *view = viewAt(index)) {
        menu.addSeparator();
        menu.addAction(icon("reload"), tr("Reload"), view, [view] { view->reload(); });
        menu.addAction(tr("Duplicate"), this, [this, view] { newTab(view->url()); });
        const bool muted = view->page()->isAudioMuted();
        menu.addAction(icon(muted ? "speaker" : "speaker-muted"), muted ? tr("Unmute site") : tr("Mute site"),
                       view, [view, muted] { view->page()->setAudioMuted(!muted); });
        menu.addSeparator();
        menu.addAction(icon("stop"), tr("Close tab"), this, [this, view] { closeTab(indexOf(view)); });
        menu.addAction(tr("Close other tabs"), this, [this, view] {
            for (int i = m_tabBar->count() - 1; i >= 0; --i)
                if (viewAt(i) != view)
                    closeTab(i);
        });
    }
    auto *reopen = menu.addAction(tr("Reopen closed tab"), this, [this] { reopenClosedTab(); });
    reopen->setEnabled(!m_closedTabs.isEmpty());
    menu.exec(m_tabBar->mapToGlobal(pos));
}

void BrowserWindow::updateTabIcon(WebView *view)
{
    const int i = indexOf(view);
    if (i < 0)
        return;
    QIcon tabIcon;
    if (view->page()->isAudioMuted())
        tabIcon = icon("speaker-muted");
    else if (view->page()->recentlyAudible())
        tabIcon = icon("speaker");
    else if (WebView::isStartPage(view->url()) || view->url().scheme() == QLatin1String("kepler"))
        tabIcon = icon("planet");
    else if (!view->icon().isNull())
        tabIcon = view->icon();
    else
        tabIcon = icon("globe");
    m_tabBar->setTabIcon(i, tabIcon);
}

// --------------------------------------------------------------- navigation

void BrowserWindow::navigate(const QString &input)
{
    const QUrl url = Browser::urlFromInput(input);
    WebView *view = currentView();
    if (!view || url.isEmpty())
        return;
    view->load(url);
    view->setFocus();
}

void BrowserWindow::updateNavigation()
{
    WebView *view = currentView();
    if (!view)
        return;
    m_back->setEnabled(view->history()->canGoBack());
    m_forward->setEnabled(view->history()->canGoForward());
    if (view->isLoading()) {
        m_reload->setIcon(icon("stop"));
        m_reload->setToolTip(tr("Stop loading"));
    } else {
        m_reload->setIcon(icon("reload"));
        m_reload->setToolTip(tr("Reload (Ctrl+R)"));
    }
}

void BrowserWindow::updateUrlBar()
{
    WebView *view = currentView();
    if (!view)
        return;
    // Don't clobber what the user is typing.
    if (m_urlBar->hasFocus() && m_urlBar->isModified())
        return;
    m_urlBar->setUrl(view->url());
}

void BrowserWindow::updateTitle()
{
    WebView *view = currentView();
    const QString app = m_orbit ? tr("Kepler Orbit") : QStringLiteral("Kepler");
    const QString title = view ? view->title() : QString();
    if (!view || title.isEmpty() || WebView::isStartPage(view->url()))
        setWindowTitle(app);
    else
        setWindowTitle(QStringLiteral("%1 — %2").arg(title, app));
}

void BrowserWindow::updateBookmarkStar()
{
    WebView *view = currentView();
    const bool canBookmark = view && !view->url().isEmpty() && view->url().scheme() != QLatin1String("kepler");
    m_starAction->setVisible(canBookmark);
    if (!canBookmark)
        return;
    const bool marked = Storage::instance().isBookmarked(view->url());
    m_starAction->setIcon(icon(marked ? "star-filled" : "star"));
    m_starAction->setToolTip(marked ? tr("Remove bookmark (Ctrl+D)") : tr("Bookmark this page (Ctrl+D)"));
}

void BrowserWindow::setZoom(qreal factor)
{
    if (WebView *view = currentView()) {
        view->setZoomFactor(factor);
        updateZoom();
    }
}

void BrowserWindow::updateZoom()
{
    WebView *view = currentView();
    if (!view)
        return;
    const int percent = qRound(view->zoomFactor() * 100);
    m_zoom->setVisible(percent != 100);
    m_zoom->setText(QStringLiteral("%1%").arg(percent));
}

void BrowserWindow::updateShields()
{
    WebView *view = currentView();
    const bool on = Storage::instance().shieldsEnabled();
    m_shields->setIcon(icon(on ? "shield" : "shield-off"));
    const int count = view && on ? Shields::instance()->blockedOn(view->url().host()) : 0;
    m_shields->setText(count > 0 ? QString::number(count) : QString());
    m_shields->setToolTip(on ? tr("Shields up: %n tracker(s) blocked on this site", nullptr, count)
                             : tr("Shields are down"));
}

void BrowserWindow::showShieldsMenu()
{
    WebView *view = currentView();
    auto &storage = Storage::instance();
    const QString host = view ? view->url().host() : QString();

    QMenu menu(this);
    auto *header = menu.addAction(icon(storage.shieldsEnabled() ? "shield" : "shield-off"),
                                  host.isEmpty() ? tr("Kepler Shields") : host);
    header->setEnabled(false);
    menu.addSeparator();
    auto *here = menu.addAction(tr("%n tracker(s) blocked on this site", nullptr,
                                   Shields::instance()->blockedOn(host)));
    here->setEnabled(false);
    auto *total = menu.addAction(tr("%n blocked this session", nullptr, Shields::instance()->totalBlocked()));
    total->setEnabled(false);
    menu.addSeparator();
    auto *toggle = menu.addAction(tr("Shields up"));
    toggle->setCheckable(true);
    toggle->setChecked(storage.shieldsEnabled());
    connect(toggle, &QAction::toggled, this, [this](bool on) {
        Storage::instance().setShieldsEnabled(on);
        if (auto *v = currentView())
            v->reload();
    });
    menu.exec(m_shields->mapToGlobal(QPoint(0, m_shields->height() + 4)));
}

void BrowserWindow::showDownloads()
{
    Browser::instance().downloads()->popup(m_downloadsButton);
}

void BrowserWindow::updateDownloadsButton(int active)
{
    m_downloadsButton->setIcon(icon(active > 0 ? "download-active" : "download"));
    m_downloadsButton->setToolTip(active > 0 ? tr("%n download(s) in progress", nullptr, active)
                                             : tr("Downloads (Ctrl+J)"));
}

void BrowserWindow::handleFullScreen(QWebEngineFullScreenRequest request)
{
    request.accept();
    const bool on = request.toggleOn();
    if (on)
        m_ai->hidePanel();
    m_topBar->setVisible(!on);
    m_navBar->setVisible(!on);
    m_progress->setVisible(!on);
    if (on) {
        m_wasMaximized = isMaximized();
        showFullScreen();
    } else {
        m_wasMaximized ? showMaximized() : showNormal();
    }
}

void BrowserWindow::positionOverlays()
{
    const QRect r = m_content->rect();
    if (m_statusBubble->isVisible() || !m_statusBubble->text().isEmpty())
        m_statusBubble->move(8, r.height() - m_statusBubble->height() - 8);
    m_findBar->adjustSize();
    m_findBar->move(r.width() - m_findBar->width() - 18, 12);
    m_ai->reposition();
}

// ------------------------------------------------------------------- extras

void BrowserWindow::savePage()
{
    WebView *view = currentView();
    if (!view || !isWebUrl(view->url()))
        return;
    QString name = view->title().isEmpty() ? QStringLiteral("page") : view->title();
    name.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("_"));
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const QString path = QFileDialog::getSaveFileName(this, tr("Save page"), dir + '/' + name + ".mhtml",
                                                      tr("Web archive (*.mhtml);;HTML (*.html)"));
    if (path.isEmpty())
        return;
    view->page()->save(path, path.endsWith(".html") ? QWebEngineDownloadRequest::CompleteHtmlSaveFormat
                                                    : QWebEngineDownloadRequest::MimeHtmlSaveFormat);
}

void BrowserWindow::savePdf()
{
    WebView *view = currentView();
    if (!view)
        return;
    QString name = view->title().isEmpty() ? QStringLiteral("page") : view->title();
    name.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("_"));
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getSaveFileName(this, tr("Save as PDF"), dir + '/' + name + ".pdf",
                                                      tr("PDF (*.pdf)"));
    if (!path.isEmpty())
        view->page()->printToPdf(path);
}

void BrowserWindow::clearBrowsingData()
{
    const auto answer = QMessageBox::question(
        this, tr("Clear browsing data"),
        tr("Erase your history, cookies, cache and site data?\nBookmarks are kept."),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (answer != QMessageBox::Yes)
        return;
    QWebEngineProfile *profile = Browser::instance().profile();
    Storage::instance().clearHistory();
    profile->clearHttpCache();
    profile->clearAllVisitedLinks();
    profile->cookieStore()->deleteAllCookies();
}

void BrowserWindow::showAbout()
{
    QMessageBox box(this);
    box.setWindowTitle(tr("About Kepler"));
    const qreal dpr = devicePixelRatioF();
    QPixmap logo = QPixmap(":/kepler/logo-icon.png").scaled(QSize(96, 96) * dpr, Qt::KeepAspectRatio,
                                                           Qt::SmoothTransformation);
    logo.setDevicePixelRatio(dpr);
    box.setIconPixmap(logo);
    box.setText(QStringLiteral("<h2 style='margin:0'>Kepler</h2><p style='color:#8b8ba7'>Version %1</p>")
                    .arg(QStringLiteral(KEPLER_VERSION)));
    box.setInformativeText(
        tr("A web browser from another orbit.<br><br>"
           "Engine: Chromium %1<br>Qt WebEngine %2<br><br>"
           "Shields have blocked <b>%3</b> trackers this session.")
            .arg(QLatin1String(qWebEngineChromiumVersion()), QLatin1String(qWebEngineVersion()))
            .arg(Shields::instance()->totalBlocked()));
    box.exec();
}

QStringList BrowserWindow::sessionUrls() const
{
    QStringList urls;
    for (int i = 0; i < m_stack->count(); ++i) {
        const QUrl url = viewAt(i)->url();
        if (!url.isEmpty() && !WebView::isStartPage(url))
            urls << url.toString();
    }
    return urls;
}

// ------------------------------------------------------------------- events

void BrowserWindow::closeEvent(QCloseEvent *event)
{
    Browser::instance().downloads()->hide();
    if (!m_orbit) {
        // The last regular window to close decides what comes back next launch.
        bool othersOpen = false;
        for (BrowserWindow *w : Browser::instance().windows())
            if (w != this && !w->isOrbit() && w->isVisible())
                othersOpen = true;
        if (!othersOpen)
            Storage::instance().saveSession(sessionUrls());
    }
    event->accept();
}

bool BrowserWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_content && event->type() == QEvent::Resize) {
        positionOverlays();
#ifdef Q_OS_MACOS
    } else if ((watched == m_topBar || watched == m_tabBar) && event->type() == QEvent::MouseButtonPress) {
        // The title bar is gone on Mac, so the top bar's empty space drags the window.
        auto *me = static_cast<QMouseEvent *>(event);
        const bool emptySpot = watched == m_topBar || m_tabBar->tabAt(me->position().toPoint()) < 0;
        if (me->button() == Qt::LeftButton && emptySpot && windowHandle()) {
            windowHandle()->startSystemMove();
            return true;
        }
    } else if (watched == m_topBar && event->type() == QEvent::MouseButtonDblClick) {
        isMaximized() ? showNormal() : showMaximized();
        return true;
#endif
    } else if (watched == m_tabBar && event->type() == QEvent::MouseButtonRelease) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::MiddleButton) {
            const int index = m_tabBar->tabAt(me->position().toPoint());
            if (index >= 0)
                closeTab(index);
            return true;
        }
    } else if (watched == m_tabBar && event->type() == QEvent::MouseButtonDblClick) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (m_tabBar->tabAt(me->position().toPoint()) < 0) {
            newTab();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

// --------------------------------------------------------------- macOS chrome

void BrowserWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
#ifdef Q_OS_MACOS
    if (!m_macStyled) {
        m_macStyled = true;
        MacWindow::setup(this);
        // The red/yellow/green buttons stay exactly where macOS puts them (like
        // Chrome); the tabs move up into the title strip to sit beside them.
        setStyleSheet(styleSheet() + QStringLiteral("QTabBar::tab { margin-top: 0px; }"));
        updateTopBarInset();
    }
#endif
}

void BrowserWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
#ifdef Q_OS_MACOS
    if (event->type() == QEvent::WindowStateChange) {
        // Mac animates into and out of full screen; settle before measuring.
        QTimer::singleShot(400, this, &BrowserWindow::updateTopBarInset);
    }
#endif
}

void BrowserWindow::updateTopBarInset()
{
#ifdef Q_OS_MACOS
    // Leave room for the red/yellow/green buttons, except in full screen where they hide.
    const bool fullScreen = isFullScreen() || MacWindow::isNativeFullScreen(this);
    m_topLayout->setContentsMargins(fullScreen ? 6 : MacWindow::TrafficLightsInset, 0, 10, 0);
#endif
}
