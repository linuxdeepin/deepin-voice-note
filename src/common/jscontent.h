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
#include <QEventLoop>
#include <QtWebEngineWidgets/qwebenginepage.h>

class JsContent : public QObject
{
    Q_OBJECT
public:
    explicit JsContent(QObject *parent = nullptr);
    static JsContent *instance();
    QVariant callJsSynchronous(QWebEnginePage *page, const QString &funtion);
signals:
    void callJsInitData(const QString &jsonData); //调用web前端，设置json格式数据
    void callJsSetHtml(const QString &html); //调用web前端，设置html格式数据
    void callJsInsertVoice(const QString &jsonData); //调用web前端，插入语音
    void callJsSetVoiceText(const QString &text); //调用web前端，设置语音转文字结果
    void callJsAsrTip(const QString &text); //调用web前端，设置语音转文字提示信息
    void callJsInsertImages(const QStringList &images); //调用web前端，插入图片
    void callJsSetPlayStatus(int status); //调用web前端, 设置播放状态，0播放中，1暂停中 2.结束播放
    bool callJsSearch(const QString &html); //调用web前端，设置json格式数据

    void textChange();
    void loadFinsh();
    void popupMenu(int type, int x, int y, const QVariant &json);
    void playVoice(const QVariant &json, bool bIsSame);

public slots:
    void jsCallTxtChange(); //web前端调用后端，通知数据变化
    void jsCallChannleFinish(); //web前端调用后端，通知网页加载完成
    void jsCallPopupMenu(int type, int x, int y, const QVariant &json); //web前端调用后端，弹出右键菜单
    void jsCallPlayVoice(const QVariant &json, bool bIsSame); //web前端调用后端，播放语音
    QString getTranslation(const QString &context, const QString &key); //web前端调用后端，获取翻译文件
private:
    QVariant m_synResult;
    QEventLoop m_synLoop;
};

#endif // JSCONTENT_H
