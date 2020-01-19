#ifndef VNOTEICONBUTTON_H
#define VNOTEICONBUTTON_H

#include <QMouseEvent>

#include <DApplicationHelper>
#include <DIconButton>

DWIDGET_USE_NAMESPACE

class VNoteIconButton : public DIconButton
{
    Q_OBJECT
public:
    explicit VNoteIconButton(QWidget *parent = nullptr
            , QString normal = ""
            , QString hover  = ""
            , QString press  = "");
    virtual ~VNoteIconButton() override;
    void setSeparateThemIcons(bool separate);
    void SetDisableIcon(const QString& disableIcon);
    void setBtnDisabled(bool disabled);
protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

signals:

public slots:
    void onThemeChanged(DGuiApplicationHelper::ColorType type);
private:
    QPixmap loadPixmap(const QString &path);
    void    updateIcon();

    enum {
        Normal,
        Hover,
        Press,
        Disabled,
        MaxState
    };

    QString m_icons[MaxState];
    int     m_state {Normal};

    //The Icon is different under different theme.
    bool m_separateThemeIcon {true};

    //Disable state
    bool m_isDisabled {false};
};

#endif // VNOTEICONBUTTON_H
