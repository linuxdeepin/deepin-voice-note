#ifndef VNOTEMULTIPLECHOICERIGHTWIDGET_H
#define VNOTEMULTIPLECHOICERIGHTWIDGET_H

#include <QWidget>

#include <DToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <DFontSizeManager>
#include <DApplicationHelper>
#include <DWidget>
#include <DLabel>

DWIDGET_USE_NAMESPACE
//dx-多选详情页
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
    void enableButtons(bool saveAsTxtButtonStatus = true,bool saveAsVoiceButtonStatus = true);
signals:
    void requestMultipleOption(int id);
protected:
    void trigger(int id);
    void changeFromThemeType();
    void initConnections();
private:
    DLabel *m_tipsLabel  {nullptr};
    DToolButton *moveButton {nullptr};
    DToolButton *m_saveVoiceButton {nullptr};
    DToolButton *m_saveTextButton {nullptr};
    DToolButton *deleteButton {nullptr};
    int m_noteNumber  = 0;
};

#endif // VNOTEMULTIPLECHOICERIGHTWIDGET_H
