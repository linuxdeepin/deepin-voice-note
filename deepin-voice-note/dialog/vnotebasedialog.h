#ifndef VNOTEBASEDIALOG_H
#define VNOTEBASEDIALOG_H

#include <DAbstractDialog>
#include <DLabel>
#include <DWindowCloseButton>

DWIDGET_USE_NAMESPACE

class QVBoxLayout;

class VNoteBaseDialog : public DAbstractDialog
{
    Q_OBJECT
public:
    explicit VNoteBaseDialog(QWidget *parent = nullptr);

    void addContent(QWidget* content);
    void setIconPixmap(const QPixmap &iconPixmap);

protected:
    void initUI();
    void InitConnections();
    void setLogoVisable(bool visible=true);
    void setTitle(const QString& title);
    QLayout* getContentLayout();

    //Overrides
    void closeEvent(QCloseEvent *event) override;
signals:
    void closed();
public slots:
private:
    QWidget *m_titleBar {nullptr};
    DLabel  *m_logoIcon {nullptr};
    DLabel  *m_tileText {nullptr};
    DWindowCloseButton* m_closeButton {nullptr};

    QWidget *m_content {nullptr};
    QVBoxLayout *m_contentLayout {nullptr};
};

#endif // VNOTEBASEDIALOG_H
