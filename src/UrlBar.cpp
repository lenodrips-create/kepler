#include "UrlBar.h"

#include "WebView.h"

#include <QAbstractItemView>
#include <QCompleter>
#include <QFocusEvent>
#include <QIcon>
#include <QKeyEvent>
#include <QStringListModel>
#include <QTimer>
#include <QUrl>

UrlBar::UrlBar(QWidget *parent) : QLineEdit(parent)
{
    setObjectName("UrlBar");
    setPlaceholderText(tr("Search the cosmos or enter an address"));
    setClearButtonEnabled(false);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    m_security = addAction(QIcon(":/kepler/icons/search.svg"), QLineEdit::LeadingPosition);

    m_model = new QStringListModel(this);
    m_completer = new QCompleter(m_model, this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchContains);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setMaxVisibleItems(8);
    setCompleter(m_completer);

    connect(this, &QLineEdit::returnPressed, this, [this] {
        if (!m_completer->popup()->isVisible())
            emit navigateRequested(text());
    });
    connect(m_completer, qOverload<const QString &>(&QCompleter::activated), this,
            [this](const QString &s) { emit navigateRequested(s); });
}

void UrlBar::setUrl(const QUrl &url)
{
    if (WebView::isStartPage(url) || url.isEmpty()) {
        setText(QString());
        m_security->setIcon(QIcon(":/kepler/icons/search.svg"));
        m_security->setToolTip(QString());
    } else {
        setText(url.toDisplayString());
        setCursorPosition(0);
        const QString scheme = url.scheme();
        if (scheme == QLatin1String("https")) {
            m_security->setIcon(QIcon(":/kepler/icons/lock.svg"));
            m_security->setToolTip(tr("Secure connection"));
        } else if (scheme == QLatin1String("kepler")) {
            m_security->setIcon(QIcon(":/kepler/icons/planet.svg"));
            m_security->setToolTip(tr("Kepler page"));
        } else if (scheme == QLatin1String("http")) {
            m_security->setIcon(QIcon(":/kepler/icons/warning.svg"));
            m_security->setToolTip(tr("Not secure"));
        } else {
            m_security->setIcon(QIcon(":/kepler/icons/globe.svg"));
            m_security->setToolTip(QString());
        }
    }
    setModified(false);
}

void UrlBar::setCompletions(const QStringList &items)
{
    if (m_completer->popup()->isVisible())
        return; // don't yank the list out from under the user
    m_model->setStringList(items);
}

void UrlBar::focusInEvent(QFocusEvent *event)
{
    QLineEdit::focusInEvent(event);
    if (event->reason() != Qt::PopupFocusReason)
        QTimer::singleShot(0, this, &QLineEdit::selectAll);
}

void UrlBar::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && !m_completer->popup()->isVisible()) {
        emit escapePressed();
        return;
    }
    QLineEdit::keyPressEvent(event);
}
