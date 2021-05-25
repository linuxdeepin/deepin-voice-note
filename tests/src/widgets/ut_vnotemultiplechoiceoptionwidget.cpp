#include "ut_vnotemultiplechoiceoptionwidget.h"
#include "vnotemultiplechoiceoptionwidget.h"

ut_vnotemultiplechoiceoptionwidget_test::ut_vnotemultiplechoiceoptionwidget_test()
{
}

void ut_vnotemultiplechoiceoptionwidget_test::SetUp()
{
    m_widget = new VnoteMultipleChoiceOptionWidget;
}

void ut_vnotemultiplechoiceoptionwidget_test::TearDown()
{
    delete m_widget;
}

TEST_F(ut_vnotemultiplechoiceoptionwidget_test, changeFromThemeType)
{
    m_widget->changeFromThemeType();
}

TEST_F(ut_vnotemultiplechoiceoptionwidget_test, buttonPressed)
{
    m_widget->buttonPressed(m_widget->Move);
    m_widget->buttonPressed(m_widget->SaveAsTxT);
    m_widget->buttonPressed(m_widget->SaveAsVoice);
    m_widget->buttonPressed(m_widget->Delete);
}

TEST_F(ut_vnotemultiplechoiceoptionwidget_test, setSvgColor)
{
    QString path(":/icons/deepin/builtin/light/detail_notes_move.svg");
    QString color = QColor(Qt::red).name();
    m_widget->setSvgColor(path, color);
}

TEST_F(ut_vnotemultiplechoiceoptionwidget_test, trigger)
{
    int id = 2;
    m_widget->trigger(id);
}

TEST_F(ut_vnotemultiplechoiceoptionwidget_test, enableButtons)
{
    bool txtButtonStatus = false;
    bool voiceButtonStatus = true;
    m_widget->enableButtons(txtButtonStatus, voiceButtonStatus);
}

TEST_F(ut_vnotemultiplechoiceoptionwidget_test, setNoteNumber)
{
    int noteNumber = 2;
    m_widget->m_noteNumber = 1;
    m_widget->setNoteNumber(noteNumber);
}

TEST_F(ut_vnotemultiplechoiceoptionwidget_test, initConnections)
{
    m_widget->initConnections();
}
