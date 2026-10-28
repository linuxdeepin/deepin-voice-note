// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// BUG-350203: dragging a notebook to reorder it did nothing, and instead moved
// the whole application window while the drag hint stayed on screen.
//
// Two things had to change on the compositor side (Treeland):
//   * the notebook list marks the pointer as list-owned while a button is held,
//     so DTK does not turn the drag into a window move;
//   * the drag preview is a plain item inside the main window. It used to be a
//     separate always-on-top window, which took the pointer release away from
//     the list and left the drag/hint state dangling.
//
// These tests drive the real FolderListView and assert the pointer contract plus
// the drag threshold that gates the preview.

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QMouseEvent>
#include <QPointF>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>
#include <QUrl>
#include <QVariant>

#include <functional>
#include <memory>

#include "VNoteMainManager.h"

namespace {

QString sourceFile(const QString &relativePath)
{
    static const QString root = []() {
        const QFileInfo self(QStringLiteral(__FILE__));
        return QDir::cleanPath(self.absolutePath() + QStringLiteral("/../../../src"));
    }();
    return root + QLatin1Char('/') + relativePath;
}

QString sourceUrl(const QString &relativePath)
{
    return QUrl::fromLocalFile(sourceFile(relativePath)).toString();
}

// One shared engine: the QML singleton providers return application-lifetime
// objects, and an engine deletes what it creates.
QQmlEngine *sharedEngine()
{
    static QQmlEngine *engine = []() {
        VNoteMainManager::instance()->initQMLRegister();
        auto *created = new QQmlEngine;
        QQmlEngine::setObjectOwnership(VNoteMainManager::instance(), QQmlEngine::CppOwnership);
        return created;
    }();
    return engine;
}

void spin(int milliseconds = 20)
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(milliseconds);
    loop.exec();
}

bool waitFor(const std::function<bool()> &predicate, int timeoutMs = 3000)
{
    QElapsedTimer elapsed;
    elapsed.start();
    while (elapsed.elapsed() < timeoutMs) {
        if (predicate())
            return true;
        spin(10);
    }
    return predicate();
}

void collectItems(QQuickItem *root, QList<QQuickItem *> *collected)
{
    if (!root)
        return;
    collected->append(root);
    const auto children = root->childItems();
    for (QQuickItem *child : children)
        collectItems(child, collected);
}

QQuickItem *findByProperty(QQuickItem *root, const QString &propertyName)
{
    QList<QQuickItem *> all;
    collectItems(root, &all);
    const QByteArray name = propertyName.toUtf8();
    for (QQuickItem *item : all) {
        if (item->metaObject()->indexOfProperty(name.constData()) >= 0)
            return item;
    }
    return nullptr;
}

QList<QQuickItem *> findAllByProperty(QQuickItem *root, const QString &propertyName)
{
    QList<QQuickItem *> all;
    collectItems(root, &all);
    QList<QQuickItem *> matches;
    const QByteArray name = propertyName.toUtf8();
    for (QQuickItem *item : all) {
        if (item->metaObject()->indexOfProperty(name.constData()) >= 0)
            matches.append(item);
    }
    return matches;
}

// Pointer events go straight to the delegate's MouseArea. Routing them through
// QQuickWindow would need a platform implicit grab that the offscreen test
// backend does not maintain, and the spurious cancel that follows would mask the
// behaviour under test.
void sendMouseToItem(QQuickItem *target, QEvent::Type type, const QPointF &localPosition,
                     Qt::MouseButtons buttons)
{
    QMouseEvent event(type, localPosition, target->mapToGlobal(localPosition),
                      Qt::LeftButton, buttons, Qt::NoModifier);
    QCoreApplication::sendEvent(target, &event);
    spin(5);
}

void press(QQuickItem *target, const QPointF &localPosition)
{
    sendMouseToItem(target, QEvent::MouseButtonPress, localPosition, Qt::LeftButton);
}

