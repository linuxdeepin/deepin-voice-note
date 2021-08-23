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

#include <QWidget>

struct VNoteItem;
class WebRichTextEditor;

class RichTextEdit : public QWidget
{
    Q_OBJECT
public:
    explicit RichTextEdit(QWidget *parent = nullptr);

    /**
     * @brief 初始化编辑区
     */
    void initWebView();

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
     * @brief 搜索当前笔记
     * @param searchKey : 搜索关键字
     * @return　搜索无结果返回false
     */
    bool findText(const QString &searchKey);

    /**
     * @brief 解除绑定的笔记数据
     */
    void unboundCurrentNoteData();

signals:
    /**
     * @brief 发送开始语音转文字的信号
     * @param json 语音json格式信息
     */
    void asrStart(const QVariant &json);

    //public slots:

private:
    WebRichTextEditor *m_webRichTextEditer {nullptr};
};

#endif // RICHTEXTEDIT_H
