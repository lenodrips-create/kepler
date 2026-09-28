#pragma once

#include <QLineEdit>

class QCompleter;
class QStringListModel;

// The omnibox: type an address or anything you want to search for.
class UrlBar : public QLineEdit
{
    Q_OBJECT

public:
    explicit UrlBar(QWidget *parent = nullptr);

    void setUrl(const QUrl &url);
    void setCompletions(const QStringList &items);

signals:
    void navigateRequested(const QString &input);
    void escapePressed();

protected:
    void focusInEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QAction *m_security;
    QCompleter *m_completer;
    QStringListModel *m_model;
};
