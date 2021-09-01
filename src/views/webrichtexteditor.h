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
#ifndef WEBRICHTEXTEDITOR_H
#define WEBRICHTEXTEDITOR_H

#include "common/vnoteitem.h"

#include <QObject>
#include <QtWebChannel/QWebChannel>
#include <QtWebEngineWidgets/QWebEngineView>
#include <QWebEngineContextMenuData>
#include <QOpenGLWidget>

struct VNoteItem;
class VNoteRightMenu;
class ImageViewerDialog;
class WebRichTextEditor : public QWebEngineView
{
    Q_OBJECT
public:
    explicit WebRichTextEditor(QWidget *parent = nullptr);
    ~WebRichTextEditor();

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
     * @param focus: 焦点
     */
    void initData(VNoteItem *data, const QString &reg, bool focus = false);
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
     */
    void searchText(const QString &searchKey);

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

    /**
     * @brief 当前编辑区搜索内容为空
     */
    void currentSearchEmpty();

    /**
     * @brief 编辑区内容更新
     */
    void contentChanged();

public slots:
    /**
    * @brief 编辑区内容变化
    */
    void onTextChange();
    /**
    * @brief 粘贴
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
    * @brief 右键菜单项点击响应
    * @param action
    */
    void onMenuActionClicked(QAction *action);

    /**
     * @brief 删除选中内容
     */
    void deleteSelectText();
    /**
     * @brief 查看图片
     */
    void viewPicture(const QString &filePath);

    /**
     * @brief 系统主题发生改变处理
     */
    void onThemeChanged();

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;
    void dropEvent(QDropEvent *event) override;

private:
    /**
     * @brief 初始化数据更新定时器
     */
    void initUpdateTimer();
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
    /**
     * @brief 另存图片
     */
    void savePictureAs();
    /**
     * @brief 另存语音
     */
    void saveMP3As();

private:
    VNoteItem *m_noteData {nullptr};
    QTimer *m_updateTimer {nullptr};
    bool m_textChange {false};
    QString m_searchKey {""};
    QPoint m_menuPoint {};
    Menu m_menuType = MaxMenu;
    QVariant m_menuJson = {};
    ImageViewerDialog *imgView {nullptr}; //

    //右键菜单
    VNoteRightMenu *m_pictureRightMenu {nullptr}; //图片右键菜单
    VNoteRightMenu *m_voiceRightMenu {nullptr}; //语音右键菜单
    VNoteRightMenu *m_txtRightMenu {nullptr}; //文字右键菜单

    QScopedPointer<VNVoiceBlock> m_voiceBlock {nullptr}; //待另存的语音数据
};

#endif // WEBRICHTEXTEDITOR_H
