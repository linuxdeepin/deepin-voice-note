#include "rightview.h"
#include "common/vnoteitem.h"
#include "rightnotelist.h"
#include "common/vnotedatamanager.h"
#include "db/vnoteitemoper.h"

#include <QBoxLayout>
#include <QDebug>
#include <QStandardPaths>
#include <QTimer>
#include <QScrollBar>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DFileDialog>
#include <DMessageBox>

RightView::RightView(QWidget *parent)
    : QWidget(parent)
{
    initUi();
    initConnection();
}

void RightView::initUi()
{
    m_stackWidget = new DStackedWidget(this);
    m_stackWidget->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *layoutList = new QVBoxLayout;
    layoutList->setContentsMargins(0, 0, 0, 0);
    layoutList->addWidget(m_stackWidget);
    layoutList->addSpacing(90);
    this->setLayout(layoutList);

    m_noSearchResult = new DLabel(this);
    m_noSearchResult->setText(tr("No search results"));
    m_noSearchResult->setAlignment(Qt::AlignCenter);
    DFontSizeManager::instance()->bind(m_noSearchResult, DFontSizeManager::T4);
    m_stackWidget->insertWidget(0, m_noSearchResult);

    m_searchNoteList = new RightNoteList(this);
    m_searchNoteList->verticalScrollBar()->setSingleStep(10);
    m_searchNoteList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_searchNoteList->setLineWidth(0);
    m_stackWidget->insertWidget(1, m_searchNoteList);
    m_stackWidget->setCurrentWidget(m_searchNoteList);

    m_addTextBtn = new DPushButton(this);
    DFontSizeManager::instance()->bind(m_addTextBtn, DFontSizeManager::T5);
    m_addTextBtn->setText(tr("Click to Add TextNote"));
    m_addTextBtn->setFixedHeight(64);

    initTextMenu();
    initVoiceMenu();

    m_player = new QMediaPlayer(this);
    m_delDialog = new VNoteMessageDialog(VNoteMessageDialog::DeleteNote, this);
    m_asrlimitErrDialog = new VNoteMessageDialog(VNoteMessageDialog::AsrTimeLimit, this);
    m_asrlimitErrDialog->setWindowFlags(m_asrlimitErrDialog->windowFlags() | Qt::WindowStaysOnTopHint);
}

void RightView::initTextMenu()
{
    m_textMenu = new DMenu(this);
    m_saveTextAction = new QAction(tr("Save As TXT"), this);
    m_delTextAction = new QAction(tr("Delete"), this);
    m_textMenu->addAction(m_saveTextAction);
    m_textMenu->addAction(m_delTextAction);
}

void RightView::initVoiceMenu()
{
    m_voiceMenu = new DMenu(this);
    m_asrVoiceAction = new QAction(tr("Voice to Text"), this);
    m_saveVoicetAction = new QAction(tr("Save As MP3"), this);
    m_delVoiceAction = new QAction(tr("Delete"), this);
    m_voiceMenu->addAction(m_asrVoiceAction);
    m_voiceMenu->addAction(m_saveVoicetAction);
    m_voiceMenu->addAction(m_delVoiceAction);
}
void RightView::initConnection()
{
    connect(m_addTextBtn, &DPushButton::clicked, this, &RightView::onAddNote);
    connect(m_searchNoteList, SIGNAL(sigDelNote(VNoteItem *)), this, SLOT(onDelNote(VNoteItem *)));
    connect(m_searchNoteList, SIGNAL(sigMenuPopup(VNoteItem *)), this, SLOT(onMenuPopup(VNoteItem *)));
    connect(m_searchNoteList, SIGNAL(sigUpdateNote(VNoteItem *)), this, SLOT(onUpdateNote(VNoteItem *)));
    connect(m_searchNoteList, SIGNAL(sigTextEditDetail(VNoteItem *, DTextEdit *, const QRegExp &)), this,
            SIGNAL(sigTextEditDetail(VNoteItem *, DTextEdit *, const QRegExp &)));
    connect(m_searchNoteList, SIGNAL(sigTextEditIsEmpty(VNoteItem *, bool)), this,
            SLOT(onTextEditIsEmpty(VNoteItem *, bool)));

    connect(m_searchNoteList, SIGNAL(sigVoicePlayBtnClicked(VoiceNoteItem *)), this,
            SLOT(onVoicePlayBtnClicked(VoiceNoteItem *)));
    connect(m_searchNoteList, SIGNAL(sigVoicePauseBtnClicked(VoiceNoteItem *)), this,
            SLOT(onVoicePauseBtnClicked(VoiceNoteItem *)));

    connect(m_searchNoteList, &RightNoteList::sigListHeightChange, this, &RightView::adjustaddTextBtn);
    connect(m_searchNoteList, &RightNoteList::sigVoicePlayPosChange, this, &RightView::onSetPlayerPos);

    connect(m_delTextAction, &QAction::triggered, this, &RightView::onDelTextAction);
    connect(m_saveTextAction, &QAction::triggered, this, &RightView::onSaveTextAction);
    connect(m_delVoiceAction, &QAction::triggered, this, &RightView::onDelVoiceAction);
    connect(m_asrVoiceAction, &QAction::triggered, this, &RightView::onAsrVoiceAction);
    connect(m_saveVoicetAction, &QAction::triggered, this, &RightView::onSaveVoiceAction);
    connect(m_player, &QMediaPlayer::positionChanged,
            this, &RightView::onVoicePlayPosChange);
}

