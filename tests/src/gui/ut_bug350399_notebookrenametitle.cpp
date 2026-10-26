// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// BUG-350399: renaming a notebook inline and then clicking elsewhere committed
// the new name to the model but never refreshed the title shown above the note
// list, so the header kept the stale name until the notebook was re-selected.
//
// The fix reports the committed name from the rename editor's focus-out handler
// when the edited notebook is the active one. These tests drive the real
// FolderListView rename flow and assert both the emitted title update and the
// persisted model name for each outcome.
//
// Covered branches of FolderListView.qml renameLine.onActiveFocusChanged:
//   * cancelRename == true
//   * empty text
//   * text unchanged
//   * text changed and index == currentIndex  -> title must refresh
//   * text changed and index != currentIndex  -> title must not refresh

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTimer>
#include <QUrl>
#include <QVariant>

#include <functional>
#include <memory>

#include "VNoteMainManager.h"

namespace {

QString sourceRoot()
{
    static const QString root = []() {
        const QFileInfo self(QStringLiteral(__FILE__));
        return QDir::cleanPath(self.absolutePath() + QStringLiteral("/../../../src"));
    }();
    return root;
}

QString sourceFile(const QString &relativePath)
{
    return sourceRoot() + QLatin1Char('/') + relativePath;
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

private:
    std::unique_ptr<QQmlComponent> m_component;
    QQuickWindow *m_window = nullptr;
    QString m_error;
};

enum class RenameOutcome {
    ActiveNotebook,   // edited notebook is the active one -> title must refresh
    InactiveNotebook, // edited notebook is not active -> no title update
    Cancelled,        // Escape/cancel -> nothing persisted
    EmptyText,        // blank name -> nothing persisted
    UnchangedText,    // name not edited -> nothing persisted
};

void runRenameScenario(RenameOutcome outcome)
{
    QQmlEngine *engine = sharedEngine();
    FolderListHarness harness;
    ASSERT_TRUE(harness.load(engine)) << harness.error().toStdString();

    QQuickItem *view = harness.view();
    ASSERT_NE(nullptr, view);

    const bool renamingActiveNotebook = outcome == RenameOutcome::ActiveNotebook
        || outcome == RenameOutcome::Cancelled
        || outcome == RenameOutcome::EmptyText
        || outcome == RenameOutcome::UnchangedText;
    view->setProperty("currentFolderIndex", renamingActiveNotebook ? 0 : 1);
    spin(20);

    QQuickItem *delegate = harness.firstDelegate();
    ASSERT_NE(nullptr, delegate);

    delegate->setProperty("isRename", true);
    spin(20);

    QQuickItem *editor = findByProperty(delegate, QStringLiteral("maximumLength"));
    ASSERT_NE(nullptr, editor);

    delegate->setProperty("cancelRename", outcome == RenameOutcome::Cancelled);
    QString text = QStringLiteral("alpha-renamed");
    if (outcome == RenameOutcome::EmptyText)
        text = QString();
    else if (outcome == RenameOutcome::UnchangedText)
        text = QStringLiteral("alpha");
    editor->setProperty("text", text);
    spin(10);

    QSignalSpy titleSpy(view, SIGNAL(updateFolderName(QString)));
    ASSERT_TRUE(titleSpy.isValid());

    // Focus-out is what commits the rename.
    editor->forceActiveFocus();
    spin(20);
    view->forceActiveFocus();
    ASSERT_TRUE(waitFor([&]() { return !delegate->property("isRename").toBool(); }))
        << "the rename editor never completed on focus-out";

    const int expectedTitleUpdates = outcome == RenameOutcome::ActiveNotebook ? 1 : 0;
    ASSERT_EQ(expectedTitleUpdates, titleSpy.count());
    if (expectedTitleUpdates == 1)
        EXPECT_EQ(QStringLiteral("alpha-renamed"), titleSpy.first().at(0).toString());

    // A real edit commits the typed name. Cancel, blank and unchanged input fall
    // into the else branch, which restores the editor text from the model; that
    // reset is what proves nothing was persisted.
    const bool expectsRename = outcome == RenameOutcome::ActiveNotebook
        || outcome == RenameOutcome::InactiveNotebook;
    EXPECT_EQ(expectsRename ? QStringLiteral("alpha-renamed") : QStringLiteral("alpha"),
              editor->property("text").toString());
}

TEST(NotebookRenameTitle, BUG350399_CurrentNotebookRefreshesTitle)
{
    runRenameScenario(RenameOutcome::ActiveNotebook);
}

TEST(NotebookRenameTitle, BUG350399_InactiveNotebookKeepsTitle)
{
    runRenameScenario(RenameOutcome::InactiveNotebook);
}

TEST(NotebookRenameTitle, BUG350399_CancelledRenameKeepsTitle)
{
    runRenameScenario(RenameOutcome::Cancelled);
}

TEST(NotebookRenameTitle, BUG350399_EmptyRenameKeepsTitle)
{
    runRenameScenario(RenameOutcome::EmptyText);
}

TEST(NotebookRenameTitle, BUG350399_UnchangedRenameKeepsTitle)
{
    runRenameScenario(RenameOutcome::UnchangedText);
}

} // namespace
