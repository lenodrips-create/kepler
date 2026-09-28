#include "DownloadsPanel.h"

#include <QDesktopServices>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QStandardPaths>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEngineDownloadRequest>

namespace {
QString humanSize(qint64 bytes)
{
    return QLocale().formattedDataSize(qMax<qint64>(bytes, 0), 1);
}
} // namespace

// ---------------------------------------------------------------- DownloadItem

DownloadItem::DownloadItem(QWebEngineDownloadRequest *download, QWidget *parent)
    : QFrame(parent), m_download(download)
{
    setObjectName("DownloadItem");
    m_clock.start();

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 10, 8, 10);
    layout->setSpacing(10);

    auto *text = new QVBoxLayout;
    text->setSpacing(5);
    m_name = new QLabel(download->downloadFileName(), this);
    m_name->setObjectName("DownloadName");
    m_name->setMaximumWidth(260);
    m_bar = new QProgressBar(this);
    m_bar->setTextVisible(false);
    m_status = new QLabel(this);
    m_status->setObjectName("DownloadStatus");
    text->addWidget(m_name);
    text->addWidget(m_bar);
    text->addWidget(m_status);

    m_action = new QToolButton(this);
    m_action->setIconSize(QSize(18, 18));

    layout->addLayout(text, 1);
    layout->addWidget(m_action, 0, Qt::AlignVCenter);

    connect(m_action, &QToolButton::clicked, this, [this] {
        if (!m_download)
            return;
        if (m_download->state() == QWebEngineDownloadRequest::DownloadInProgress)
            m_download->cancel();
        else
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_download->downloadDirectory()));
    });
    connect(download, &QWebEngineDownloadRequest::receivedBytesChanged, this, &DownloadItem::refresh);
    connect(download, &QWebEngineDownloadRequest::totalBytesChanged, this, &DownloadItem::refresh);
    connect(download, &QWebEngineDownloadRequest::stateChanged, this, [this] {
        refresh();
        emit stateChanged();
    });
    refresh();
}

QString DownloadItem::filePath() const
{
    return m_download ? QDir(m_download->downloadDirectory()).filePath(m_download->downloadFileName())
                      : QString();
}

bool DownloadItem::isActive() const
{
    return m_download && m_download->state() == QWebEngineDownloadRequest::DownloadInProgress;
}

void DownloadItem::refresh()
{
    if (!m_download) {
        m_status->setText(tr("Gone"));
        m_action->hide();
        return;
    }
    m_name->setText(fontMetrics().elidedText(m_download->downloadFileName(), Qt::ElideMiddle, 260));
    m_name->setToolTip(filePath());

    const qint64 got = m_download->receivedBytes();
    const qint64 total = m_download->totalBytes();

    switch (m_download->state()) {
    case QWebEngineDownloadRequest::DownloadRequested:
    case QWebEngineDownloadRequest::DownloadInProgress: {
        m_action->setIcon(QIcon(":/kepler/icons/stop.svg"));
        m_action->setToolTip(tr("Cancel"));
        const double secs = qMax(0.001, m_clock.elapsed() / 1000.0);
        const QString speed = humanSize(qint64(got / secs)) + tr("/s");
        if (total > 0) {
            m_bar->setRange(0, 1000);
            m_bar->setValue(int(got * 1000 / total));
            m_status->setText(tr("%1 of %2 · %3").arg(humanSize(got), humanSize(total), speed));
        } else {
            m_bar->setRange(0, 0); // unknown size: busy indicator
            m_status->setText(tr("%1 · %2").arg(humanSize(got), speed));
        }
        break;
    }
    case QWebEngineDownloadRequest::DownloadCompleted:
        m_bar->setRange(0, 1);
        m_bar->setValue(1);
        m_bar->hide();
        m_status->setText(tr("Done · %1 · double-click to open").arg(humanSize(got)));
        m_action->setIcon(QIcon(":/kepler/icons/folder.svg"));
        m_action->setToolTip(tr("Show in folder"));
        break;
    case QWebEngineDownloadRequest::DownloadCancelled:
        m_bar->hide();
        m_status->setText(tr("Cancelled"));
        m_action->setIcon(QIcon(":/kepler/icons/folder.svg"));
        m_action->setToolTip(tr("Open downloads folder"));
        break;
    case QWebEngineDownloadRequest::DownloadInterrupted:
        m_bar->hide();
        m_status->setText(tr("Failed: %1").arg(m_download->interruptReasonString()));
        m_action->setIcon(QIcon(":/kepler/icons/folder.svg"));
        m_action->setToolTip(tr("Open downloads folder"));
        break;
    }
}

void DownloadItem::mouseDoubleClickEvent(QMouseEvent *)
{
    if (m_download && m_download->state() == QWebEngineDownloadRequest::DownloadCompleted)
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath()));
}

// -------------------------------------------------------------- DownloadsPanel

DownloadsPanel::DownloadsPanel(QWidget *parent) : QFrame(parent, Qt::Popup | Qt::FramelessWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto *card = new QFrame(this);
    card->setObjectName("DownloadsCard");
    outer->addWidget(card);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);

    auto *header = new QHBoxLayout;
    auto *title = new QLabel(tr("Downloads"), card);
    title->setObjectName("DownloadsTitle");
    auto *clear = new QPushButton(tr("Clear"), card);
    auto *folder = new QPushButton(tr("Open folder"), card);
    header->addWidget(title);
    header->addStretch();
    header->addWidget(clear);
    header->addWidget(folder);
    layout->addLayout(header);

    auto *scroll = new QScrollArea(card);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *listHost = new QWidget;
    m_list = new QVBoxLayout(listHost);
    m_list->setContentsMargins(0, 0, 0, 0);
    m_list->setSpacing(8);
    m_empty = new QLabel(tr("Nothing downloaded yet.\nFiles you download will land here."), listHost);
    m_empty->setObjectName("DownloadsEmpty");
    m_empty->setAlignment(Qt::AlignCenter);
    m_list->addWidget(m_empty);
    m_list->addStretch();
    scroll->setWidget(listHost);
    layout->addWidget(scroll, 1);

    connect(clear, &QPushButton::clicked, this, &DownloadsPanel::clearFinished);
    connect(folder, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(
            QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)));
    });

    resize(380, 360);
}

void DownloadsPanel::addDownload(QWebEngineDownloadRequest *download)
{
    auto *item = new DownloadItem(download);
    m_list->insertWidget(0, item);
    m_items.prepend(item);
    m_empty->hide();
    connect(item, &DownloadItem::stateChanged, this, [this] { emit activeCountChanged(activeCount()); });
    emit activeCountChanged(activeCount());
}

int DownloadsPanel::activeCount() const
{
    int n = 0;
    for (const auto &item : m_items)
        if (item && item->isActive())
            ++n;
    return n;
}

void DownloadsPanel::clearFinished()
{
    for (auto &item : m_items) {
        if (item && !item->isActive())
            delete item;
    }
    m_items.removeAll(nullptr);
    m_empty->setVisible(m_items.isEmpty());
}

void DownloadsPanel::popup(QWidget *anchor)
{
    const QPoint bottomRight = anchor->mapToGlobal(QPoint(anchor->width(), anchor->height() + 6));
    QPoint pos(bottomRight.x() - width(), bottomRight.y());
    if (QScreen *screen = anchor->screen()) {
        const QRect avail = screen->availableGeometry();
        pos.setX(qBound(avail.left(), pos.x(), avail.right() - width()));
    }
    move(pos);
    show();
    raise();
}
