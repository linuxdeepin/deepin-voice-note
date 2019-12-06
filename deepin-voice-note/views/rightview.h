#ifndef RIGHTVIEW_H
#define RIGHTVIEW_H

#include "QPushButton"

#include <DStackedWidget>

DWIDGET_USE_NAMESPACE

class RightView : public DWidget
{
    Q_OBJECT
public:
    explicit RightView(QWidget *parent = nullptr);
protected:
    void resizeEvent(QResizeEvent * event);
private:
    void initUi();
    void initConnection();
    void adjustaddTextBtn();
signals:

public slots:
    void handleFolderAdd(qint64 id);
    void handleFolderDel(qint64 id);
    void handleFolderSwitch(qint64 id);
    void handleNoteAdd();
private:
    DStackedWidget *m_stackWidget {nullptr};
    QPushButton *m_addTextBtn {nullptr};
};

#endif // RIGHTVIEW_H
