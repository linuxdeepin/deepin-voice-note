#ifndef VNOTEICONBUTTON_H
#define VNOTEICONBUTTON_H

#include <QMouseEvent>

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
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

signals:

public slots:
private:
    QPixmap loadPixmap(const QString &path);
    void    updateIcon();

    enum {
        Normal,
        Hover,
        Press,
        MaxState
    };

    QString m_icons[MaxState];
    int     m_state {Normal};
};

#endif // VNOTEICONBUTTON_H
