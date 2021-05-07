#ifndef UT_VNOTEMULTIPLECHOICEOPTIONWIDGET_H
#define UT_VNOTEMULTIPLECHOICEOPTIONWIDGET_H

#include <QWidget>
#include "gtest/gtest.h"
#include <QObject>

class VnoteMultipleChoiceOptionWidget;
class ut_vnotemultiplechoiceoptionwidget_test : public QObject
    , public ::testing::Test
{
    Q_OBJECT
public:
    ut_vnotemultiplechoiceoptionwidget_test();
    virtual void SetUp() override;
    virtual void TearDown() override;

protected:
    VnoteMultipleChoiceOptionWidget *m_widget {nullptr};
};

#endif // UT_VNOTEMULTIPLECHOICEOPTIONWIDGET_H
