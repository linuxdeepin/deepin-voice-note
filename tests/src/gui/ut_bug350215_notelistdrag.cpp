// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// BUG-350215: dragging a note onto another notebook did nothing. The drag moved
// the whole window instead, and the drag hint stayed on screen.
//
// The fix keeps the note list's drag state list-local and resets it when the
// pointer is taken away, so a completed drag emits exactly one dropRelease that
// MainWindow turns into a moveNotesToFolderId call, while a cancelled drag emits
// none and a plain press still keeps DTK from moving the window.
//
// These tests drive the real ItemListView and assert that contract.

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
#include <QSignalSpy>
#include <QTimer>
#include <QUrl>
#include <QVariant>
#include <QVariantList>

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

// Fixture for the real note list. The component owns its internal ListModel, so
// the fixture appends through the component: assigning `model:` from outside
// would replace that binding and leave the list empty.
class NoteListHarness
{
public:
    bool load(QQmlEngine *engine)
    {
        const QString qml = QStringLiteral(
                                "import QtQuick 2.15\n"
                                "import QtQuick.Window 2.15\n"
                                "import \"%1\" as MW\n"
                                "Window {\n"
                                "    width: 500; height: 600; visible: true\n"
                                "    property bool listPointerDown: false\n"
                                "    property alias view: view\n"
                                "    function addNote(name, noteId) {\n"
                                "        view.model.append({name: name, time: \"2026-01-01\", noteId: noteId,\n"
                                "                          folderId: 1, folderName: \"alpha\", icon: \"default\"});\n"
                                "    }\n"
                                "    MW.ItemListView { id: view; anchors.fill: parent }\n"
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
        QMetaObject::invokeMethod(m_window, "addNote", Q_ARG(QVariant, QStringLiteral("note-a")), Q_ARG(QVariant, 11));
        return waitFor([this]() { return delegate() != nullptr; });
    }

    ~NoteListHarness()
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

    QQuickItem *delegate() const { return findByProperty(m_window->contentItem(), QStringLiteral("startMove")); }
    QQuickItem *delegateMouseArea() const { return findByProperty(delegate(), QStringLiteral("held")); }
    QQuickItem *dragControl() const { return findByProperty(m_window->contentItem(), QStringLiteral("itemNumber")); }

    void selectFirstNote() const
    {
        QQuickItem *listView = view();
        listView->setProperty("selectedNoteItem", QVariantList{0});
        listView->setProperty("selectSize", 1);
        spin(20);
    }

private:
    std::unique_ptr<QQmlComponent> m_component;
    QQuickWindow *m_window = nullptr;
    QString m_error;
};

// A note drag must be owned by the list for the whole press, so DTK cannot
// reinterpret it as a window move.
TEST(NoteListDrag, BUG350215_NotePressDisablesWindowMove)
{
    QQmlEngine *engine = sharedEngine();
    NoteListHarness harness;
    ASSERT_TRUE(harness.load(engine)) << harness.error().toStdString();
    harness.selectFirstNote();

    QQuickWindow *window = harness.window();
    QQuickItem *mouseArea = harness.delegateMouseArea();
    ASSERT_NE(nullptr, mouseArea);

    const QPointF point(mouseArea->width() / 2, mouseArea->height() / 2);
    press(mouseArea, point);
    EXPECT_TRUE(window->property("listPointerDown").toBool());

    release(mouseArea, point);
    EXPECT_FALSE(window->property("listPointerDown").toBool());
}

// A completed drag moves the note: MainWindow listens for dropRelease, so the
// list must emit it exactly once, and only after the pointer actually moved.
TEST(NoteListDrag, BUG350215_DragReleaseEmitsSingleDrop)
{
    QQmlEngine *engine = sharedEngine();
    NoteListHarness harness;
    ASSERT_TRUE(harness.load(engine)) << harness.error().toStdString();
    harness.selectFirstNote();

    QQuickWindow *window = harness.window();
    QQuickItem *view = harness.view();
    QQuickItem *mouseArea = harness.delegateMouseArea();
    QQuickItem *dragControl = harness.dragControl();
    ASSERT_NE(nullptr, mouseArea);
    ASSERT_NE(nullptr, dragControl);

    QSignalSpy dropSpy(view, SIGNAL(dropRelease()));
    ASSERT_TRUE(dropSpy.isValid());

    const QPointF start(mouseArea->width() / 2, mouseArea->height() / 2);
    press(mouseArea, start);
    moveTo(mouseArea, start + QPointF(0, 5));
    EXPECT_EQ(0, dropSpy.count()) << "a sub-threshold move must not drop";
    EXPECT_FALSE(dragControl->property("visible").toBool());

    moveTo(mouseArea, start + QPointF(0, 60));
    ASSERT_TRUE(dragControl->property("visible").toBool())
        << "dragging a note past the threshold must show the preview";

    release(mouseArea, start + QPointF(0, 60));
    EXPECT_EQ(1, dropSpy.count()) << "a note drag must emit exactly one dropRelease";
    EXPECT_FALSE(dragControl->property("visible").toBool());
    EXPECT_FALSE(window->property("listPointerDown").toBool());
}

// A cancelled drag must move nothing and must not leave the hint on screen.
TEST(NoteListDrag, BUG350215_CancelDoesNotDrop)
{
    QQmlEngine *engine = sharedEngine();
    NoteListHarness harness;
    ASSERT_TRUE(harness.load(engine)) << harness.error().toStdString();
    harness.selectFirstNote();

    QQuickWindow *window = harness.window();
    QQuickItem *view = harness.view();
    QQuickItem *mouseArea = harness.delegateMouseArea();
    QQuickItem *dragControl = harness.dragControl();
    ASSERT_NE(nullptr, mouseArea);
    ASSERT_NE(nullptr, dragControl);

    QSignalSpy dropSpy(view, SIGNAL(dropRelease()));
    ASSERT_TRUE(dropSpy.isValid());

    const QPointF start(mouseArea->width() / 2, mouseArea->height() / 2);
    press(mouseArea, start);
    moveTo(mouseArea, start + QPointF(0, 60));
    ASSERT_TRUE(dragControl->property("visible").toBool());

    cancelDrag(mouseArea);
    EXPECT_EQ(0, dropSpy.count()) << "a cancelled drag must not move the note";
    EXPECT_FALSE(dragControl->property("visible").toBool())
        << "a cancelled drag must tear the preview down";
    EXPECT_FALSE(window->property("listPointerDown").toBool());
}

} // namespace
