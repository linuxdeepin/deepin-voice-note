/*
 * Copyright (C) 2019 ~ 2020 tx Technology Co., Ltd.
 *
 * Author:     duanxiaohui
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
#ifndef RIGHTVIEWPRIVATE_H
#define RIGHTVIEWPRIVATE_H

#include <QObject>

#include "../rightview.h"

/**
 * @brief The RightViewPrivate class
 * 右侧详情页视图数据私有类
 */
class RightViewPrivate : public QObject
{
    Q_OBJECT
public:
    RightViewPrivate(RightView *parent = nullptr);

protected:
    RightView *q_ptr;
    Q_DECLARE_PUBLIC(RightView)
};

#endif // RIGHTVIEWPRIVATE_H
