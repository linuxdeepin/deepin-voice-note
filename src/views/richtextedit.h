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
#ifndef RICHTEXTEDIT_H
#define RICHTEXTEDIT_H

#include <QObject>
#include <QTimer>
#include <QtWebChannel/QWebChannel>
#include <QtWebEngineWidgets/QWebEngineView>

struct VNoteItem;
class JsContent;

class RichTextEdit : public QWidget
{
    Q_OBJECT
public:
    explicit RichTextEdit(QWidget *parent = nullptr);
    /**
     * @brief 设置笔记内容
     * @param data: 笔记内容
     * @param reg: 搜索关键字
     * @param fouse: 焦点
     */
    void initData(VNoteItem *data, const QString &reg, bool fouse = false);
    /**
     * @brief 插入语音
     * @param voicePath：语音路径
     * @param voiceSize: 语音时长，单位毫秒
     */
    void insertVoiceItem(const QString &voicePath, qint64 voiceSize);
    /**
     * @brief 更新编辑区内容
     */
    void updateNote();
    /**
     * @brief 从剪贴板中获取数据
     */
    void getImagePathsByClipboard();
signals:

public slots:
    /**
     * @brief 编辑区内容变化
     */
    void onTextChange();
    /**
     * @brief 图片插入点击事件响应
     */
    void onImgInsertClicked();
    /**
     * @brief ctrl+V事件
     */
    void onPaste();

private:
    /**
     * @brief 初始化编辑区
     */
    void initWebView();

private:
    VNoteItem *m_noteData {nullptr};
    QWebChannel *m_channel {nullptr};
    JsContent *m_jsContent {nullptr};
    QTimer *m_updateTimer {nullptr};
    QWebEngineView *m_webView {nullptr};
    bool m_textChange {false};
};

#endif // RICHTEXTEDIT_H
