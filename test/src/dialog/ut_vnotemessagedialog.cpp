/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     zhangteng <zhangteng@uniontech.com>
* Maintainer: zhangteng <zhangteng@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ut_vnotemessagedialog.h"

#define protected public
#include "vnotemessagedialog.h"
#undef protected

ut_vnotemessagedialog_test::ut_vnotemessagedialog_test()
{

}

TEST_F(ut_vnotemessagedialog_test, setSingleButton)
{
    VNoteMessageDialog vnotemessagedialog(0);
    vnotemessagedialog.setSingleButton();
    vnotemessagedialog.m_msgType = vnotemessagedialog.AbortRecord;
    vnotemessagedialog.initMessage();
    vnotemessagedialog.m_msgType = vnotemessagedialog.DeleteFolder;
    vnotemessagedialog.initMessage();
    vnotemessagedialog.m_msgType = vnotemessagedialog.AsrTimeLimit;
    vnotemessagedialog.initMessage();
    vnotemessagedialog.m_msgType = vnotemessagedialog.AborteAsr;
    vnotemessagedialog.initMessage();
    vnotemessagedialog.m_msgType = vnotemessagedialog.VolumeTooLow;
    vnotemessagedialog.initMessage();
    vnotemessagedialog.m_msgType = vnotemessagedialog.CutNote;
    vnotemessagedialog.initMessage();
}
