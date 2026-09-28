#include "ClaudePanel.h"

#include "BrowserWindow.h"
#include "WebView.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
const QUrl ClaudeUrl(QStringLiteral("https://claude.ai/new"));
constexpr int Margin = 12;
constexpr int MaxWidth = 480;
} // namespace

ClaudePanel::ClaudePanel(QWebEngineProfile *profile, BrowserWindow *window, QWidget *parent)
    : QFrame(parent), m_window(window)
{
    setObjectName("ClaudePanel");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(1, 0, 1, 1);
    layout->setSpacing(0);

    auto *header = new QWidget(this);
    header->setObjectName("ClaudeHeader");
    auto *row = new QHBoxLayout(header);
    row->setContentsMargins(12, 6, 6, 6);
    row->setSpacing(2);

    auto *icon = new QLabel(header);
    icon->setPixmap(QIcon(":/kepler/icons/sparkle.svg").pixmap(18, 18));
    auto *title = new QLabel(tr("Claude"), header);
    title->setObjectName("ClaudeTitle");

    auto button = [header](const char *name, const QString &tip) {
        auto *b = new QToolButton(header);
        b->setIcon(QIcon(QStringLiteral(":/kepler/icons/%1.svg").arg(QLatin1String(name))));
        b->setIconSize(QSize(16, 16));
        b->setToolTip(tip);
        b->setCursor(Qt::PointingHandCursor);
        return b;
    };
    auto *home = button("plus", tr("New chat"));
    auto *popOut = button("external", tr("Open in a new tab"));
    auto *hide = button("stop", tr("Hide Claude (Ctrl+Shift+Space)"));

    row->addWidget(icon);
    row->addSpacing(6);
    row->addWidget(title);
    row->addStretch();
    row->addWidget(home);
    row->addWidget(popOut);
    row->addWidget(hide);

    m_view = new WebView(profile, window);
    layout->addWidget(header);
    layout->addWidget(m_view, 1);

    m_anim = new QPropertyAnimation(this, "pos", this);
    m_anim->setDuration(220);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_anim, &QPropertyAnimation::finished, this, [this] {
        if (!m_open)
            QFrame::hide();
    });

    connect(home, &QToolButton::clicked, this, [this] { m_view->load(ClaudeUrl); });
    connect(popOut, &QToolButton::clicked, this, [this] {
        m_window->newTab(m_view->url().isEmpty() ? ClaudeUrl : m_view->url());
        hidePanel();
    });
    connect(hide, &QToolButton::clicked, this, &ClaudePanel::hidePanel);

    QFrame::hide();
}

QRect ClaudePanel::targetGeometry() const
{
    const QRect area = parentWidget()->rect();
    const int width = qMin(MaxWidth, qMax(320, area.width() * 2 / 5));
    return QRect(area.width() - width - Margin, Margin, width, area.height() - 2 * Margin);
}

void ClaudePanel::toggle()
{
    m_open ? hidePanel() : showPanel();
}

void ClaudePanel::showPanel()
{
    if (!m_loaded) {
        m_view->load(ClaudeUrl);
        m_loaded = true;
    }
    m_open = true;
    const QRect target = targetGeometry();
    resize(target.size());
    m_anim->stop();
    m_anim->setStartValue(QPoint(parentWidget()->width(), target.y()));
    m_anim->setEndValue(target.topLeft());
    show();
    raise();
    m_anim->start();
    m_view->setFocus();
    emit openChanged(true);
}

void ClaudePanel::hidePanel()
{
    if (!m_open)
        return;
    m_open = false;
    m_anim->stop();
    m_anim->setStartValue(pos());
    m_anim->setEndValue(QPoint(parentWidget()->width(), y()));
    m_anim->start();
    emit openChanged(false);
}

void ClaudePanel::reposition()
{
    if (!m_open || m_anim->state() == QAbstractAnimation::Running)
        return;
    setGeometry(targetGeometry());
}
