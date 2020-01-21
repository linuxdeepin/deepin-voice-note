#ifndef VNOTEMESSAGEDIALOG_H
#define VNOTEMESSAGEDIALOG_H

#include "vnotebasedialog.h"

#include <DPushButton>
#include <DWarningButton>
#include <DVerticalLine>

DWIDGET_USE_NAMESPACE;

class VNoteMessageDialog : public VNoteBaseDialog
{
    Q_OBJECT
public:
    explicit VNoteMessageDialog(int msgType, QWidget *parent = nullptr);

    enum MessageType {
        DeleteNote,
        AbortRecord,
        DeleteFolder,
        AsrTimeLimit,
        AborteAsr,
    };

protected:
    void initUI();
    void initConnections();
    void initMessage();
    void setSingleButton(); //Need to be Optimzed
signals:

public slots:

protected:
    DLabel         *m_pMessage {nullptr};
    DPushButton    *m_cancelBtn {nullptr};
    DWarningButton *m_confirmBtn {nullptr};
    DVerticalLine  *m_buttonSpliter {nullptr};

    MessageType    m_msgType;
};

#endif // VNOTEMESSAGEDIALOG_H
