/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
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
#ifndef MoveView_H
#define MoveView_H

#include <DWidget>
#include <DApplicationHelper>
DWIDGET_USE_NAMESPACE

struct VNoteFolder;
struct VNoteItem;
class MoveView : public DWidget
{
    Q_OBJECT
public:
    explicit MoveView( QWidget *parent = nullptr);
    void setFolder(VNoteFolder* folder);
    void setNote(VNoteItem* note);
protected:
    void paintEvent(QPaintEvent *) override;
private:
    VNoteFolder *m_folder {nullptr};
    VNoteItem   *m_note {nullptr};

};

#endif // MoveView_H
