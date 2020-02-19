#ifndef DNOTERIGHTCLICKMAINWINDOW_H
#define DNOTERIGHTCLICKMAINWINDOW_H

class DVoiceNoteRightClickMenu;
class DNoteRightClickMenu;

#include <DMainWindow>
#include <DMessageBox>

DWIDGET_USE_NAMESPACE

class DNoteRightClickMainWindow : public DMainWindow
{
    Q_OBJECT
public:
    explicit DNoteRightClickMainWindow(DMainWindow *parent = nullptr);

signals:

public slots:
    void OnToggledNoteMenu(const QString& text);
protected:
    void mousePressEvent(QMouseEvent *event) override;
private:

};

#endif // DNOTERIGHTCLICKMAINWINDOW_H
