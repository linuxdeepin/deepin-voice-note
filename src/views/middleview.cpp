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

#include "middleview.h"
#include "vnoteapplication.h"
#include "globaldef.h"
#include "middleviewdelegate.h"
#include "middleviewsortfilter.h"
#include "common/actionmanager.h"
#include "common/standarditemcommon.h"
#include "task/exportnoteworker.h"

#include <QMouseEvent>
#include <QVBoxLayout>

#include <DApplication>
#include <DFileDialog>
#include <DLog>

MiddleView::MiddleView(QWidget *parent)
    : DListView(parent)
{
    initModel();
    initDelegate();
    initMenu();
    initUI();
    initAppSetting();
}

void MiddleView::initModel()
{
    m_pDataModel = new QStandardItemModel(this);

    m_pSortViewFilter = new MiddleViewSortFilter(this);
    m_pSortViewFilter->setDynamicSortFilter(false);
    m_pSortViewFilter->setSourceModel(m_pDataModel);

    this->setModel(m_pSortViewFilter);
}

void MiddleView::initDelegate()
{
    m_pItemDelegate = new MiddleViewDelegate(this);
    this->setItemDelegate(m_pItemDelegate);
}

void MiddleView::initMenu()
{
    m_noteMenu = ActionManager::Instance()->noteContextMenu();
}

void MiddleView::setSearchKey(const QString &key)
{
    m_searchKey = key;
    m_pItemDelegate->setSearchKey(key);
}

void MiddleView::setCurrentId(qint64 id)
{
    m_currentId = id;
}

qint64 MiddleView::getCurrentId()
{
    return m_currentId;
}

void MiddleView::addRowAtHead(VNoteItem *note)
{
    if (nullptr != note) {
        QStandardItem *item = StandardItemCommon::createStandardItem(note, StandardItemCommon::NOTEITEM);
        m_pDataModel->insertRow(0, item);

        DListView::setCurrentIndex(m_pSortViewFilter->index(0, 0));
    }
}

void MiddleView::appendRow(VNoteItem *note)
{
    if (nullptr != note) {
        QStandardItem *item = StandardItemCommon::createStandardItem(note, StandardItemCommon::NOTEITEM);
        m_pDataModel->appendRow(item);
    }
}

void MiddleView::clearAll()
{
    m_pDataModel->clear();
}

VNoteItem *MiddleView::deleteCurrentRow()
{
    QModelIndex index = currentIndex();
    VNoteItem *noteData = reinterpret_cast< VNoteItem *>(
                              StandardItemCommon::getStandardItemData(index)
                          );

    m_pSortViewFilter->removeRow(index.row());

    return noteData;
}

VNoteItem *MiddleView::getCurrVNotedata() const
{
    QModelIndex index = currentIndex();
    VNoteItem *noteData = reinterpret_cast< VNoteItem *>(
                              StandardItemCommon::getStandardItemData(index)
                          );

    return noteData;
}

void MiddleView::onNoteChanged()
{
    m_pSortViewFilter->sortView();
    this->scrollToTop();
}

qint32 MiddleView::rowCount() const
{
    return m_pDataModel->rowCount();
}

void MiddleView::setCurrentIndex(int index)
{
    DListView::setCurrentIndex(m_pSortViewFilter->index(index, 0));
}

void MiddleView::editNote()
{
    edit(currentIndex());
}

void MiddleView::saveAsText()
{
    QModelIndex index = currentIndex();
    VNoteItem *noteData = reinterpret_cast< VNoteItem *>(
                              StandardItemCommon::getStandardItemData(index)
                          );
    if (nullptr != noteData) {
        //TODO:
        //    Should check if this note is doing save action

        DFileDialog dialog;
        dialog.setFileMode(DFileDialog::DirectoryOnly);
        dialog.setLabelText(DFileDialog::Accept, DApplication::translate("MiddleView", "Save"));
        dialog.setNameFilter("TXT(*.txt)");

        QString historyDir = m_qspSetting->value(VNOTE_EXPORT_TEXT_PATH_KEY).toString();
        if (historyDir.isEmpty()) {
            historyDir = QDir::homePath();
        }
        dialog.setDirectory(historyDir);

        if (QDialog::Accepted == dialog.exec()) {
            // save the directory string to config file.
            m_qspSetting->setValue(VNOTE_EXPORT_TEXT_PATH_KEY, dialog.directoryUrl().toLocalFile());

            QString exportDir = dialog.directoryUrl().toLocalFile();
            ExportNoteWorker *exportWorker = new ExportNoteWorker(
                exportDir, ExportNoteWorker::ExportText, noteData);
            exportWorker->setAutoDelete(true);

            QThreadPool::globalInstance()->start(exportWorker);
        }
    }
}

