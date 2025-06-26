// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ACTIONMANAGER_H
#define ACTIONMANAGER_H

#include <QObject>
#include <QPointer>
#include <QMap>

class ActionManager : public QObject
{
    Q_OBJECT
public:
    // 右键菜单类型
    enum ActionKind {
        Invalid = 0,
        // Notepad
        NotebookMenuBase,
        NotebookRename = NotebookMenuBase,
        NotebookDelete,
        NotebookAddNew,
        // Add notebook menu item begin {

        // Add notebook menu item end }
        NotebookMenuMax,

        // notes
        NoteMenuBase,
        NoteRename = NoteMenuBase,
        NoteTop,
        NoteMove,
        NoteDelete,
        NoteSave,
        NoteSaveVoice,
        NoteSeparator,
        NoteAddNew,
        // Add note menu item begin {

        // Add note menu item end }
        NoteMenuMax,

        // Voice
        VoiceMenuBase,
        VoiceAsSave = VoiceMenuBase,
        VoiceToText,
        VoiceDelete,
        VoiceSelectAll,
        VoiceCopy,
        VoiceCut,
        VoicePaste,
        // Add voice menu item begin {

        // Add voice menu item end }
        VoiceMenuMax,

        // pciture
        PictureMenuBase,
        PictureView = PictureMenuBase,
        PictureDelete,
        PictureSelectAll,
        PictureCopy,
        PictureCut,
        PicturePaste,
        PictureSaveAs,
        // Add pciture menu item begin {

        // Add pciture menu item end }
        PictureMenuMax,

        // txt
        TxtMenuBase,
        TxtDelete = TxtMenuBase,
        TxtSelectAll,
        TxtCopy,
        TxtCut,
        TxtPaste,

        TxtSeparator,   // 分割线
        TxtSpeech,
        TxtStopreading,
        TxtDictation,
        // TxtTranslate,
        // Add Txt menu item begin {

        // Add Txt menu item end }
        TxtMenuMax,

        SaveNoteMenuBase,
        SaveNoteAsHtml = SaveNoteMenuBase,   // 保存为HTML
        SaveNoteAsText,   // 保存为txt
        SaveNoteMax,

        MenuMaxId
    };

    Q_ENUM(ActionKind)

    // Menu types
    enum MenuType {
        UnknownMenu,
        NotebookCtxMenu,
        NoteCtxMenu,
        VoiceCtxMenu,
        PictureCtxMenu,
        TxtCtxMenu,
        SaveNoteCtxMenu,   // 保存笔记二级菜单
    };
    Q_ENUM(MenuType)

    enum ComponentType {
        MenuItemComponent,
        MenuComponent,
        MenuSeparatorComponent,
    };
    Q_ENUM(ComponentType)

    static ActionManager *instance();

    // 获取菜单项对应的 QML 组件
    Q_INVOKABLE QObject *getActionById(ActionKind id);

    // 设置菜单项是否可用
    Q_INVOKABLE void enableAction(ActionKind actionId, bool enable);
    // 设置菜单项是否可见
    Q_INVOKABLE void visibleAction(ActionKind actionId, bool visible);
    // 重置菜单项可用状态
    Q_INVOKABLE void resetCtxMenu(MenuType type, bool enable = true);

    // 隐藏/显示语音助手相关功能
    Q_INVOKABLE void visibleAiActions(bool visible);
    Q_INVOKABLE void visibleMulChoicesActions(bool visible);

    //失能语音播放中相关功能
    Q_INVOKABLE void enableVoicePlayActions(bool enable);

    // 动作被触发
    Q_SIGNAL void actionTriggered(ActionKind actionId);

    // \internal 内部使用
    // 用于获取菜单项详细参数
    Q_INVOKABLE QString actionText(ActionKind id);
    Q_INVOKABLE ComponentType actionCompType(ActionKind id);
    // 获取子菜单项
    Q_INVOKABLE QVariantList childActions(ActionKind id);
    // 用于 QML 组件初始化和标记动作触发
    Q_INVOKABLE void setActionObject(ActionKind id, QObject *obj);
    Q_SLOT void actionTriggerFromQuick(ActionKind actionId);

private:
    explicit ActionManager(QObject *parent = nullptr);
    void initMenu();

private:
    // 菜单动作信息 后续按需增加快捷键等属性
    struct ActionMeta
    {
        ActionKind id;
        QString text;
        ComponentType type = MenuItemComponent;
        QPointer<QObject> qmlObject { nullptr };   // 和 QML 组件通信

        ActionMeta() = default;
        ActionMeta(ActionKind i, const QString &tex, ComponentType t = MenuItemComponent)
            : id(i), text(tex), type(t) { }
    };
    using ActionPtr = QSharedPointer<ActionMeta>;
    QMap<ActionKind, ActionPtr> metaData;   // 菜单项信息
    QMap<ActionKind, QList<ActionKind>> childActionMap;   // 子菜单项信息

    Q_DISABLE_COPY(ActionManager)
};

#endif   // ACTIONMANAGER_H