void RightView::addNewNoteList(qint64 id)
{
    RightNoteList *listWidget = new RightNoteList(m_stackWidget);
    listWidget->verticalScrollBar()->setSingleStep(10);
    connect(listWidget, SIGNAL(sigDelNote(VNoteItem *)), this, SLOT(onDelNote(VNoteItem *)));
    connect(listWidget, SIGNAL(sigMenuPopup(VNoteItem *)), this, SLOT(onMenuPopup(VNoteItem *)));
    connect(listWidget, SIGNAL(sigUpdateNote(VNoteItem *)), this, SLOT(onUpdateNote(VNoteItem *)));
    connect(listWidget, SIGNAL(sigTextEditDetail(VNoteItem *, DTextEdit *, const QRegExp &)), this,
            SIGNAL(sigTextEditDetail(VNoteItem *, DTextEdit *, const QRegExp &)));
    connect(listWidget, SIGNAL(sigTextEditIsEmpty(VNoteItem *, bool)), this, SLOT(onTextEditIsEmpty(VNoteItem *, bool)));

    connect(listWidget, SIGNAL(sigVoicePlayBtnClicked(VoiceNoteItem *)), this,
            SLOT(onVoicePlayBtnClicked(VoiceNoteItem *)));
    connect(listWidget, SIGNAL(sigVoicePauseBtnClicked(VoiceNoteItem *)), this,
            SLOT(onVoicePauseBtnClicked(VoiceNoteItem *)));

    connect(listWidget, &RightNoteList::sigListHeightChange, this, &RightView::adjustaddTextBtn);
    connect(listWidget, &RightNoteList::sigVoicePlayPosChange, this, &RightView::onSetPlayerPos);

    VNoteItemOper noteOper;
    VNOTE_ITEMS_MAP *folderNotes = noteOper.getFolderNotes(id);
    listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listWidget->setLineWidth(0);
    listWidget->initNoteItem(id, folderNotes, m_searchKey);
    m_stackWidget->addWidget(listWidget);
    m_stackWidget->setCurrentWidget(listWidget);

}

void RightView::noteDelByFolder(qint64 id)
{
    RightNoteList *widget = getNormalNoteList(id);
    if (widget != nullptr) {
        m_stackWidget->removeWidget(widget);
        widget->deleteLater();
        adjustaddTextBtn();
    }
}

void RightView::noteSwitchByFolder(qint64 id)
{
    if (m_searchKey.isEmpty()) {
        bool find = false;

        for (int i = 2; i < m_stackWidget->count(); i++) {
            RightNoteList *widget = static_cast<RightNoteList *>(m_stackWidget->widget(i));
            if (widget != nullptr && widget->getFolderId() == id) {
                m_stackWidget->setCurrentWidget(widget);
                find = true;
            }
        }
        if (find == false) {
            addNewNoteList(id);//没加载创建新的
        }
        m_stackWidget->currentWidget()->setFocus();
    } else { //搜索逻辑
        VNoteItemOper noteOper;
        VNOTE_ITEMS_MAP *datafolderNotes = noteOper.getFolderNotes(id);
        VNOTE_ITEMS_MAP searchNoteData;
        if (datafolderNotes != nullptr) {
            datafolderNotes->lock.lockForRead();
            for (auto it : datafolderNotes->folderNotes) {
                if (it->noteText.contains(m_searchKey)) {
                    searchNoteData.folderNotes.insert(it->noteId, it);
                }
            }
            datafolderNotes->lock.unlock();
        }
        if (searchNoteData.folderNotes.size()) {
            m_stackWidget->setCurrentWidget(m_searchNoteList);
            m_searchNoteList->initNoteItem(id, &searchNoteData, m_searchKey);
        } else {
            m_stackWidget->setCurrentWidget(m_noSearchResult);
        }
    }
    stopCurVoicePlaying(0);
    m_lastFolderId = id;
    adjustaddTextBtn();
}

