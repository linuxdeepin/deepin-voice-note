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
    EXPECT_FALSE(m_widget->m_moveButton->icon().isNull());
    m_widget->buttonPressed(m_widget->SaveAsTxT);
    EXPECT_FALSE(m_widget->m_saveTextButton->icon().isNull());
    m_widget->buttonPressed(m_widget->SaveAsVoice);
    EXPECT_FALSE(m_widget->m_saveVoiceButton->icon().isNull());
    m_widget->buttonPressed(m_widget->Delete);
    EXPECT_FALSE(m_widget->m_deleteButton->icon().isNull());
}

TEST_F(ut_vnotemultiplechoiceoptionwidget_test, setSvgColor)
{
    QString path(":/icons/deepin/builtin/light/detail_notes_move.svg");
    QString color = QColor(Qt::red).name();
    EXPECT_FALSE(m_widget->setSvgColor(path, color).isNull());
}

TEST_F(ut_vnotemultiplechoiceoptionwidget_test, trigger)
{
    int id = 2;
    m_widget->trigger(id);
    EXPECT_EQ(m_widget->m_moveButton->testAttribute(Qt::WA_UnderMouse), false);
    EXPECT_EQ(m_widget->m_saveTextButton->testAttribute(Qt::WA_UnderMouse), false);
    EXPECT_EQ(m_widget->m_saveVoiceButton->testAttribute(Qt::WA_UnderMouse), false);
    EXPECT_EQ(m_widget->m_deleteButton->testAttribute(Qt::WA_UnderMouse), false);
}

TEST_F(ut_vnotemultiplechoiceoptionwidget_test, enableButtons)
{
    bool txtButtonStatus = false;
    bool voiceButtonStatus = true;
    m_widget->enableButtons(txtButtonStatus, voiceButtonStatus);
    EXPECT_EQ(m_widget->m_saveTextButton->isEnabled(), txtButtonStatus);
    EXPECT_EQ(m_widget->m_saveVoiceButton->isEnabled(), voiceButtonStatus);
}

TEST_F(ut_vnotemultiplechoiceoptionwidget_test, setNoteNumber)
{
    int noteNumber = 2;
    m_widget->m_noteNumber = 1;
    m_widget->setNoteNumber(noteNumber);
    EXPECT_EQ(m_widget->m_noteNumber, noteNumber);
}

TEST_F(ut_vnotemultiplechoiceoptionwidget_test, initConnections)
{
    m_widget->initConnections();
}
