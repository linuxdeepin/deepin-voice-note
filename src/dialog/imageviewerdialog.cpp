/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     leilong <leilong@uniontech.com>
*
* Maintainer: leilong <leilong@uniontech.com>
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

#include "dialog/imageviewerdialog.h"

#include <DLog>
#include <QApplication>
#include <QDesktopWidget>
#include <QShortcut>
#include <QImageReader>

namespace {

const int kBorderSize = 12;
const int kCloseBtnSize = 48;

} // namespace

ImageViewerDialog::ImageViewerDialog(QWidget *parent)
    : QDialog(parent)
{
    this->setObjectName("ImageViewer");
    this->initUI();

    connect(m_closeButton, &Dtk::Widget::DIconButton::clicked, this, &ImageViewerDialog::close);
}

ImageViewerDialog::~ImageViewerDialog()
{
}

/**
 * @brief ImageViewer::open
 * @param filepath
 * @note 根据给定的图片文件路径，打开图片并全屏显示
 */
void ImageViewerDialog::open(const QString &filepath)
{
    QImage image;
    QImageReader reader(filepath);
    reader.setDecideFormatFromContent(true);
    if (!reader.canRead() || !reader.read(&image)) {
        return;
    }

    //获取图片显示的最大大小（显示屏大小的80%）
    const QRect screenRect = qApp->desktop()->screenGeometry(QCursor::pos());
    const int pixmapMaxWidth = static_cast<int>(screenRect.width() * 0.8);
    const int pixmapMaxHeight = static_cast<int>(screenRect.height() * 0.8);

    //设置图片保存纵横比平滑模式自适应控件大小
    image = image.scaled(pixmapMaxWidth, pixmapMaxHeight, Qt::KeepAspectRatio,
                         Qt::SmoothTransformation);

    //设置控件占据整个屏幕大小
    this->move(screenRect.topLeft());
    this->resize(screenRect.size());
    this->showFullScreen();

    //加载图片
    m_imgLabel->setPixmap(QPixmap::fromImage(image));
    m_imgLabel->setFixedSize(image.width(), image.height());

    //将图片移至居中位置
    QRect imgRect = m_imgLabel->rect();
    imgRect.moveTo(static_cast<int>((screenRect.width() - image.width()) / 2.0),
                   static_cast<int>((screenRect.height() - image.height()) / 2.0));
    m_imgLabel->move(imgRect.topLeft());

    //关闭按钮移到图片右上角
    const QPoint top_right_point = imgRect.topRight();
    m_closeButton->move(top_right_point.x() - kCloseBtnSize / 2,
                        top_right_point.y() - kCloseBtnSize / 2);
    m_closeButton->show();
    m_closeButton->raise();
}

/**
 * @brief ImageViewer::initUI
 * 界面初始化
 */
void ImageViewerDialog::initUI()
{
    m_imgLabel = new DLabel(this);
    m_imgLabel->setObjectName("ImageLabel");

    //右上角关闭按钮初始化
    m_closeButton = new DDialogCloseButton(this);
    m_closeButton->setFlat(true);
    m_closeButton->setObjectName("CloseButton");
    m_closeButton->raise();
    m_closeButton->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    m_closeButton->setIconSize(QSize(36, 36));
    m_closeButton->setFixedSize(45, 45);

    this->setContentsMargins(kBorderSize, kBorderSize, kBorderSize, kBorderSize);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::BypassWindowManagerHint
                         | Qt::Dialog | Qt::WindowStaysOnTopHint);
    this->setAttribute(Qt::WA_TranslucentBackground, true);
    this->setModal(true);
}

/**
 * @brief ImageViewer::mousePressEvent
 * @param event
 * 重写鼠标点击事件，打开图片后点击窗口任意位置，都隐藏此界面
 */
void ImageViewerDialog::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    this->hide();
}

/**
 * @brief ImageViewer::paintEvent
 * @param event
 * 画图事件，重绘整个屏幕大小
 */
void ImageViewerDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(0, 0, this->width(), this->height(), QColor(0, 0, 0, 77));
}