void RightView::adjustaddTextBtn()
{
    int pos = 10;
    if (m_stackWidget->currentWidget() != m_noSearchResult) {
        RightNoteList *widget = static_cast<RightNoteList *>(m_stackWidget->currentWidget());

        if (widget != nullptr) {
            pos += widget->getListHeight();
            int maxPos = this->height() - m_addTextBtn->height();
            if (pos > maxPos) {
                pos = maxPos;
            }
        }
    }
    m_addTextBtn->setFixedWidth(this->width() - 18);
    m_addTextBtn->move(9, pos);
}

void RightView::onDelNote(VNoteItem *item)
{
    RightNoteList *widget = static_cast<RightNoteList *>(sender());
    delNoteFromList(item, widget);
    m_addTextBtn->setEnabled(true);
}

void RightView::onAddNote() //添加文字记录
{
    VNoteItem tmpNote;
    tmpNote.folderId = m_lastFolderId;
    tmpNote.noteType = VNoteItem::VNT_Text;
    tmpNote.noteText = QString("");
    addNewNote(tmpNote, true);
    m_addTextBtn->setEnabled(false);
}

void RightView::onUpdateNote(VNoteItem *item)
{
    qInfo() << "onUpdateNote:" << item
            << "NoteId:" << item->noteId
            << "NoteText:" << item->noteText;
    VNoteItemOper noteOper(item);
    noteOper.modifyNoteText(item->noteText);
    RightNoteList *widget = static_cast<RightNoteList *>(m_stackWidget->currentWidget());
    if (widget == m_searchNoteList) {
        RightNoteList *widgetTmp = getNormalNoteList(widget->getFolderId());
        if (widgetTmp != nullptr) {
            widgetTmp->updateNodeItem(item);
        }
    }
}

void RightView::resizeEvent(QResizeEvent *event)
{
    adjustaddTextBtn();
    DWidget::resizeEvent(event);
}

QList<qint64> RightView::getNoteContainsKeyFolders(const QRegExp &searchKey) //内容包含查找内容
{
    QList<qint64> folderIds;
    VNOTE_ALL_NOTES_MAP *noteAll = VNoteDataManager::instance()->getAllNotesInFolder();
    if (noteAll != nullptr) {
        noteAll->lock.lockForRead();
        for (auto &it1 : noteAll->notes) {
            for (auto &it2 : it1->folderNotes) {
                if (it2->noteType == VNoteItem::VNOTE_TYPE::VNT_Text
                        && it2->noteText.contains(searchKey)) {
                    folderIds.push_back(it2->folderId);
                    break;
                }
            }
        }
        noteAll->lock.unlock();
    }
    return folderIds;
}

void RightView::onTextEditIsEmpty(VNoteItem *textNode, bool empty)
{
    Q_UNUSED(textNode);
    m_addTextBtn->setEnabled(!empty);
}

void RightView::setSearchKey(const QRegExp &searchKey)
{
    m_searchKey = searchKey;
}

void RightView::onMenuPopup(VNoteItem *item)
{
    if (m_curMenuNoteItem != item) {
        m_curMenuNoteItem = item;
        emit sigMenuNoteItemChange();
    }
    if (m_curMenuNoteItem != nullptr) {
        QPoint pos = QCursor::pos();
        pos.setX(pos.x() - 44);
        pos.setY(pos.y() + 20);
        if (m_curMenuNoteItem->noteType == VNoteItem::VNOTE_TYPE::VNT_Text) {
            m_textMenu->exec(pos);
        } else {
            if (m_asrVoiceItem == nullptr) {
                bool textEmpty = m_curMenuNoteItem->noteText.isEmpty();
                m_asrVoiceAction->setEnabled(textEmpty); //已经转语音的文字不能再次转语音
            }
            m_voiceMenu->exec(pos);
        }
    }
}

