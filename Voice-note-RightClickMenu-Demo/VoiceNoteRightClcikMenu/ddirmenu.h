#ifndef DDIRMENU_H
#define DDIRMENU_H

#include <DMenu>

DWIDGET_USE_NAMESPACE

class DDirMenu : public DMenu
{
    Q_OBJECT
    Q_DISABLE_COPY(DDirMenu)
public:
    explicit DDirMenu(QWidget *parent = nullptr);

signals:
    void sigTriggered(const QString& text);
private slots:
    void slotTriggered(const QString &text);
private:
    void initUi();
};

#endif // DDIRMENU_H
