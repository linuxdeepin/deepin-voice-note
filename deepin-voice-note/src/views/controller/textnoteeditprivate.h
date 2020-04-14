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
#ifndef TEXTNOTEEDITPRIVATE_H
#define TEXTNOTEEDITPRIVATE_H

#include <QObject>

#include "../textnoteedit.h"

/**
 * @brief The TextNoteEditPrivate class
 * 该类是类TextNoteEdit的私有数据类，专门处理数据与UI分离
 */
class TextNoteEditPrivate : public QObject
{
    Q_OBJECT
public:
    TextNoteEditPrivate(TextNoteEdit *parent = nullptr);

public:
    void print();

protected:
    TextNoteEdit *q_ptr;
    Q_DECLARE_PUBLIC(TextNoteEdit)
};

#endif // TEXTNOTEEDITPRIVATE_H
