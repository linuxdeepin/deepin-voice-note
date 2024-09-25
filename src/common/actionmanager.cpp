// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "actionmanager.h"

#include <QVariant>
#include <QCoreApplication>

// 用于动态设置 QML 组件的属性
const char EnabledProperty[] = "enabled";
const char VisibleProperty[] = "visible";

/*!
 * \class ActionManager
 * \brief 动作项管理，结合 QML 组件 VNoteRightMenu 使用
 *  ActionManager 管理动作项元信息，通过 VNoteRightMenu 设置的不同菜单类型 MenuType
 *  动态创建菜单项，并支持通过信号抛出按键点击事件。
 * \warning 单个菜单类型仅能同时存在一个菜单实例
 */

ActionManager::ActionManager(QObject *parent)
    : QObject { parent }
{
    initMenu();
}

ActionManager *ActionManager::instance()
{
    static ActionManager ins;
    return &ins;
}

/*!
 * \brief 初始化菜单项元信息，含菜单项的标识和默认显示文本
 */
void ActionManager::initMenu()
{
    static auto makeAction = [this](ActionKind id, const QString &text, ComponentType type = MenuItemComponent) -> ActionPtr {
        ActionPtr meta = ActionPtr::create(id, text, type);
        metaData.insert(id, meta);
        return meta;
    };

    // NotebookCtxMenu 记事本页面右键菜单
    makeAction(NotebookRename, QCoreApplication::translate("NotebookContextMenu", "Rename"));
    makeAction(NotebookDelete, QCoreApplication::translate("NotebookContextMenu", "Delete"));
    makeAction(NotebookAddNew, QCoreApplication::translate("NotebookContextMenu", "New note"));

    // NoteCtxMenu 笔记本页面右键菜单
    makeAction(NoteRename, QCoreApplication::translate("NotesContextMenu", "Rename"));
    makeAction(NoteTop, QCoreApplication::translate("NotesContextMenu", "Sticky on Top"));
    makeAction(NoteMove, QCoreApplication::translate("NotesContextMenu", "Move"));
    makeAction(NoteDelete, QCoreApplication::translate("NotesContextMenu", "Delete"));
    makeAction(NoteSeparator, {}, MenuSeparatorComponent);
    makeAction(NoteSave, QCoreApplication::translate("NotesContextMenu", "Save note"), MenuComponent);
    makeAction(NoteSaveVoice, QCoreApplication::translate("NotesContextMenu", "Save voice recording"));
    makeAction(NoteSeparator, {}, MenuSeparatorComponent);
    makeAction(NoteAddNew, QCoreApplication::translate("NotesContextMenu", "New note"));

    // SaveNoteCtxMenu 保存笔记二级菜单项文案
    makeAction(SaveNoteAsHtml, QCoreApplication::translate("NotesContextMenu", "Save as HTML"));
    makeAction(SaveNoteAsText, QCoreApplication::translate("NotesContextMenu", "Save as TXT"));
    // 设置子菜单项信息
    childActionMap.insert(NoteSave, { SaveNoteAsHtml, SaveNoteAsText });

    // VoiceCtxMenu 初始化语音文本右键菜单
    makeAction(VoiceAsSave, QCoreApplication::translate("NoteDetailContextMenu", "Save as MP3"));
    makeAction(VoiceToText, QCoreApplication::translate("NoteDetailContextMenu", "Voice to Text"));
    makeAction(VoiceDelete, QCoreApplication::translate("NoteDetailContextMenu", "Delete"));
    makeAction(VoiceSelectAll, QCoreApplication::translate("NoteDetailContextMenu", "Select all"));
    makeAction(VoiceCopy, QCoreApplication::translate("NoteDetailContextMenu", "Copy"));
    makeAction(VoiceCut, QCoreApplication::translate("NoteDetailContextMenu", "Cut"));
    makeAction(VoicePaste, QCoreApplication::translate("NoteDetailContextMenu", "Paste"));

    // PictureCtxMenu 初始化图片文本右键菜单
    makeAction(PictureView, QCoreApplication::translate("NoteDetailContextMenu", "View"));
    makeAction(PictureDelete, QCoreApplication::translate("NoteDetailContextMenu", "Delete"));
    makeAction(PictureSelectAll, QCoreApplication::translate("NoteDetailContextMenu", "Select all"));
    makeAction(PictureCopy, QCoreApplication::translate("NoteDetailContextMenu", "Copy"));
    makeAction(PictureCut, QCoreApplication::translate("NoteDetailContextMenu", "Cut"));
    makeAction(PicturePaste, QCoreApplication::translate("NoteDetailContextMenu", "Paste"));
    makeAction(PictureSaveAs, QCoreApplication::translate("NoteDetailContextMenu", "Save as"));

    // TxtCtxMenu 初始化文字文本右键菜单
    makeAction(TxtDelete, QCoreApplication::translate("NoteDetailContextMenu", "Delete"));
    makeAction(TxtSelectAll, QCoreApplication::translate("NoteDetailContextMenu", "Select all"));
    makeAction(TxtCopy, QCoreApplication::translate("NoteDetailContextMenu", "Copy"));
    makeAction(TxtCut, QCoreApplication::translate("NoteDetailContextMenu", "Cut"));
    makeAction(TxtPaste, QCoreApplication::translate("NoteDetailContextMenu", "Paste"));
    makeAction(TxtSeparator, {}, MenuSeparatorComponent);
    makeAction(TxtSpeech, QCoreApplication::translate("NoteDetailContextMenu", "Text to Speech"));
    makeAction(TxtStopreading, QCoreApplication::translate("NoteDetailContextMenu", "Stop reading"));
    makeAction(TxtDictation, QCoreApplication::translate("NoteDetailContextMenu", "Speech to Text"));
    makeAction(TxtTranslate, QCoreApplication::translate("NoteDetailContextMenu", "Translate"));
}

