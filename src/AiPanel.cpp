#include "AiPanel.h"

#include "BrowserWindow.h"
#include "WebView.h"

#include <QButtonGroup>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPropertyAnimation>
#include <QSettings>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

constexpr int Margin = 12;
constexpr int MaxWidth = 560;

// A colored letter badge, shown until the assistant's real logo has downloaded.
QIcon letterIcon(const QString &name, const QColor &color)
{
    QPixmap pm(64, 64);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRectF(0, 0, 64, 64), 16, 16);
    QFont f = p.font();
    f.setPixelSize(38);
    f.setBold(true);
    p.setFont(f);
    p.setPen(Qt::white);
    p.drawText(pm.rect(), Qt::AlignCenter, name.left(1));
    return QIcon(pm);
}

// Real logos ship inside the app (resources/ai), so they show even offline.
QString logoPath(const QString &name)
{
    return QStringLiteral(":/kepler/ai/%1.png").arg(name.toLower());
}

} // namespace

AiPanel::AiPanel(QWebEngineProfile *profile, BrowserWindow *window, QWidget *parent)
    : QFrame(parent), m_profile(profile), m_window(window)
{
    setObjectName("AiPanel");

    m_assistants = {
        {"Claude", QUrl("https://claude.ai/new"), QColor(0xd9, 0x77, 0x57)},
        {"ChatGPT", QUrl("https://chatgpt.com/"), QColor(0x10, 0xa3, 0x7f)},
        {"Gemini", QUrl("https://gemini.google.com/app"), QColor(0x4f, 0x7d, 0xf2)},
        {"Copilot", QUrl("https://copilot.microsoft.com/"), QColor(0x2b, 0x88, 0xd8)},
        {"Perplexity", QUrl("https://www.perplexity.ai/"), QColor(0x20, 0x80, 0x8d)},
    };

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(1, 0, 1, 1);
    layout->setSpacing(0);

    // --- header: title and actions
    auto *header = new QWidget(this);
    header->setObjectName("AiHeader");
    auto *headerCol = new QVBoxLayout(header);
    headerCol->setContentsMargins(12, 8, 6, 10);
    headerCol->setSpacing(8);

    auto *row = new QHBoxLayout;
    row->setSpacing(2);
    auto *icon = new QLabel(header);
    icon->setPixmap(QIcon(":/kepler/icons/sparkle.svg").pixmap(18, 18));
    auto *title = new QLabel(tr("AI"), header);
    title->setObjectName("AiTitle");

    auto button = [header](const char *name, const QString &tip) {
        auto *b = new QToolButton(header);
        b->setIcon(QIcon(QStringLiteral(":/kepler/icons/%1.svg").arg(QLatin1String(name))));
        b->setIconSize(QSize(16, 16));
        b->setToolTip(tip);
        b->setCursor(Qt::PointingHandCursor);
        return b;
    };
    auto *fresh = button("plus", tr("New chat"));
    auto *popOut = button("external", tr("Open in a new tab"));
    auto *hide = button("stop", tr("Hide (Ctrl+Shift+Space)"));

    row->addWidget(icon);
    row->addSpacing(6);
    row->addWidget(title);
    row->addStretch();
    row->addWidget(fresh);
    row->addWidget(popOut);
    row->addWidget(hide);
    headerCol->addLayout(row);

    // --- assistant picker
    auto *pills = new QHBoxLayout;
    pills->setSpacing(4);
    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);
    for (int i = 0; i < m_assistants.size(); ++i) {
        auto *pill = new QToolButton(header);
        pill->setObjectName("AiPill");
        pill->setText(m_assistants[i].name);
        pill->setToolTip(tr("Chat with %1").arg(m_assistants[i].name));
        pill->setCheckable(true);
        pill->setIconSize(QSize(16, 16));
        pill->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        pill->setCursor(Qt::PointingHandCursor);
        pill->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_group->addButton(pill, i);
        pills->addWidget(pill);
        m_assistants[i].pill = pill;
    }
    pills->addStretch();
    headerCol->addLayout(pills);

    m_stack = new QStackedWidget(this);
    layout->addWidget(header);
    layout->addWidget(m_stack, 1);

    m_anim = new QPropertyAnimation(this, "pos", this);
    m_anim->setDuration(220);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_anim, &QPropertyAnimation::finished, this, [this] {
        if (!m_open)
            QFrame::hide();
    });

    connect(m_group, &QButtonGroup::idClicked, this, &AiPanel::select);
    connect(fresh, &QToolButton::clicked, this, [this] {
        if (m_current >= 0)
            viewFor(m_current)->load(m_assistants[m_current].home);
    });
    connect(popOut, &QToolButton::clicked, this, [this] {
        if (m_current < 0)
            return;
        const QUrl url = viewFor(m_current)->url();
        m_window->newTab(url.isEmpty() ? m_assistants[m_current].home : url);
        hidePanel();
    });
    connect(hide, &QToolButton::clicked, this, &AiPanel::hidePanel);

    refreshIcons();
    QFrame::hide();
}

void AiPanel::refreshIcons()
{
    for (Assistant &a : m_assistants) {
        const QString path = logoPath(a.name);
        a.pill->setIcon(QFile::exists(path) ? QIcon(path) : letterIcon(a.name, a.color));
    }
}

WebView *AiPanel::viewFor(int index)
{
    Assistant &a = m_assistants[index];
    if (!a.view) {
        a.view = new WebView(m_profile, m_window);
        m_stack->addWidget(a.view);
        a.view->load(a.home);
    }
    return a.view;
}

void AiPanel::select(int index)
{
    if (index < 0 || index >= m_assistants.size())
        return;
    m_current = index;
    m_assistants[index].pill->setChecked(true);
    WebView *view = viewFor(index);
    m_stack->setCurrentWidget(view);
    view->setFocus();
    QSettings().setValue("ai/assistant", m_assistants[index].name);
}

QRect AiPanel::targetGeometry() const
{
    const QRect area = parentWidget()->rect();
    const int width = qMin(MaxWidth, qMax(510, area.width() * 2 / 5));
    return QRect(area.width() - width - Margin, Margin, width, area.height() - 2 * Margin);
}

void AiPanel::toggle()
{
    m_open ? hidePanel() : showPanel();
}

void AiPanel::showPanel()
{
    if (m_current < 0) {
        // Reopen whichever assistant was used last.
        const QString last = QSettings().value("ai/assistant", "Claude").toString();
        int index = 0;
        for (int i = 0; i < m_assistants.size(); ++i)
            if (m_assistants[i].name == last)
                index = i;
        select(index);
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
    if (m_current >= 0)
        viewFor(m_current)->setFocus();
    emit openChanged(true);
}

void AiPanel::hidePanel()
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

void AiPanel::reposition()
{
    if (!m_open || m_anim->state() == QAbstractAnimation::Running)
        return;
    setGeometry(targetGeometry());
}
