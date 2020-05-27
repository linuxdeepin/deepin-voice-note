#ifndef VNOTE2SICONBUTTON_H
#define VNOTE2SICONBUTTON_H

#include <QMouseEvent>

#include <DApplicationHelper>
#include <DFloatingButton>

DWIDGET_USE_NAMESPACE

class VNote2SIconButton : public DFloatingButton
{
    Q_OBJECT
public:
    explicit VNote2SIconButton(const QString normal
            ,const QString press
            ,QWidget *parent = nullptr);

    enum {
        Invalid = -1,
        Normal,
        Press,
//        Disabled, //Not support this state now
        MaxState
    };

    bool isPressed() const;
    void setState(int state);
signals:

public slots:
protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void updateIcon();
    void setCommonIcon(bool isCommon);

    QString m_icons[MaxState];
    int     m_state {Normal};

    bool    m_useCommonIcons {true};
};

#endif // VNOTE2SICONBUTTON_H
