#ifndef MIDDLEVIEW_H
#define MIDDLEVIEW_H

#include <QSettings>

#include <DListView>
#include <DMenu>
#include <DLabel>

DWIDGET_USE_NAMESPACE
class MiddleViewDelegate;
class MiddleViewSortFilter;
struct VNoteItem;

class MiddleView : public DListView
{
    Q_OBJECT
public:
    MiddleView(QWidget *parent = nullptr);
    void setSearchKey(const QString &key);
    void setCurrentId(qint64 id);
    void setVisibleEmptySearch(bool visible);
    void setOnlyCurItemMenuEnable(bool enable);
    void addRowAtHead(VNoteItem *note);
    void appendRow(VNoteItem *note);
    void clearAll();
    void setCurrentIndex(int index);
    void editNote();
    void saveAsText();
    void saveRecords();

    qint64 getCurrentId();
    qint32 rowCount() const;

    VNoteItem *deleteCurrentRow();
    VNoteItem *getCurrVNotedata() const;

signals:
public slots:
    void onNoteChanged();
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    void initDelegate();
    void initModel();
    void initMenu();
    void initUI();
    void initAppSetting();

    bool                m_onlyCurItemMenuEnable {false};
    qint64              m_currentId {-1};
    QString             m_searchKey;
    DLabel             *m_emptySearch {nullptr};
    DMenu              *m_noteMenu {nullptr};
    QStandardItemModel *m_pDataModel {nullptr};
    MiddleViewDelegate *m_pItemDelegate {nullptr};
    MiddleViewSortFilter *m_pSortViewFilter {nullptr};

    //App setting, reference to VNoteApplication's setting
    QSharedPointer<QSettings> m_qspSetting {nullptr};
};

#endif // MIDDLEVIEW_H
