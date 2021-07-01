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
#include "ut_vnoteplaywidget.h"
#include "vnoteplaywidget.h"
#include "vnoteitem.h"
#include "vnote2siconbutton.h"
#include "utils.h"

ut_vnoteplaywidget_test::ut_vnoteplaywidget_test()
{
}

void ut_vnoteplaywidget_test::SetUp()
{
    m_vnoteplaywidget = new VNotePlayWidget;
}

void ut_vnoteplaywidget_test::TearDown()
{
    delete m_vnoteplaywidget;
}

TEST_F(ut_vnoteplaywidget_test, onVoicePlayPosChange)
{
    m_vnoteplaywidget->onVoicePlayPosChange(24);
}

TEST_F(ut_vnoteplaywidget_test, setVoiceBlock)
{
}

TEST_F(ut_vnoteplaywidget_test, playVideo)
{
    m_vnoteplaywidget->onSliderPressed();
    m_vnoteplaywidget->initPlayer();
    m_vnoteplaywidget->onPlayBtnClicked();
    m_vnoteplaywidget->onSliderReleased();
    m_vnoteplaywidget->onPauseBtnClicked();
    m_vnoteplaywidget->onCloseBtnClicked();
    m_vnoteplaywidget->getPlayerStatus();
    m_vnoteplaywidget->getVoiceData();
    m_vnoteplaywidget->onDurationChanged(100);
}
