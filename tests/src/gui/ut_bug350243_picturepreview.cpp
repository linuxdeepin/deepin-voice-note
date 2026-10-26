// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// BUG-350243: the picture preview's close button was drawn outside the image
// area, and on Wayland it could end up outside the preview window entirely.
//
// The fix anchors the button on the image's top-right corner and binds both the
// fitted image size and the clamped button position to the real window size.
// These tests load the real ViewPictureDialog and assert the rendered geometry
// for the image/window combinations that used to fail.
//
// Covered branches of ViewPictureDialog.qml:
//   * fitScale clamped to 1 (image smaller than 80% of the window)
//   * fitScale < 1 (image larger than the window)
//   * x/y clamped at the window edges for very small windows

#include <gtest/gtest.h>

#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QImage>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>

#include <functional>

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

// The QML singleton providers hand out application-lifetime objects, and a
// QQmlEngine deletes what it creates. One shared engine keeps those singletons
// valid across the whole test binary.
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

QQuickItem *findByObjectName(QQuickItem *root, const QString &objectName)
{
    if (!root)
        return nullptr;
    if (root->objectName() == objectName)
        return root;
    const auto children = root->childItems();
    for (QQuickItem *child : children) {
        if (QQuickItem *match = findByObjectName(child, objectName))
            return match;
    }
    return nullptr;
}

QString writeTestImage(const QSize &size, const QColor &color)
{
    static QTemporaryDir dir;
    static int counter = 0;
    const QString path = dir.filePath(QStringLiteral("image-%1-%2x%3.png")
                                          .arg(++counter)
                                          .arg(size.width())
                                          .arg(size.height()));
    QImage image(size, QImage::Format_ARGB32);
    image.fill(color);
    if (!image.save(path))
        return QString();
    return path;
}

void verifyPreviewGeometry(const QSize &windowSize, const QSize &imageSize)
{
    QQmlEngine *engine = sharedEngine();
    QQmlComponent component(engine, QUrl::fromLocalFile(sourceFile(QStringLiteral("gui/dialog/ViewPictureDialog.qml"))));
    ASSERT_FALSE(component.isError()) << component.errorString().toStdString();

    QObject *created = component.create();
    ASSERT_NE(nullptr, created) << component.errorString().toStdString();
    QQuickWindow *window = qobject_cast<QQuickWindow *>(created);
    ASSERT_NE(nullptr, window);

    const QString imagePath = writeTestImage(imageSize, Qt::darkCyan);
    ASSERT_FALSE(imagePath.isEmpty()) << "could not write the test image";
    window->setProperty("filePath", imagePath);
    window->resize(windowSize);
    window->show();

    QQuickItem *image = findByObjectName(window->contentItem(), QStringLiteral("PreviewImage"));
    QQuickItem *close = findByObjectName(window->contentItem(), QStringLiteral("PreviewCloseButton"));
    ASSERT_NE(nullptr, image);
    ASSERT_NE(nullptr, close);

    // The dialog fits and centres the image only after a couple of layout
    // passes, so wait for the settled geometry instead of reading it eagerly.
    ASSERT_TRUE(waitFor([&]() {
        return image->property("status").toInt() == 1 // Image.Ready
            && image->property("x").toReal() >= 0
            && image->property("y").toReal() >= 0
            && image->property("y").toReal() + image->property("height").toReal() <= window->height() + 1;
    })) << "preview never settled";

    const qreal imageX = image->x();
    const qreal imageY = image->y();
    const qreal imageWidth = image->width();
    const qreal imageHeight = image->height();
    const qreal closeX = close->x();
    const qreal closeY = close->y();
    const qreal closeWidth = close->width();
    const qreal closeHeight = close->height();

    // The preview never overflows the window.
    EXPECT_LE(imageWidth, window->width() + 1);
    EXPECT_LE(imageHeight, window->height() + 1);

    // The button centre sits on the image's top-right corner. Before the fix the
    // button was placed a full button height above the image.
    EXPECT_NEAR(closeX + closeWidth / 2, imageX + imageWidth, 1.5)
        << "close button is not anchored on the image's right edge";
    EXPECT_NEAR(closeY + closeHeight / 2, imageY, 1.5)
        << "close button is not anchored on the image's top edge";

    // ...and the whole button stays inside the window even when clamping applies.
    EXPECT_GE(closeX, 0);
    EXPECT_GE(closeY, 0);
    EXPECT_LE(closeX + closeWidth, window->width() + 0.5);
    EXPECT_LE(closeY + closeHeight, window->height() + 0.5);

    window->close();
}

TEST(PicturePreviewCloseButton, BUG350243_StraddlesImageCorner)
{
    verifyPreviewGeometry(QSize(800, 600), QSize(640, 480));
}

TEST(PicturePreviewCloseButton, BUG350243_StaysInsideSmallWindow)
{
    verifyPreviewGeometry(QSize(500, 320), QSize(1600, 1200));
}

TEST(PicturePreviewCloseButton, BUG350243_FollowsTallImage)
{
    verifyPreviewGeometry(QSize(1000, 700), QSize(300, 2400));
}

TEST(PicturePreviewCloseButton, BUG350243_FollowsWideImage)
{
    verifyPreviewGeometry(QSize(1000, 700), QSize(3200, 200));
}

} // namespace
