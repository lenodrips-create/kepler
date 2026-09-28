#pragma once

#include <QFrame>
#include <QPointer>

class QLabel;
class QLineEdit;
class QWebEngineView;

// Floating find-in-page card (Ctrl+F).
class FindBar : public QFrame
{
    Q_OBJECT

public:
    explicit FindBar(QWidget *parent = nullptr);

    void setView(QWebEngineView *view);
    void open();
    void findNext();
    void findPrevious();
    void dismiss();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void find(bool backward);

    QLineEdit *m_edit;
    QLabel *m_count;
    QPointer<QWebEngineView> m_view;
};
