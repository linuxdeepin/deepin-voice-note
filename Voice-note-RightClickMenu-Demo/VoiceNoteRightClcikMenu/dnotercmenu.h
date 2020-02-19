#ifndef DNOTERCMENU_H
#define DNOTERCMENU_H

#include <DMenu>

DWIDGET_USE_NAMESPACE

class DNoteRCMenu : public DMenu
{
    Q_OBJECT
    Q_DISABLE_COPY(DNoteRCMenu)
public:
    explicit DNoteRCMenu(QWidget *parent = nullptr);

signals:
    void sigTriggered(const QString& text);
private slots:
    void slotTriggered(const QString &text);
private:
    void initUi();
};

#endif // DNOTERCMENU_H
