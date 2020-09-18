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

#include "ut_vnotea2tmanager.h"

#define protected public
#include "vnotea2tmanager.h"
#undef protected

ut_vnotea2tmanager_test::ut_vnotea2tmanager_test()
{

}

TEST_F(ut_vnotea2tmanager_test, startAsr)
{
    VNoteA2TManager vnotea2tmanager;
    QString filepath = "/usr/share/music/bensound-sunny.mp3";
    qint64 fileDuration = 5460;
    vnotea2tmanager.startAsr(filepath, fileDuration, "", "");
    vnotea2tmanager.stopAsr();
}

TEST_F(ut_vnotea2tmanager_test, onNotify)
{
    VNoteA2TManager vnotea2tmanager;
    QString messg = "{\n    \"code\": \"000000\",\n    \"descInfo\": \"success\",\n    \"failType\": 0,\n    \"status\": 4,\n    \"text\": \"乙醇汽油的保质期呢是15~25天，建议呢在15天之内用完，那乙醇汽油确实是比较容易变质的，但过了一个月还是可以继续使用的。这就好比我买了这个薯片过期了一天，啊那不是说马上它就烂掉了长蘑菇了，呃只是过了这个期限。啊\"\n}\n";
    vnotea2tmanager.onNotify(messg);
    QString messg1 = "{\n    \"code\": \"900003\",\n    \"descInfo\": \"success\",\n    \"failType\": 0,\n    \"status\": 4,\n    \"text\": \"乙醇汽油的保质期呢是15~25天，建议呢在15天之内用完，那乙醇汽油确实是比较容易变质的，但过了一个月还是可以继续使用的。这就好比我买了这个薯片过期了一天，啊那不是说马上它就烂掉了长蘑菇了，呃只是过了这个期限。啊\"\n}\n";
    vnotea2tmanager.onNotify(messg1);
    QString messg2 = "{\n    \"code\": \"000000\",\n    \"descInfo\": \"success\",\n    \"failType\": 0,\n    \"status\": -1,\n    \"text\": \"乙醇汽油的保质期呢是15~25天，建议呢在15天之内用完，那乙醇汽油确实是比较容易变质的，但过了一个月还是可以继续使用的。这就好比我买了这个薯片过期了一天，啊那不是说马上它就烂掉了长蘑菇了，呃只是过了这个期限。啊\"\n}\n";
    vnotea2tmanager.onNotify(messg2);
    QString messg3 = "{\n    \"code\": \"000000\",\n    \"descInfo\": \"success\",\n    \"failType\": 0,\n    \"status\": 0,\n    \"text\": \"乙醇汽油的保质期呢是15~25天，建议呢在15天之内用完，那乙醇汽油确实是比较容易变质的，但过了一个月还是可以继续使用的。这就好比我买了这个薯片过期了一天，啊那不是说马上它就烂掉了长蘑菇了，呃只是过了这个期限。啊\"\n}\n";
    vnotea2tmanager.onNotify(messg3);
}

TEST_F(ut_vnotea2tmanager_test, getErrorCode)
{
    VNoteA2TManager vnotea2tmanager;
    asrMsg tmpstruct;
    tmpstruct.code = vnotea2tmanager.CODE_SUCCESS;
    tmpstruct.status = vnotea2tmanager.XF_fail;
    tmpstruct.failType = vnotea2tmanager.XF_upload;
    tmpstruct.descInfo = "test";
    tmpstruct.text = "test";
    vnotea2tmanager.getErrorCode(tmpstruct);

    tmpstruct.failType = vnotea2tmanager.XF_decode;
    vnotea2tmanager.getErrorCode(tmpstruct);

    tmpstruct.failType = vnotea2tmanager.XF_recognize;
    vnotea2tmanager.getErrorCode(tmpstruct);

    tmpstruct.failType = vnotea2tmanager.XF_limit;
    vnotea2tmanager.getErrorCode(tmpstruct);

    tmpstruct.failType = vnotea2tmanager.XF_verify;
    vnotea2tmanager.getErrorCode(tmpstruct);

    tmpstruct.failType = vnotea2tmanager.XF_mute;
    vnotea2tmanager.getErrorCode(tmpstruct);

    tmpstruct.failType = vnotea2tmanager.XF_other;
    vnotea2tmanager.getErrorCode(tmpstruct);

    tmpstruct.code = vnotea2tmanager.CODE_NETWORK;
    vnotea2tmanager.getErrorCode(tmpstruct);

    tmpstruct.code = vnotea2tmanager.CODE_SUCCESS;
    tmpstruct.status = vnotea2tmanager.XF_finish;
    vnotea2tmanager.getErrorCode(tmpstruct);
}



















