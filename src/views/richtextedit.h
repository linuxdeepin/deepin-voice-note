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

#include "widgets/vnoterightmenu.h"

#include <QObject>
#include <QTimer>
#include <QtWebChannel/QWebChannel>
#include <QtWebEngineWidgets/QWebEngineView>
#include <QWebEngineContextMenuData>

struct VNoteItem;
class JsContent;
class WebView : public QWebEngineView
{
    Q_OBJECT
public:
    explicit WebView(QWidget *parent = nullptr)
        : QWebEngineView(parent)
    {
    }

signals:
    /**
     * @brief QWebEngineView的右键信号
     */
    void sendMenuEvent(QContextMenuEvent *e);

protected:
    /**
      * @brief 右键操作
      * 拦截并屏蔽掉QWebEngineView的右键操作
      */
    void contextMenuEvent(QContextMenuEvent *e) override
    {
        emit sendMenuEvent(e);
    }
};

class RichTextEdit : public QWidget
{
    Q_OBJECT
public:
    explicit RichTextEdit(QWidget *parent = nullptr);

    enum Menu {
        PictureMenu = 0,
        VoiceMenu,
        TxtMenu,
        MaxMenu,
    };

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
    /**
     * @brief saveMenuParam 接收web弹出菜单类型及数据
     * @param type  菜单类型
     * @param x 弹出位置横坐标
     * @param y 弹出位置纵坐标
     * @param json  内容
     */
    void saveMenuParam(int m_menuType, int x, int y, const QVariant &json);

    /**
     * @brief 编辑区内容设置完成
     */
    void onSetDataFinsh();
    /**
     * @brief web端右键响应
     */
    void webContextMenuEvent(QContextMenuEvent *e);
    /**
     * @brief 右键菜单项点击响应
     * @param action
     */
    void onMenuActionClicked(QAction *action);

private:
    /**
     * @brief 初始化编辑区
     */
    void initWebView();
    /**
     * @brief 初始化右键菜单
     */
    void initRightMenu();
    /**
     * @brief 初始化信号连接
     */
    void initConnections();
    /**
     * @brief 文字菜单显示
     */
    void showTxtMenu();
    /**
     * @brief 图片菜单显示
     */
    void showPictureMenu();
    /**
     * @brief 语音菜单显示
     */
    void showVoiceMenu();

protected:
    /**
     * @brief 事件过滤处理
     */
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    VNoteItem *m_noteData {nullptr};
    QWebChannel *m_channel {nullptr};
    JsContent *m_jsContent {nullptr};
    QTimer *m_updateTimer {nullptr};
    WebView *m_webView {nullptr};
    bool m_textChange {false};
    QString m_searchKey {""};
    QPoint m_menuPoint {};
    Menu m_menuType = MaxMenu;
    QVariant m_menuJson = {};

    //右键菜单
    VNoteRightMenu *m_pictureRightMenu {nullptr}; //图片右键菜单
    VNoteRightMenu *m_voiceRightMenu {nullptr}; //语音右键菜单
    VNoteRightMenu *m_txtRightMenu {nullptr}; //文字右键菜单
};

#endif // RICHTEXTEDIT_H
