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

#ifndef VTEXTSPEECHANDTRMANAGER_H
#define VTEXTSPEECHANDTRMANAGER_H

class VTextSpeechAndTrManager
{
public:
    // 检测是否在朗读文本
    static bool isTextToSpeechInWorking();
    // 检测语音朗读开关是否打开
    static bool getTextToSpeechEnable();
    // 检测语音听写开关是否打开
    static bool getSpeechToTextEnable();
    // 检测文本翻译开关是否打开
    static bool getTransEnable();
    // 语音朗读
    static void onTextToSpeech();
    // 停止语音朗读
    static void onStopTextToSpeech();
    // 语音听写
    static void onSpeechToText();
    // 文本翻译
    static void onTextTranslate();
};

#endif // VTEXTSPEECHANDTRMANAGER_H
