#ifndef INITEMPTYPAGE_H
#define INITEMPTYPAGE_H

#include "widgets/vnoteiconbutton.h"

#include <QWidget>

#include <DLabel>
#include <DPushButton>
#include <DSuggestButton>

DWIDGET_USE_NAMESPACE

class HomePage : public QWidget
{
    Q_OBJECT
public:
    explicit HomePage(QWidget *parent = nullptr);

signals:
    void sigAddFolderByInitPage();

private:
    void initUi();
    void initConnection();

    VNoteIconButton *m_Image;
    DSuggestButton *m_PushButton;
    DLabel *m_Text;

};

#endif // INITEMPTYPAGE_H
