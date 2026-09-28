#include "FindBar.h"

#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QWebEngineFindTextResult>
#include <QWebEnginePage>
#include <QWebEngineView>

FindBar::FindBar(QWidget *parent) : QFrame(parent)
{
    setObjectName("FindBar");

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(30);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(0, 0, 0, 160));
    setGraphicsEffect(shadow);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 6, 6, 6);
    layout->setSpacing(2);

    auto *icon = new QLabel(this);
    icon->setPixmap(QIcon(":/kepler/icons/search.svg").pixmap(16, 16));
    m_edit = new QLineEdit(this);
    m_edit->setPlaceholderText(tr("Find in page"));
    m_count = new QLabel(this);
    m_count->setObjectName("FindCount");

    auto button = [this](const char *icon, const QString &tip) {
        auto *b = new QToolButton(this);
        b->setIcon(QIcon(QString(":/kepler/icons/%1.svg").arg(icon)));
        b->setToolTip(tip);
        b->setIconSize(QSize(16, 16));
        return b;
    };
    auto *prev = button("chevron-up", tr("Previous (Shift+Enter)"));
    auto *next = button("chevron-down", tr("Next (Enter)"));
    auto *close = button("stop", tr("Close (Esc)"));

    layout->addWidget(icon);
    layout->addWidget(m_edit, 1);
    layout->addWidget(m_count);
    layout->addWidget(prev);
    layout->addWidget(next);
    layout->addWidget(close);

    connect(m_edit, &QLineEdit::textChanged, this, [this] { find(false); });
    connect(m_edit, &QLineEdit::returnPressed, this, [this] {
        find(QGuiApplication::keyboardModifiers() & Qt::ShiftModifier);
    });
    connect(prev, &QToolButton::clicked, this, &FindBar::findPrevious);
    connect(next, &QToolButton::clicked, this, &FindBar::findNext);
    connect(close, &QToolButton::clicked, this, &FindBar::dismiss);

    adjustSize();
    hide();
}

void FindBar::setView(QWebEngineView *view)
{
    if (m_view == view)
        return;
    if (m_view)
        m_view->page()->findText(QString());
    m_view = view;
    if (isVisible())
        find(false);
}

void FindBar::open()
{
    show();
    raise();
    m_edit->setFocus();
    m_edit->selectAll();
    if (!m_edit->text().isEmpty())
        find(false);
}

void FindBar::findNext() { find(false); }
void FindBar::findPrevious() { find(true); }

void FindBar::dismiss()
{
    if (m_view) {
        m_view->page()->findText(QString());
        m_view->setFocus();
    }
    m_count->clear();
    hide();
}

void FindBar::find(bool backward)
{
    if (!m_view)
        return;
    const QString text = m_edit->text();
    if (text.isEmpty()) {
        m_view->page()->findText(QString());
        m_count->clear();
        return;
    }
    QWebEnginePage::FindFlags flags;
    if (backward)
        flags |= QWebEnginePage::FindBackward;
    QPointer<FindBar> self(this);
    m_view->page()->findText(text, flags, [self](const QWebEngineFindTextResult &r) {
        if (!self)
            return;
        self->m_count->setText(r.numberOfMatches() ? tr("%1 of %2").arg(r.activeMatch()).arg(r.numberOfMatches())
                                                   : tr("No results"));
    });
}

void FindBar::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        dismiss();
        return;
    }
    QFrame::keyPressEvent(event);
}