void RightView::onSaveTextAction()
{
    DFileDialog fileDialog(this);
    fileDialog.setWindowTitle(tr("Save as TXT"));
    fileDialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    fileDialog.setFileMode(DFileDialog::Directory);
    fileDialog.setAcceptMode(DFileDialog::AcceptSave);
    fileDialog.setDefaultSuffix("txt");
    fileDialog.setNameFilter("TXT(*.txt)");
    fileDialog.selectFile(tr("TextNote") + QString::number(m_curMenuNoteItem->noteId));
    if (fileDialog.exec() == QDialog::Accepted) {
        QString path = fileDialog.selectedFiles()[0];
        QString fileName = QFileInfo(path).fileName();
        if (fileName.isEmpty()) {
            DMessageBox::information(this, tr(""), tr("File name cannot be empty"));
        } else {
            QFile file(path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream textStream(&file);
                textStream << m_curMenuNoteItem->noteText;
                file.close();
            }
        }
    }
}

void RightView::onDelTextAction()
{
    if (m_delDialog->exec() == QDialog::Accepted) {
        RightNoteList *list = static_cast<RightNoteList *>(m_stackWidget->currentWidget());
        delNoteFromList(m_curMenuNoteItem, list);
    }
}

void RightView::onAsrVoiceAction()
{
    if (m_curMenuNoteItem->voiceSize > 20 * 60 * 1000) {
        m_asrlimitErrDialog->exec();
        return;
    }
    RightNoteList *list = static_cast<RightNoteList *>(m_stackWidget->currentWidget());
    m_asrVoiceItem = list->getVoiceItem(m_curMenuNoteItem);
    if (m_asrVoiceItem != nullptr) {
        m_asrVoiceItem->enbleMenuBtn(false);
        m_asrVoiceAction->setEnabled(false);
        m_asrVoiceItem->showAsrStartWindow();
        QString file = m_curMenuNoteItem->voicePath;
        qint64 durtation = m_curMenuNoteItem->voiceSize;
        qDebug() << "onAsrVoiceAction: " << file << ", " << durtation;
        Q_EMIT asrStart(file, durtation);
    }
}

void RightView::onDelVoiceAction()
{
    if (m_delDialog->exec() == QDialog::Accepted) {
        RightNoteList *list = static_cast<RightNoteList *>(m_stackWidget->currentWidget());
        VoiceNoteItem *item = list->getVoiceItem(m_curMenuNoteItem);
        if (item == m_playVoiceItem) {
            stopCurVoicePlaying();
        }
        delNoteFromList(m_curMenuNoteItem, list);
    }
}

void RightView::onSaveVoiceAction()
{
    DFileDialog fileDialog(this);
    fileDialog.setWindowTitle(tr("Save as MP3"));
    fileDialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    fileDialog.setFileMode(DFileDialog::Directory);
    fileDialog.setAcceptMode(DFileDialog::AcceptSave);
    fileDialog.setDefaultSuffix("mp3");
    fileDialog.setNameFilter("MP3(*.mp3)");
    fileDialog.selectFile(tr("Voice note") + QString::number(m_curMenuNoteItem->noteId));
    if (fileDialog.exec() == QDialog::Accepted) {
        QString path = fileDialog.selectedFiles()[0];
        if (path.isEmpty()) {
            DMessageBox::information(this, tr(""), tr("File name cannot be empty"));
        } else {
            bool ret = QFile::copy(m_curMenuNoteItem->voicePath, path);
            qDebug() << ret;
        }
    }
}

void RightView::delNoteFromList(VNoteItem *item, RightNoteList *list)
{
    list->delNodeItem(item);
    if (list == m_searchNoteList) {
        RightNoteList *widgetTmp = getNormalNoteList(list->getFolderId());
        if (widgetTmp != nullptr) {
            widgetTmp->delNodeItem(item);
        }
        if (m_searchNoteList->count() == 0) { //搜索时全部删除通知记事本项更新
            emit sigSearchNoteEmpty(m_searchNoteList->getFolderId());
        }
    }

    qInfo() << "Delete VNoteItem:" << item
            << "NoteID:" << item->noteId
            << "NoteText:" << item->noteText;

    VNoteItemOper noteOper;
    noteOper.deleteNote(item->folderId, item->noteId);

    adjustaddTextBtn();
}

