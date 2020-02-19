#ifndef DVOICEMENU_H
#define DVOICEMENU_H

#include <DMenu>

DWIDGET_USE_NAMESPACE

class DVoiceMenu : public DMenu
{
    Q_OBJECT
    Q_DISABLE_COPY(DVoiceMenu)
public:
    explicit DVoiceMenu(QWidget *parent = nullptr);

signals:
    void sigTriggered(const QString& text);
private slots:
    void slotTriggered(const QString &text);
private:
    void initUi();
};

#endif // DVOICEMENU_H
