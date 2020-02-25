#include "middleview.h"
#include "notelistview.h"
#include "widgets/vnoteiconbutton.h"

#include "common/utils.h"
#include "common/vnotedatamanager.h"
#include "common/vnoteitem.h"
#include "common/vnoteforlder.h"
#include "db/vnoteitemoper.h"

#include <QMouseEvent>
#include <QVBoxLayout>

#include <DLog>
#include <DMenu>
#include <DAnchors>

MiddleView::MiddleView(QWidget *parent)
    : DWidget(parent)
{
    initUi();
    initConnection();
}

void MiddleView::initConnection()
{
    connect(m_addNoteBtn, &VNoteIconButton::clicked, this, &MiddleView::onAddNote);
    connect(m_noteListView, SIGNAL(currentChanged(const QModelIndex &)),
            this,SLOT(onNoteChange(const QModelIndex &)));
}

void MiddleView::mousePressEvent(QMouseEvent *event)
{
    DWidget::mouseMoveEvent(event);
}

void MiddleView::initUi()
{
    m_noteListView = new NoteListView(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;;
    m_noSearchResultLab = new DLabel(this);
    m_noSearchResultLab->setText(DApplication::translate("MiddleView","No search results"));
    m_noSearchResultLab->setAlignment(Qt::AlignCenter);
    DFontSizeManager::instance()->bind(m_noSearchResultLab, DFontSizeManager::T4);
    m_noSearchResultLab->setForegroundRole(DPalette::TextTips);
    m_noSearchResultLab->setVisible(false);
    m_centerWidget = new DStackedWidget(this);
    m_centerWidget->addWidget(m_noteListView);
    m_centerWidget->addWidget(m_noSearchResultLab);
    m_centerWidget->setContentsMargins(0,0,0,0);
    m_centerWidget->setCurrentWidget(m_noteListView);
    mainLayout->setContentsMargins(0, 5, 0, 0);
    mainLayout->addWidget(m_centerWidget);

    m_addNoteBtn = new VNoteIconButton(
        this,
        "add_note_normal.svg",
        "add_note_hover.svg",
        "add_note_press.svg"
    );
    m_addNoteBtn->SetDisableIcon("add_note_disabled.svg");
    m_addNoteBtn->setFlat(true);
    m_addNoteBtn->setIconSize(QSize(68, 68));
    m_addNoteBtn->raise();

    DAnchorsBase buttonAnchor(m_addNoteBtn);
    buttonAnchor.setAnchor(Qt::AnchorLeft, this, Qt::AnchorLeft);
    buttonAnchor.setAnchor(Qt::AnchorBottom, this, Qt::AnchorBottom);
    buttonAnchor.setBottomMargin(5);
    buttonAnchor.setLeftMargin(97);
    this->setLayout(mainLayout);
}

void MiddleView::onRenameItem()
{
    m_noteListView->edit(m_noteListView->currentIndex());
}

void MiddleView::onDeleteItem()
{
    m_noteListView->removeCurItem();
}

void MiddleView::onAddNote()
{
    if (m_notepad) {
        VNoteItem tmpNote;
        tmpNote.folderId = m_notepad->id;
        tmpNote.noteType = VNoteItem::VNT_Text;
        tmpNote.noteText = QString("");
        tmpNote.noteTitle = "新笔记１";
        VNoteItemOper noteOper;
        VNoteItem *newNote = noteOper.addNote(tmpNote);
        if(newNote){
           const QStandardItem *item = m_noteListView->addNoteItem(newNote);
           if(item){
               m_noteListView->setCurrentIndex(item->index());
           }
        }
    }
}

void MiddleView::initNoteData(VNoteFolder *notepad, const QRegExp &searchKey)
{
    m_notepad = notepad;
    VNoteItemOper noteOper;
    if (searchKey.isEmpty()) {
        VNOTE_ITEMS_MAP *datafolderNotes = noteOper.getFolderNotes(m_notepad->id);
        m_noteListView->initNoteData(datafolderNotes, searchKey);
        m_addNoteBtn->setVisible(true);
        m_centerWidget->setCurrentWidget(m_noteListView);
    } else {
        VNOTE_ALL_NOTES_MAP *noteAll = VNoteDataManager::instance()->getAllNotesInFolder();
        if (noteAll) {
            VNOTE_ITEMS_MAP searchNoteData;
            noteAll->lock.lockForRead();
            for (auto &it1 : noteAll->notes) {
                for (auto &it2 : it1->folderNotes) {
                    if (it2->noteTitle.contains(searchKey)
                            || it2->noteText.contains(searchKey)) {
                        searchNoteData.folderNotes.insert(it2->noteId, it2);
                    }
                }
            }
            noteAll->lock.unlock();
            m_noteListView->initNoteData(&searchNoteData, searchKey);
            m_addNoteBtn->setVisible(false);
            if(searchNoteData.folderNotes.size()){
                m_centerWidget->setCurrentWidget(m_noteListView);
            }else {
                m_centerWidget->setCurrentWidget(m_noSearchResultLab);
            }
        }
    }
}

void MiddleView::onNoteChange(const QModelIndex &previous)
{
    Q_UNUSED(previous);
    emit sigNotepadItemChange(m_noteListView->currentIndex());
}

 VNoteItem* MiddleView::getNoteData(const QModelIndex &index) const
 {
     return  m_noteListView->getItemData(index);
 }

 void MiddleView::clearNoteData()
 {
     return m_noteListView->clearAllItems();
 }