void MiddleView::saveRecords()
{
    QModelIndex index = currentIndex();
    VNoteItem *noteData = reinterpret_cast< VNoteItem *>(
                              StandardItemCommon::getStandardItemData(index)
                          );
    if (nullptr != noteData) {
        //TODO:
        //    Should check if this note is doing save action

        DFileDialog dialog;
        dialog.setFileMode(DFileDialog::DirectoryOnly);
        dialog.setLabelText(DFileDialog::Accept, DApplication::translate("MiddleView", "Save"));
        dialog.setNameFilter("MP3(*.mp3)");

        QString historyDir = m_qspSetting->value(VNOTE_EXPORT_VOICE_PATH_KEY).toString();
        if (historyDir.isEmpty()) {
            historyDir = QDir::homePath();
        }
        dialog.setDirectory(historyDir);

        if (QDialog::Accepted == dialog.exec()) {
            // save the directory string to config file.
            m_qspSetting->setValue(VNOTE_EXPORT_VOICE_PATH_KEY, dialog.directoryUrl().toLocalFile());

            QString exportDir = dialog.directoryUrl().toLocalFile();

            ExportNoteWorker *exportWorker = new ExportNoteWorker(
                exportDir, ExportNoteWorker::ExportAllVoice, noteData);
            exportWorker->setAutoDelete(true);

            QThreadPool::globalInstance()->start(exportWorker);
        }
    }
}

void MiddleView::mousePressEvent(QMouseEvent *event)
{
    this->setFocus();

    if (!m_onlyCurItemMenuEnable) {
        DListView::mouseMoveEvent(event);
    }

    if (event->button() == Qt::RightButton) {
        QModelIndex index = this->indexAt(event->pos());
        if (index.isValid()
                && (!m_onlyCurItemMenuEnable || index == this->currentIndex())) {
            DListView::setCurrentIndex(index);
            m_noteMenu->popup(event->globalPos());
        }
    }
}

void MiddleView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_onlyCurItemMenuEnable) {
        DListView::mouseReleaseEvent(event);
    }
}

void MiddleView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!m_onlyCurItemMenuEnable) {
        DListView::mouseDoubleClickEvent(event);
    }
}

void MiddleView::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_onlyCurItemMenuEnable) {
        DListView::mouseMoveEvent(event);
    }
}

bool MiddleView::eventFilter(QObject *o, QEvent *e)
{
    Q_UNUSED(o);
    if(e->type() == QEvent::FocusIn){
        m_pItemDelegate->setEditIsVisible(true);
        this->update(currentIndex());
    }else if (e->type() == QEvent::Destroy) {
        m_pItemDelegate->setEditIsVisible(false);
        this->update(currentIndex());
    }
    return  false;
}

void MiddleView::keyPressEvent(QKeyEvent *e)
{
    if (m_onlyCurItemMenuEnable || e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown) {
        e->ignore();
    }else {
        DListView::keyPressEvent(e);
    }
}

void MiddleView::initUI()
{
    //TODO:
    //    HQ & scaler > 1 have one line at
    //the footer of DListView,so add footer
    //to solve this bug
    QWidget *footer = new QWidget(this);
    footer->setFixedHeight(1);
    addFooterWidget(footer);

    m_emptySearch = new DLabel(this);
    m_emptySearch->setText(DApplication::translate("MiddleView", "No search results"));
    m_emptySearch->setAlignment(Qt::AlignCenter);
    DFontSizeManager::instance()->bind(m_emptySearch, DFontSizeManager::T4);
    m_emptySearch->setForegroundRole(DPalette::PlaceholderText);
    m_emptySearch->setVisible(false);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_emptySearch);
    this->setLayout(layout);
//    this->installEventFilter(this);
}

void MiddleView::initAppSetting()
{
    m_qspSetting = reinterpret_cast<VNoteApplication *>(qApp)->appSetting();
}

void MiddleView::setVisibleEmptySearch(bool visible)
{
    m_emptySearch->setVisible(visible);
}

void MiddleView::setOnlyCurItemMenuEnable(bool enable)
{
    m_onlyCurItemMenuEnable = enable;
    m_pItemDelegate->setEnableItem(!enable);
    this->update();
}

void MiddleView::closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
    Q_UNUSED(hint);
    DListView::closeEditor(editor,QAbstractItemDelegate::NoHint);
}
