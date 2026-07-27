// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TiptapTempBridge_H
#define TiptapTempBridge_H

#include <QObject>

// TTP-022: 临时最小 Tiptap 通道（命名不稳定，TTP-023 将替换为正式通道）。
// 仅提供「加载 envelope + 取回保存 envelope」两件事，不触碰 JsContent/note 存储。
class TiptapTempBridge : public QObject
{
    Q_OBJECT
public:
    explicit TiptapTempBridge(QObject *parent = nullptr);

    // 读 DVN_TIPTAP_DEBUG 环境变量，便于 QA 与 GTest。
    Q_INVOKABLE bool debugEnabled() const;

    // 返回 Tiptap 运行时 HTML 路径（首选 qrc，install 回退 file://）。
    Q_INVOKABLE QString tiptapHtmlPath() const;

signals:
    // C++ → JS：加载 envelope
    void callLoadEnvelope(const QString &json);
    // C++ → JS：请求保存
    void callRequestContent();

public slots:
    // JS → C++：编辑器初始化完成
    Q_INVOKABLE void jsEditorReady();
    // JS → C++：保存 envelope 回传
    Q_INVOKABLE void jsContentSaved(const QString &envelopeJson);
};

#endif // TiptapTempBridge_H
