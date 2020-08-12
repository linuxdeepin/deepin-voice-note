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

#include <QVBoxLayout>
#include <QList>
#include <QShortcut>
#include <QMultiMap>
#include <QList>

#include <DWidget>
#include <DDialog>

#include "textnoteedit.h"
#include "vtextspeechandtrmanager.h"

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
    void initData(VNoteItem *data, QString reg, bool fouse = false);
    DetailItemWidget *insertVoiceItem(const QString &voicePath, qint64 voiceSize);
    DetailItemWidget *insertTextEdit(VNoteBlock *data, bool focus = false,
                                     QTextCursor::MoveOperation op = QTextCursor::NoMove,
                                     QString reg = "");

    void setEnablePlayBtn(bool enable);
    void delWidget(DetailItemWidget *widget, bool merge = true);
    void saveNote();
    void updateData();
    void mouseMoveSelect(QMouseEvent *event);
    void selectAllItem();
    void clearAllSelection();
    void pasteText();
    void removeSelectWidget(DetailItemWidget *widget);

    int initAction(DetailItemWidget *widget);

    bool isAllWidgetEmpty(const QList<DetailItemWidget *> &widget);
    DetailItemWidget *getOnlyOneSelectVoice();

    QString getSelectText(bool voiceText = true);

    void copySelectText(bool voiceText = true);
    void cutSelectText(); //剪切文本
    void delSelectText();

    void setCurVoicePlay(VoiceNoteItem *item);
    void setCurVoiceAsr(VoiceNoteItem *item);
    void saveMp3();
    void closeMenu();
    void removeCacheWidget(VNoteItem *data);
    void removeCacheWidget(const VNoteFolder *data);

    VoiceNoteItem *getCurVoicePlay();
    VoiceNoteItem *getCurVoiceAsr();
    int showWarningDialog();

signals:
    void sigVoicePlay(VNVoiceBlock *voiceData);
    void sigVoicePause(VNVoiceBlock *voiceData);
    void contentChanged();
    void sigCursorChange(int height, bool mouseMove);
public slots:
    void onTextEditFocusOut();
    void onTextEditTextChange();
    void onTextEditSelectChange();
    void onVoicePlay(VoiceNoteItem *item);
    void onVoicePause(VoiceNoteItem *item);
    void onPlayUpdate();

protected:
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void adjustVerticalScrollBar(QWidget *widget, int defaultHeight);

private:
    void initUi();
    void initConnection();
    void initMenu();
    void onMenuShow(DetailItemWidget *widget);

    bool checkFileExist(const QString &file);
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
};

#endif // RIGHTVIEW_H
