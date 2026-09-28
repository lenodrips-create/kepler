#pragma once

#include <QElapsedTimer>
#include <QFrame>
#include <QPointer>

class QLabel;
class QProgressBar;
class QToolButton;
class QVBoxLayout;
class QWebEngineDownloadRequest;

class DownloadItem : public QFrame
{
    Q_OBJECT

public:
    DownloadItem(QWebEngineDownloadRequest *download, QWidget *parent = nullptr);
    bool isActive() const;

signals:
    void stateChanged();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void refresh();
    QString filePath() const;

    QPointer<QWebEngineDownloadRequest> m_download;
    QLabel *m_name;
    QLabel *m_status;
    QProgressBar *m_bar;
    QToolButton *m_action;
    QElapsedTimer m_clock;
};

// Popup listing downloads from every window.
class DownloadsPanel : public QFrame
{
    Q_OBJECT

public:
    explicit DownloadsPanel(QWidget *parent = nullptr);

    void addDownload(QWebEngineDownloadRequest *download);
    void popup(QWidget *anchor);
    int activeCount() const;

signals:
    void activeCountChanged(int count);

private:
    void clearFinished();

    QVBoxLayout *m_list;
    QLabel *m_empty;
    QList<QPointer<DownloadItem>> m_items;
};
