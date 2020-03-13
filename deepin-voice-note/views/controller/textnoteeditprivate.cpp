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
#include "textnoteeditprivate.h"
#include <QDebug>

TextNoteEditPrivate::TextNoteEditPrivate(TextNoteEdit *parent):
    q_ptr(parent), QObject(parent)
{

}

void TextNoteEditPrivate::print()
{
//    Q_Q(TextNoteEdit);
    qInfo() << "66666666666666666666";
}