void moveTo(QQuickItem *target, const QPointF &localPosition)
{
    sendMouseToItem(target, QEvent::MouseMove, localPosition, Qt::LeftButton);
}

void release(QQuickItem *target, const QPointF &localPosition)
{
    sendMouseToItem(target, QEvent::MouseButtonRelease, localPosition, Qt::NoButton);
}

// The offscreen platform keeps no implicit grab, so neither ungrabMouse() nor a
// synthetic UngrabMouse event reaches the cancel handler. Emitting the signal
// drives exactly the handler the component installs for a stolen pointer.
void cancelDrag(QQuickItem *mouseArea)
{
    QMetaObject::invokeMethod(mouseArea, "canceled");
    spin(20);
}

// Fixture for the real notebook list. The component owns its internal
// ListModel, so the fixture appends through the component: assigning `model:`
// from outside would replace that binding and leave the list empty.
class FolderListHarness
{
public:
    bool load(QQmlEngine *engine)
    {
        const QString qml = QStringLiteral(
                                "import QtQuick 2.15\n"
                                "import QtQuick.Window 2.15\n"
                                "import \"%1\" as MW\n"
                                "Window {\n"
                                "    width: 400; height: 600; visible: true\n"
                                "    property bool listPointerDown: false\n"
                                "    property alias view: view\n"
                                "    function addFolder(name, id) {\n"
                                "        view.model.append({name: name, count: 1, icon: \"default\", folderId: id});\n"
                                "    }\n"
                                "    MW.FolderListView { id: view; anchors.fill: parent }\n"
                                "}\n")
                                .arg(sourceUrl(QStringLiteral("gui/mainwindow")));
        m_component = std::make_unique<QQmlComponent>(engine);
        m_component->setData(qml.toUtf8(), QUrl::fromLocalFile(sourceFile(QStringLiteral("tests-harness.qml"))));
        if (m_component->isError()) {
            m_error = m_component->errorString();
            return false;
        }
        QObject *created = m_component->create();
        if (!created) {
            m_error = m_component->errorString();
            return false;
        }
        m_window = qobject_cast<QQuickWindow *>(created);
        if (!m_window) {
            m_error = QStringLiteral("root object is not a Window");
            delete created;
            return false;
        }
        m_window->show();
        QMetaObject::invokeMethod(m_window, "addFolder", Q_ARG(QVariant, QStringLiteral("alpha")), Q_ARG(QVariant, 1));
        QMetaObject::invokeMethod(m_window, "addFolder", Q_ARG(QVariant, QStringLiteral("beta")), Q_ARG(QVariant, 2));
        return waitFor([this]() { return delegates().size() >= 2; });
    }

    ~FolderListHarness()
    {
        // The component must outlive the window, otherwise Qt frees the QML
        // objects the window still references.
        if (m_window) {
            m_window->close();
            m_window->hide();
            spin(10);
            delete m_window;
            m_window = nullptr;
        }
    }

    QQuickItem *view() const { return m_window->property("view").value<QQuickItem *>(); }
    QQuickWindow *window() const { return m_window; }
    const QString &error() const { return m_error; }

    QList<QQuickItem *> delegates() const
    {
        return findAllByProperty(m_window->contentItem(), QStringLiteral("isRename"));
    }
    QQuickItem *firstDelegate() const { return delegates().value(0); }
    QQuickItem *firstDelegateMouseArea() const { return findByProperty(firstDelegate(), QStringLiteral("held")); }
    QQuickItem *dragControl() const { return findByProperty(m_window->contentItem(), QStringLiteral("itemNumber")); }

private:
    std::unique_ptr<QQmlComponent> m_component;
    QQuickWindow *m_window = nullptr;
    QString m_error;
};