void RightView::onVoicePlayBtnClicked(VoiceNoteItem *item)
{
    if (m_playVoiceItem != item) { //切换播放文件
        stopCurVoicePlaying(0);
        m_player->setMedia(QUrl::fromLocalFile(item->getNoteItem()->voicePath));
    }
    m_playVoiceItem = item;
    item->showPauseBtn();
    m_playVoiceItem->setSliderEnable(true);
    m_player->play();
}

void RightView::onVoicePauseBtnClicked(VoiceNoteItem *item)
{
    if (m_playVoiceItem == item) {
        m_playVoiceItem->showPlayBtn();
        m_player->pause();
    }
}

void RightView::addVoiceNoteItem(const QString &voicePath, qint64 voiceSize)
{
    VNoteItem tmpNote;
    tmpNote.folderId = m_lastFolderId;
    tmpNote.noteType = VNoteItem::VNT_Voice;
    tmpNote.noteText = QString("");
    tmpNote.voicePath = voicePath;
    tmpNote.voiceSize = voiceSize;
    addNewNote(tmpNote);
}

RightNoteList *RightView::getNormalNoteList(qint64 id)
{
    for (int i = 2; i < m_stackWidget->count(); i++) {
        RightNoteList *tempList = static_cast<RightNoteList *>(m_stackWidget->widget(i));
        if (tempList->getFolderId() == id) {
            return  tempList;
        }
    }
    return nullptr;
}

void RightView::onVoicePlayPosChange(qint64 pos)
{
    if (pos && m_playVoiceItem != nullptr) {
        m_playVoiceItem->setPlayPos(static_cast<int>(pos));
        qint64 duration = m_playVoiceItem->getNoteItem()->voiceSize;
        qDebug() << pos << ";" << duration;
        if (pos >= duration) {
            stopCurVoicePlaying();
        }
    }
}

void RightView::onSetPlayerPos(int pos)
{
    qDebug() << "seek pos:" << pos;
    m_player->setPosition(pos);
}

void RightView::setVoicePlayEnable(bool enable)
{
    int index = m_stackWidget->currentIndex();
    if (index > 1) {
        stopCurVoicePlaying(0);
        RightNoteList *list = static_cast<RightNoteList *>(m_stackWidget->currentWidget());
        list->setVoicePlayEnable(enable);
    }
}

void RightView::stopCurVoicePlaying(int pos)
{
    if (m_player->state() != QMediaPlayer::State::StoppedState) {
        m_player->stop();
        if (m_playVoiceItem != nullptr) {
            m_playVoiceItem->showPlayBtn();
            if (pos >= 0) {
                m_playVoiceItem->setPlayPos(pos);
            }
            m_playVoiceItem->setSliderEnable(false);
            m_playVoiceItem = nullptr;
        }
    }
}

void RightView::addNewNote(VNoteItem &data, bool isByBtn)
{
    if (data.folderId != VNoteFolder::INVALID_ID) {
        VNoteItemOper noteOper;
        VNoteItem *newNote = noteOper.addNote(data);
        if (newNote != nullptr) {
            int index = m_stackWidget->currentIndex();
            RightNoteList *list = nullptr;
            if (index == 0) {
                list = m_searchNoteList;
                m_searchNoteList->initNoteItem(m_lastFolderId, nullptr, m_searchKey);
                m_stackWidget->setCurrentWidget(m_searchNoteList);
            } else {
                list = static_cast<RightNoteList *>(m_stackWidget->currentWidget());
            }
            list->addNodeItem(newNote, QRegExp(), isByBtn);
            if (list == m_searchNoteList) {
                list = getNormalNoteList(m_lastFolderId);
                if (list != nullptr) {
                    list->addNodeItem(newNote, QRegExp());
                    list->adjustSize();
                }
            }
            adjustaddTextBtn();
        }
    }
}

void RightView::setAsrResult(const QString &result)
{
    if (m_asrVoiceItem != nullptr) {
        m_asrVoiceItem->showAsrEndWindow(result);
        m_asrVoiceItem->enbleMenuBtn(true);
        if (!result.isEmpty()) {
            VNoteItem *data = m_asrVoiceItem->getNoteItem();
            data->noteText = result;
            onUpdateNote(data);
        }
        m_asrVoiceItem = nullptr;
    }
}
