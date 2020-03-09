#ifndef MIDDLEVIEW_H
#define MIDDLEVIEW_H

#include <DListView>
#include <DMenu>
#include <DLabel>

DWIDGET_USE_NAMESPACE
class MiddleViewDelegate;

class MiddleView : public DListView
{
    Q_OBJECT
public:
    MiddleView(QWidget *parent = nullptr);
    void setSearchKey(const QRegExp &key);
    void setCurrentId(qint64 id);
    void setVisibleEmptySearch(bool visible);
    qint64 getCurrentId();
    QStandardItemModel *getStandardItemModel();

signals:

protected:
    void mousePressEvent(QMouseEvent *event);
private:
    void initDelegate();
    void initModel();
    void initMenu();
    void initUI();
    qint64              m_currentId {-1};
    QRegExp             m_searchKey;
    DLabel             *m_emptySearch {nullptr};
    DMenu              *m_noteMenu {nullptr};
    QStandardItemModel *m_pDataModel {nullptr};
    MiddleViewDelegate *m_pItemDelegate {nullptr};
};

#endif // MIDDLEVIEW_H
