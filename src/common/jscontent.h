/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
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
#ifndef JSCONTENT_H
#define JSCONTENT_H

#include <QObject>
#include <QVariant>
#include <QWebEnginePage>
#include <QEventLoop>

struct VNVoiceBlock;

class JsContent : public QObject
{
    Q_OBJECT
public:
    explicit JsContent(QObject *parent = nullptr);
    ~JsContent();
    static JsContent *instance();
    QVariant callJsSynchronous(const QString &js);
    VNVoiceBlock *getCurrentVoice();
    void setWebPage(QWebEnginePage *page);
    QWebEnginePage *getWebPage();
    bool webLoadFinsh();
signals:
    void callJsInitData(const QString &jsonData);
    void callJsSetHtml(const QString &html);
    void callJsInsertVoice(const QString &jsonData);
    //设置播放状态，0,播放中，1暂停中，2.结束播放
    void callJsSetPlayStatus(int status);
    //设置语音转文字结果，text为内容，flag 0 提示性文本（＂正在转文字中＂,１结果文本,空代表转失败了
    void callJsSetVoiceText(const QString &text, int flag);

    void playVoice(const VNVoiceBlock *block, const bool &bIsSame);
    void textChange();
public slots:
    QString jsCallGetVoiceSize(const QString &millisecond);
    QString jsCallGetVoiceTime(const QString &time);
    int  jsCallPlayVoice(const QVariant &json, const bool &bIsSame);
    //弹出语音操作选项
    void jsCallPopVoiceMenu(const QVariant &json);
    void jsCallTxtChange();
    void jsCallChannleFinish();
private:
    bool m_loadFinsh {false};
    QWebEnginePage *m_page {nullptr};
    QVariant m_synResult;
    QEventLoop m_synLoop;
    VNVoiceBlock *m_currentPlay {nullptr};
    VNVoiceBlock *m_currentVoice {nullptr};
};

#endif // JSCONTENT_H
