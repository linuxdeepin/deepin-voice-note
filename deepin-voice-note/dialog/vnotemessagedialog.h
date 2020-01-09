#ifndef VNOTEMESSAGEDIALOG_H
#define VNOTEMESSAGEDIALOG_H

#include "vnotebasedialog.h"

#include <DPushButton>
#include <DWarningButton>

DWIDGET_USE_NAMESPACE;

class VNoteMessageDialog : public VNoteBaseDialog
{
    Q_OBJECT
public:
    explicit VNoteMessageDialog(int msgType, QWidget *parent = nullptr);

    enum MessageType {
        DeleteNote,
        AbortRecord,
    };

protected:
    void initUI();
    void initConnections();
    void initMessage();
signals:

public slots:

protected:
    DLabel         *m_pMessage {nullptr};
    DPushButton    *m_cancelBtn {nullptr};
    DWarningButton *m_confirmBtn {nullptr};

    MessageType    m_msgType;
};

#endif // VNOTEMESSAGEDIALOG_H