/*!
 * \return 获取菜单项 \a id 对应的 QML 组件
 */
QObject *ActionManager::getActionById(ActionKind id)
{
    auto ptr = metaData.value(id);
    return ptr ? ptr->qmlObject : nullptr;
}

/*!
 * \brief 设置动作项 \a actionId 使能属性为 \a enable
 */
void ActionManager::enableAction(ActionKind actionId, bool enable)
{
    if (!metaData.contains(actionId)) {
        return;
    }

    auto ptr = metaData.value(actionId);
    if (ptr && ptr->qmlObject) {
        ptr->qmlObject->setProperty(EnabledProperty, QVariant(enable));
    }
}

/*!
 * \brief 设置动作项 \a actionId 显示属性为 \a visible
 */
void ActionManager::visibleAction(ActionKind actionId, bool visible)
{
    if (!metaData.contains(actionId)) {
        return;
    }

    auto ptr = metaData.value(actionId);
    if (ptr && ptr->qmlObject) {
        ptr->qmlObject->setProperty(VisibleProperty, QVariant(visible));
    }
}

/*!
 * \brief 设置 \a type 类型菜单的使能属性为 \a enable
 */
void ActionManager::resetCtxMenu(MenuType type, bool enable)
{
    int startMenuId = MenuMaxId;
    int endMenuId = MenuMaxId;

    if (MenuType::NotebookCtxMenu == type) {
        startMenuId = NotebookMenuBase;
        endMenuId = NotebookMenuMax;
    } else if (MenuType::NoteCtxMenu == type) {
        startMenuId = NoteMenuBase;
        endMenuId = NoteMenuMax;
    } else if (MenuType::VoiceCtxMenu == type) {
        startMenuId = VoiceMenuBase;
        endMenuId = VoiceMenuMax;
    } else if (MenuType::PictureCtxMenu == type) {
        startMenuId = PictureMenuBase;
        endMenuId = PictureMenuMax;
    } else if (MenuType::TxtCtxMenu == type) {
        startMenuId = TxtMenuBase;
        endMenuId = TxtMenuMax;
    } else if (MenuType::SaveNoteCtxMenu == type) {
        startMenuId = SaveNoteMenuBase;
        endMenuId = SaveNoteMax;
    }

    for (; startMenuId < endMenuId; ++startMenuId) {
        auto ptr = metaData.value(static_cast<ActionKind>(startMenuId));
        if (ptr && ptr->qmlObject) {
            ptr->qmlObject->setProperty(EnabledProperty, enable);
        }
    }
}

/*!
 * \brief 设置 AI 相关动作项目显示属性为 \a visible
 */
void ActionManager::visibleAiActions(bool visible)
{
    visibleAction(VoiceToText, visible);
    visibleAction(TxtSpeech, visible);
    visibleAction(TxtStopreading, visible);
    visibleAction(TxtDictation, visible);
    visibleAction(TxtTranslate, visible);
    visibleAction(TxtSeparator, visible);
}

void ActionManager::visibleMulChoicesActions(bool visible)
{
    visibleAction(NoteRename, visible);
    visibleAction(NoteTop, visible);
    enableAction(NoteSaveVoice, visible);
}

/*!
 * \brief 使用 model 同样需要多次查询，减少使用中间类，因此直接通过接口访问而没有使用
 *      QAbstractListModel 设计
 * \return 返回动作项 \a id 对应的文本
 */
QString ActionManager::actionText(ActionKind id)
{
    auto ptr = metaData.value(id);
    return ptr ? ptr->text : QString();
}

/*!
 * \return 返回动作项 \a id 对应的 QML 组件类型
 */
ActionManager::ComponentType ActionManager::actionCompType(ActionKind id)
{
    auto ptr = metaData.value(id);
    return ptr ? ptr->type : MenuItemComponent;
}

/*!
 * \brief 返回菜单项 \a id 的子菜单项列表
 */
QList<ActionManager::ActionKind> ActionManager::childActions(ActionKind id)
{
    return childActionMap.value(id);
}

/*!
 * \brief 设置动作项 \a id 对应的 QML 组件
 */
void ActionManager::setActionObject(ActionKind id, QObject *obj)
{
    auto ptr = metaData.value(id);
    if (ptr) {
        if (ptr->qmlObject) {
            qCritical() << "Duplicate qml objects, create multiple menus of the same type?" << id << ptr->qmlObject;
        }
        // 不能同时创建多个相同类型菜单
        Q_ASSERT_X(!ptr->qmlObject, "Set action qml object",
                   "Duplicate qml objects, create multiple menus of the same type?");
        ptr->qmlObject = obj;
    }
}

/*!
 * \brief QML 组件触发动作项
 */
void ActionManager::actionTriggerFromQuick(ActionKind actionId)
{
    Q_EMIT actionTriggered(actionId);
}
