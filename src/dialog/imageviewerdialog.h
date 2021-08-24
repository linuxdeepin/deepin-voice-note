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

#ifndef DEEPIN_MANUAL_VIEW_WIDGETS_IMAGE_VIEWER_H
#define DEEPIN_MANUAL_VIEW_WIDGETS_IMAGE_VIEWER_H

#include <DLabel>
#include <DDialogCloseButton>

#include <QDialog>

DWIDGET_USE_NAMESPACE

/**
 * @brief ImageViewer::ImageViewer
 * @param parent
 * 点击图片后展示全屏的图片
 */
class ImageViewerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ImageViewerDialog(QWidget *parent = nullptr);
    ~ImageViewerDialog() override;

public slots:
    /**
     * @brief 加载图片并显示
     * @param filepath 图片路径
     */
    void open(const QString &filepath);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    /**
     * @brief 初始化ui
     */
    void initUI();

    using QDialog::open;

private:
    DLabel *m_imgLabel = nullptr; //图片显示控件
    DDialogCloseButton *m_closeButton; //关闭按钮控件
};

#endif // DEEPIN_MANUAL_VIEW_WIDGETS_IMAGE_VIEWER_H
