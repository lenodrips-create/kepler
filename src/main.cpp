// Kepler - a web browser from another orbit.

#include "Browser.h"
#include "BrowserWindow.h"
#include "SchemeHandler.h"
#include "Storage.h"
#include "Theme.h"
#include "WebView.h"

#include <QApplication>
#include <QIcon>
#include <QStyleFactory>
#include <QUrl>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName(QStringLiteral("Kepler"));
    QCoreApplication::setApplicationName(QStringLiteral("Kepler"));
    QCoreApplication::setApplicationVersion(QStringLiteral(KEPLER_VERSION));

    // Custom URL schemes have to be known before the web engine starts.
    SchemeHandler::registerScheme();

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/kepler/logo-icon.png")));
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    Theme::applyPalette();
    app.setStyleSheet(Theme::styleSheet());

    // kepler https://example.com other.org   -> opens those
    // kepler --secure                        -> starts in Secure browsing
    // kepler                                 -> restores last session (or the start page)
    const QStringList args = app.arguments().mid(1);
    const bool secure = args.contains(QStringLiteral("--secure"));

    Browser browser;
    BrowserWindow *window = browser.createWindow(secure);

    QStringList urls;
    for (const QString &arg : args)
        if (!arg.startsWith('-'))
            urls << arg;
    if (urls.isEmpty() && !secure)
        urls = Storage::instance().sessionUrls();

    for (const QString &u : urls)
        window->newTab(Browser::urlFromInput(u), false);
    if (urls.isEmpty())
        window->newTab();
    else
        window->currentView()->setFocus();

    window->show();
    return app.exec();
}
