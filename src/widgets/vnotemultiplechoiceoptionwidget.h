#ifndef VNOTEMULTIPLECHOICERIGHTWIDGET_H
#define VNOTEMULTIPLECHOICERIGHTWIDGET_H

#include <QWidget>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDomDocument>
#include <QDomElement>
#include <QSvgRenderer>
#include <QFile>

#include <DToolButton>
#include <DFontSizeManager>
#include <DApplicationHelper>
#include <DWidget>
#include <DLabel>

DWIDGET_USE_NAMESPACE
//多选-多选详情页
class VnoteMultipleChoiceOptionWidget : public DWidget
{
    Q_OBJECT
public:
    enum ButtonValue{
       Move = 1,
        SaveAsTxT = 2,
        SaveAsVoice = 3,
        Delete = 4
    };
    explicit VnoteMultipleChoiceOptionWidget(QWidget *parent = nullptr);
    //初始化ui
    void initUi();
    //更新笔记数量
    void setNoteNumber(int number);
    //设置按钮是否置灰
    void enableButtons(bool saveAsTxtButtonStatus = true,bool saveAsVoiceButtonStatus = true , bool moveButtonStatus = true);
    //press更新svg
    void buttonPressed(ButtonValue value);

private:
    //获得svg
    QPixmap setSvgColor(QString svgPath,QString color);
    //设置svg颜色属性
    void setSVGBackColor(QDomElement &elem,QString attr,QString val);
signals:
    void requestMultipleOption(int id);
protected:
    void trigger(int id);
    void initConnections();
    void changeFromThemeType();
    void onFontChanged();
    void resizeEvent(QResizeEvent *event)override;
private:
    DLabel *m_tipsLabel  {nullptr};
    DToolButton *m_moveButton {nullptr};
    DToolButton *m_saveVoiceButton {nullptr};
    DToolButton *m_saveTextButton {nullptr};
    DToolButton *m_deleteButton {nullptr};
    int m_noteNumber  = 0;
};

#endif // VNOTEMULTIPLECHOICERIGHTWIDGET_H
