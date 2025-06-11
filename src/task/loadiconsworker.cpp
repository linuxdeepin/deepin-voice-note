// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "loadiconsworker.h"
#include "common/vnotedatamanager.h"

#include <QPixmap>
#include <QPainter>
#include <DSvgRenderer>
#include <QImage>
#include <QDebug>

DGUI_USE_NAMESPACE

QVector<QPixmap> VNoteDataManager::m_defaultIcons[IconsType::MaxIconsType];
QReadWriteLock VNoteDataManager::m_iconLock;

/**
 * @brief LoadIconsWorker::LoadIconsWorker
 * @param parent
 */
LoadIconsWorker::LoadIconsWorker(QObject *parent)
    : VNTask(parent)
{
    //Hold the lock at constructor, this can void
    //other thread acess the icons before that being
    //loaded.
    VNoteDataManager::m_iconLock.lockForWrite();
}

/**
 * @brief LoadIconsWorker::greyPix
 * @param pix
 * @return 置灰后的图标
 */
QPixmap LoadIconsWorker::greyPix(QPixmap pix)
{
    // 输入验证
    if (pix.isNull() || pix.size().isEmpty()) {
        qWarning() << "LoadIconsWorker::greyPix: Invalid input pixmap";
        return QPixmap();
    }

#ifdef __mips64
    // MIPS64 特殊处理：避免 QPainter 相关的内存对齐问题
    qDebug() << "Using MIPS64-specific greyPix implementation";
    
    // 转换为 QImage 进行处理
    QImage image = pix.toImage();
    if (image.isNull()) {
        qWarning() << "Failed to convert QPixmap to QImage";
        return pix; // 返回原图像作为后备
    }
    
    // 确保图像格式兼容
    if (image.format() != QImage::Format_ARGB32_Premultiplied) {
        image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }
    
    // 手动处理像素以应用灰度效果
    int width = image.width();
    int height = image.height();
    
    for (int y = 0; y < height; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < width; ++x) {
            QRgb pixel = line[x];
            int alpha = qAlpha(pixel);
            
            // 应用灰度效果，降低透明度
            int newAlpha = alpha * 64 / 255; // 相当于原来的 QColor(0, 0, 0, 64) 效果
            line[x] = qRgba(qRed(pixel), qGreen(pixel), qBlue(pixel), newAlpha);
        }
    }
    
    return QPixmap::fromImage(image);
    
#else
    // 非 MIPS64 架构使用原来的实现
    QPixmap temp(pix.size());
    temp.fill(Qt::transparent);

    QPainter iconPainer(&temp);
    iconPainer.setCompositionMode(QPainter::CompositionMode_Source);
    iconPainer.drawPixmap(0, 0, pix);
    iconPainer.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    iconPainer.fillRect(temp.rect(), QColor(0, 0, 0, 64));
    iconPainer.end();

    return temp;
#endif
}

/**
 * @brief LoadIconsWorker::run
 */
void LoadIconsWorker::run()
{
    QString defaultIconPathFmt(":/icon/default_folder_icons/%1.svg");

    for (int i = 0; i < DEFAULTICONS_COUNT; i++) {
        QString iconPath = defaultIconPathFmt.arg(i + 1);
        DSvgRenderer svg(iconPath);
        QPixmap bitmap(QPixmap::fromImage(svg.toImage(svg.defaultSize())));
        VNoteDataManager::m_defaultIcons[IconsType::DefaultIcon].push_back(bitmap);
        VNoteDataManager::m_defaultIcons[IconsType::DefaultGrayIcon].push_back(greyPix(bitmap));
    }

    VNoteDataManager::m_iconLock.unlock();
}
