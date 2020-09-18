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
#include "ut_setting.h"

#define private public
#include "setting.h"
#undef private

ut_setting_test::ut_setting_test()
{

}

TEST_F(ut_setting_test, keys)
{
    CustemBackend custembackend("/home/zhangteng/.config/deepin/deepin-voice-note/config.conf");
    QStringList keyList;
    ASSERT_EQ(keyList.size(), 0);
    keyList = custembackend.keys();
    ASSERT_GE(keyList.size(), 0);
}

TEST_F(ut_setting_test, setOption)
{
    QString key = "old._app_main_wnd_sz_key_";
    QString value = "/home/zhangteng/test";
    setting::instance()->setOption(key, value);
    ASSERT_FALSE(key.isEmpty());
    QStringList keyList = setting::instance()->m_setting->keys();
    value = "test";
    setting::instance()->setOption(key, value);
}

TEST_F(ut_setting_test, doSetOption)
{
    CustemBackend custembackend("/home/zhangteng/.config/deepin/deepin-voice-note/config.conf");
    QString key = "old._app_main_wnd_sz_key_";
    QString value = "/home/zhangteng/test";
    custembackend.doSetOption(key, value);
    key = "_app_main_wnd_sz_key_";
    custembackend.doSetOption(key, value);
    custembackend.getOption(key);
}

TEST_F(ut_setting_test, getSetting)
{
    DSettings* tmpset = setting::instance()->getSetting();
    ASSERT_TRUE(tmpset != nullptr);
}

TEST_F(ut_setting_test, GenerateSettingTranslate)
{
    extern void GenerateSettingTranslate();
    GenerateSettingTranslate();
}

TEST_F(ut_setting_test, doSync)
{
    CustemBackend custembackend("/home/zhangteng/.config/deepin/deepin-voice-note/config.conf");
    custembackend.doSync();
}
