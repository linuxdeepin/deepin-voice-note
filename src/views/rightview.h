/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
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

#ifndef RIGHTVIEW_H
#define RIGHTVIEW_H

#include "textnoteedit.h"
#include "vtextspeechandtrmanager.h"

#include <DWidget>
#include <DDialog>

#include <QVBoxLayout>
#include <QList>
#include <QShortcut>
#include <QMultiMap>
#include <QList>

DWIDGET_USE_NAMESPACE
struct VNoteFolder;
struct VNoteItem;
struct VNoteBlock;
struct VNVoiceBlock;

class VoiceNoteItem;
class DetailItemWidget;

enum ItemWidgetType { VoicePlugin,
                      TextEditPlugin };
typedef QMultiMap<ItemWidgetType, DetailItemWidget *> MultiMapWidget;

class RightView : public DWidget
{
    Q_OBJECT
public:
    explicit RightView(QWidget *parent = nullptr);
    //加载数据
    void initData(VNoteItem *data, QString reg, bool fouse = false);
    //插入语音
    DetailItemWidget *insertVoiceItem(const QString &voicePath, qint64 voiceSize);
    //插入编辑框
    DetailItemWidget *insertTextEdit(VNoteBlock *data, bool focus = false,
                                     QTextCursor::MoveOperation op = QTextCursor::NoMove,
                                     QString reg = "");
    //设置语音播放按钮是否可用
    void setEnablePlayBtn(bool enable);
    //删除窗口
    void delWidget(DetailItemWidget *widget, bool merge = true);
    //更新数据
    void saveNote();
    void updateData();
    //鼠标移动选中
    void mouseMoveSelect(QMouseEvent *event);
    //选中所有
    void selectAllItem();
    //清除所有选中
    void clearAllSelection();
    //粘贴
    void pasteText();
    //从选中数据中移除一个窗口
    void removeSelectWidget(DetailItemWidget *widget);
    //处理右键菜单
    int initAction(DetailItemWidget *widget);
    //判断窗口内容是否为空
    bool isAllWidgetEmpty(const QList<DetailItemWidget *> &widget);

    //只有一个语音项选中
    DetailItemWidget *getOnlyOneSelectVoice();
    //获取选中文本
    QString getSelectText(bool voiceText = true);
    //复制选中
    void copySelectText(bool voiceText = true);
    //剪切选中
    void cutSelectText();
    //删除选中
    void delSelectText();
    //设置当前播放语音窗口
    void setCurVoicePlay(VoiceNoteItem *item);
    //设置当前转文字语音窗口
    void setCurVoiceAsr(VoiceNoteItem *item);
    //语音，保存mp3
    void saveMp3();
    //关闭右键菜单
    void closeMenu();
    //删除一个记事项的窗口缓存
    void removeCacheWidget(VNoteItem *data);
    //删除一个记事本的窗口缓存
    void removeCacheWidget(const VNoteFolder *data);
    //获取当前播放语音窗口
    VoiceNoteItem *getCurVoicePlay();
    //获取当前转文字语音窗口
    VoiceNoteItem *getCurVoiceAsr();
    //操作是否需要弹出提示框
    int showWarningDialog();
    //更新详情页状态
    void setIsNormalView(bool value);
    //详情页是否为当前笔记
    bool getIsNormalView() const;
    //弹出右键菜单
    void popupMenu();
    //刷新语音插件创建时间
    void refreshVoiceCreateTime();
signals:
    //播放信号
    void sigVoicePlay(VNVoiceBlock *voiceData);
    //暂停信号
    void sigVoicePause(VNVoiceBlock *voiceData);
    //内容更新
    void contentChanged();
    //滚动条调整
    void sigCursorChange(int height, bool mouseMove);
public slots:
    //编辑框获取焦点
    void onTextEditFocusIn(Qt::FocusReason reason);
    //编辑框失去焦点
    void onTextEditFocusOut();
    //编辑框内容改变
    void onTextEditTextChange();
    //编辑框选中改变
    void onTextEditSelectChange();
    //播放语音
    void onVoicePlay(VoiceNoteItem *item);
    //暂停播放
    void onVoicePause(VoiceNoteItem *item);
    //更新语音播放动画
    void onPlayUpdate();

protected:
    //鼠标与按键事件
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    //调整滚动条
    void adjustVerticalScrollBar(QWidget *widget, int defaultHeight);
    void adjustVoiceVerticalScrollBar(DetailItemWidget *widget, int defaultHeight);

private:
    //初始化布局
    void initUi();
    //连接槽函数
    void initConnection();
    //初始化右键菜单
    void initMenu();
    //弹出菜单
    void onMenuShow(DetailItemWidget *widget);
    //判断语音文件是否存在
    bool checkFileExist(const QString &file);
    //通过坐标获取窗口
    DetailItemWidget *getWidgetByPos(const QPoint &pos);

    VoiceNoteItem *m_curPlayItem {nullptr};
    VoiceNoteItem *m_curAsrItem {nullptr};

    VNoteItem *m_noteItemData {nullptr};
    DetailItemWidget *m_curItemWidget {nullptr};
    DWidget *m_placeholderWidget {nullptr};
    QVBoxLayout *m_viewportLayout {nullptr};
    //Voice control context menu
    DMenu *m_noteDetailContextMenu {nullptr};
    DDialog *m_fileHasDelDialog {nullptr};
    bool m_fIsNoteModified {false};
    bool m_isFristTextChange {false};

    QTimer *m_playAnimTimer {nullptr};

    MultiMapWidget m_selectWidget;
    QString m_searchKey {""};

    QMap<VNoteBlock *, DetailItemWidget *> m_mapWidgetCache;
    //详情页是否为当前笔记
    bool m_isNormalView {true};
};

#endif // RIGHTVIEW_H
