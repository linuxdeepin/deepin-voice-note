/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
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

#ifndef VNOTEBASEDIALOG_H
#define VNOTEBASEDIALOG_H

#include <DAbstractDialog>
#include <DLabel>
#include <DWindowCloseButton>

DWIDGET_USE_NAMESPACE

class QVBoxLayout;

class VNoteBaseDialog : public DAbstractDialog
{
    Q_OBJECT
public:
    explicit VNoteBaseDialog(QWidget *parent = nullptr);
    //添加文本窗口
    void addContent(QWidget *content);
    //设置logo图标
    void setIconPixmap(const QPixmap &iconPixmap);

    const int DEFAULT_WINDOW_W = 380;
    const int DEFAULT_WINDOW_H = 140;
    const int TITLEBAR_H = 50;

protected:
    //初始化界面
    void initUI();
    //连接槽函数
    void InitConnections();
    //设置logo是否可见
    void setLogoVisable(bool visible = true);
    //设置标题
    void setTitle(const QString &title);
    //获取窗口布局
    QLayout *getContentLayout();

    //重写窗口事件
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
signals:
    //窗口关闭信号
    void closed();
public slots:
private:
    QWidget *m_titleBar {nullptr};
    DLabel *m_logoIcon {nullptr};
    DLabel *m_tileText {nullptr};
    DWindowCloseButton *m_closeButton {nullptr};

    QWidget *m_content {nullptr};
    QVBoxLayout *m_contentLayout {nullptr};
};

#endif // VNOTEBASEDIALOG_H
