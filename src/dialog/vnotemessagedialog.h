/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
    explicit VNoteMessageDialog(int msgType , QWidget *parent = nullptr ,int notesCount =1 );

    enum MessageType {
        DeleteNote,
        AbortRecord,
        DeleteFolder,
        AsrTimeLimit,
        AborteAsr,
        VolumeTooLow,
        CutNote,
    };

protected:
    //初始化布局
    void initUI();
    //连接槽函数
    void initConnections();
    //设置文本
    void initMessage();
    //显示单按钮
    void setSingleButton(); //Need to be Optimzed
signals:

public slots:

protected:
    DLabel *m_pMessage {nullptr};
    DPushButton *m_cancelBtn {nullptr};
    DWarningButton *m_confirmBtn {nullptr};
    DVerticalLine *m_buttonSpliter {nullptr};
    //笔记数量
    int m_notesCount = 1;
    MessageType m_msgType;
};

#endif // VNOTEMESSAGEDIALOG_H
