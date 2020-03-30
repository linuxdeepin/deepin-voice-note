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

    bool isPressed() const;
signals:

public slots:
protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void updateIcon();
    void setCommonIcon(bool isCommon);

    enum {
        Normal,
        Press,
        Disabled,
        MaxState
    };

    QString m_icons[MaxState];
    int     m_state {Normal};

    bool    m_useCommonIcons {true};
};

#endif // VNOTE2SICONBUTTON_H
