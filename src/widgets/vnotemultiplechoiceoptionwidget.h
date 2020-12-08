#ifndef VNOTEMULTIPLECHOICERIGHTWIDGET_H
#define VNOTEMULTIPLECHOICERIGHTWIDGET_H

#include <QWidget>

#include <QVBoxLayout>
#include <QHBoxLayout>

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
    void initUi();
    void setNoteNumber(int number);
    //设置按钮是否置灰
    void enableButtons(bool saveAsTxtButtonStatus = true,bool saveAsVoiceButtonStatus = true , bool moveButtonStatus = true);

signals:
    void requestMultipleOption(int id);
protected:
    void trigger(int id);
    void changeFromThemeType();
    void initConnections();
private:
    DLabel *m_tipsLabel  {nullptr};
    DToolButton *m_moveButton {nullptr};
    DToolButton *m_saveVoiceButton {nullptr};
    DToolButton *m_saveTextButton {nullptr};
    DToolButton *m_deleteButton {nullptr};
    int m_noteNumber  = 0;
};

#endif // VNOTEMULTIPLECHOICERIGHTWIDGET_H
