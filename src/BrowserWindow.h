#pragma once

#include <QMainWindow>
#include <QUrl>

class AiPanel;
class FindBar;
class QHBoxLayout;
class QLabel;
class QMenu;
class QProgressBar;
class QStackedWidget;
class QTabBar;
class QToolBar;
class QToolButton;
class QWebEngineFullScreenRequest;
class QWebEngineProfile;
class UrlBar;
class WebView;

class BrowserWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BrowserWindow(bool orbit);
    ~BrowserWindow() override;

    bool isOrbit() const { return m_orbit; }
    QWebEngineProfile *profile() const { return m_profile; }

    // Opens a tab and loads url (the start page when empty).
    WebView *newTab(const QUrl &url = QUrl(), bool makeCurrent = true);
    // Opens an empty tab for the web engine to fill (links, window.open).
    WebView *createTab(bool makeCurrent, bool nextToCurrent);

    WebView *currentView() const;
    void showDownloads();
    QStringList sessionUrls() const;

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void buildUi();
    void buildActions();
    QMenu *buildMainMenu();
    void rebuildBookmarksBar();

    WebView *viewAt(int index) const;
    int indexOf(WebView *view) const;
    void closeTab(int index);
    void reopenClosedTab();
    void onCurrentTabChanged(int index);
    void onTabMoved(int from, int to);
    void showTabMenu(const QPoint &pos);

    void navigate(const QString &input);
    void updateNavigation();
    void updateUrlBar();
    void updateTitle();
    void updateBookmarkStar();
    void updateTabIcon(WebView *view);
    void savePdf();
    void updateZoom();
    void updateShields();
    void updateDownloadsButton(int active);
    void positionOverlays();
    void setZoom(qreal factor);
    void handleFullScreen(QWebEngineFullScreenRequest request);
    void showShieldsMenu();
    void savePage();
    void clearBrowsingData();
    void showAbout();
    void updateTopBarInset();

    bool m_orbit;
    QWebEngineProfile *m_profile;

    QWidget *m_topBar = nullptr;
    QWidget *m_navBar = nullptr;
    QTabBar *m_tabBar = nullptr;
    QStackedWidget *m_stack = nullptr;
    QWidget *m_content = nullptr;
    UrlBar *m_urlBar = nullptr;
    QToolButton *m_back = nullptr;
    QToolButton *m_forward = nullptr;
    QToolButton *m_reload = nullptr;
    QAction *m_starAction = nullptr;
    QToolButton *m_zoom = nullptr;
    QToolButton *m_shields = nullptr;
    QToolButton *m_downloadsButton = nullptr;
    QToolBar *m_bookmarksBar = nullptr;
    QProgressBar *m_progress = nullptr;
    QLabel *m_statusBubble = nullptr;
    FindBar *m_findBar = nullptr;
    AiPanel *m_ai = nullptr;
    QToolButton *m_aiButton = nullptr;
    QHBoxLayout *m_topLayout = nullptr;
    bool m_macStyled = false;

    QList<QUrl> m_closedTabs;
    bool m_wasMaximized = false;
};