// A notebook drag must be owned by the list for the whole press, so DTK cannot
// reinterpret it as a window move.
TEST(NotebookListDrag, BUG350203_ListPressDisablesWindowMove)
{
    QQmlEngine *engine = sharedEngine();
    FolderListHarness harness;
    ASSERT_TRUE(harness.load(engine)) << harness.error().toStdString();

    QQuickWindow *window = harness.window();
    QQuickItem *mouseArea = harness.firstDelegateMouseArea();
    ASSERT_NE(nullptr, mouseArea);
    ASSERT_FALSE(window->property("listPointerDown").toBool());

    const QPointF point(mouseArea->width() / 2, mouseArea->height() / 2);
    press(mouseArea, point);
    EXPECT_TRUE(window->property("listPointerDown").toBool())
        << "pressing the notebook list must disable the system window move";

    release(mouseArea, point);
    EXPECT_FALSE(window->property("listPointerDown").toBool())
        << "releasing inside the list must restore the window move";
}

// Losing the pointer (grab theft, window switch) must not leave the window
// permanently non-movable.
TEST(NotebookListDrag, BUG350203_CancelRestoresWindowMove)
{
    QQmlEngine *engine = sharedEngine();
    FolderListHarness harness;
    ASSERT_TRUE(harness.load(engine)) << harness.error().toStdString();

    QQuickWindow *window = harness.window();
    QQuickItem *mouseArea = harness.firstDelegateMouseArea();
    QQuickItem *dragControl = harness.dragControl();
    ASSERT_NE(nullptr, mouseArea);
    ASSERT_NE(nullptr, dragControl);

    const QPointF point(mouseArea->width() / 2, mouseArea->height() / 2);
    press(mouseArea, point);
    ASSERT_TRUE(window->property("listPointerDown").toBool());

    cancelDrag(mouseArea);
    ASSERT_TRUE(waitFor([&]() { return !window->property("listPointerDown").toBool(); }))
        << "cancel must restore the window move";
    EXPECT_FALSE(dragControl->property("visible").toBool())
        << "cancel must also tear the drag preview down";
}

// The preview used to be a separate always-on-top window. That window took the
// pointer release, so the list never saw it and the drag stayed active.
TEST(NotebookListDrag, BUG350203_DragPreviewStaysInTheMainWindow)
{
    QQmlEngine *engine = sharedEngine();
    FolderListHarness harness;
    ASSERT_TRUE(harness.load(engine)) << harness.error().toStdString();

    QQuickItem *dragControl = harness.dragControl();
    ASSERT_NE(nullptr, dragControl);
    EXPECT_EQ(nullptr, qobject_cast<QQuickWindow *>(dragControl))
        << "a separate preview window steals the pointer release";
    EXPECT_EQ(harness.window(), dragControl->window());
}

// The preview only appears once the move exceeds the drag threshold, which is
// what keeps a plain click from starting a drag.
TEST(NotebookListDrag, BUG350203_DragThresholdThenPreview)
{
    QQmlEngine *engine = sharedEngine();
    FolderListHarness harness;
    ASSERT_TRUE(harness.load(engine)) << harness.error().toStdString();

    QQuickWindow *window = harness.window();
    QQuickItem *dragControl = harness.dragControl();
    QQuickItem *mouseArea = harness.firstDelegateMouseArea();
    ASSERT_NE(nullptr, dragControl);
    ASSERT_NE(nullptr, mouseArea);

    harness.view()->setProperty("currentFolderIndex", 0);
    spin(20);

    const QPointF start(mouseArea->width() / 2, mouseArea->height() / 2);
    press(mouseArea, start);
    moveTo(mouseArea, start + QPointF(0, 5));
    EXPECT_FALSE(dragControl->property("visible").toBool())
        << "a 5px move is below the drag threshold";

    moveTo(mouseArea, start + QPointF(0, 40));
    EXPECT_TRUE(dragControl->property("visible").toBool())
        << "dragging past the threshold must show the in-window preview";

    // Cancel instead of dropping so the test never rewrites the folder order.
    cancelDrag(mouseArea);
    ASSERT_TRUE(waitFor([&]() { return !dragControl->property("visible").toBool(); }));
    EXPECT_FALSE(window->property("listPointerDown").toBool());
}

} // namespace
