#include "ut_exportnoteworker.h"
#include "task/exportnoteworker.h"
#include "common/vnotedatamanager.h"
#include "common/vnoteitem.h"

#include <QApplication>
#include <QList>

void ut_exportnoteworker_test::SetUp()
{
    VNOTE_ALL_NOTES_MAP *notes = VNoteDataManager::instance()->getAllNotesInFolder();
    if (notes && !notes->notes.isEmpty()) {
        VNOTE_ITEMS_MAP *tmp = notes->notes.first();
        if (tmp && !tmp->folderNotes.isEmpty()) {
            m_noteList.push_back(tmp->folderNotes.first());
            for (auto it : tmp->folderNotes.first()->datas.dataConstRef()) {
                if (it->getType() == VNoteBlock::Voice) {
                    m_block = it;
                    break;
                }
            }
        }
    }
}

void ut_exportnoteworker_test::TearDown()
{
}

ut_exportnoteworker_test::ut_exportnoteworker_test()
{
}

TEST_F(ut_exportnoteworker_test, exportText)
{
    ExportNoteWorker work(QCoreApplication::applicationDirPath(), ExportNoteWorker::ExportText, m_noteList, m_block);
    work.run();
}

TEST_F(ut_exportnoteworker_test, exportAllVoice)
{
    ExportNoteWorker work(QCoreApplication::applicationDirPath(), ExportNoteWorker::ExportAllVoice, m_noteList, m_block);
    work.run();
}

TEST_F(ut_exportnoteworker_test, exportAll)
{
    ExportNoteWorker work(QCoreApplication::applicationDirPath(), ExportNoteWorker::ExportAll, m_noteList, m_block);
    work.run();
}
TEST_F(ut_exportnoteworker_test, exportOneVoice)
{
    ExportNoteWorker work(QCoreApplication::applicationDirPath(), ExportNoteWorker::ExportOneVoice, m_noteList, m_block);
    work.run();
}

TEST_F(ut_exportnoteworker_test, checkPath)
{
    ExportNoteWorker work("test", ExportNoteWorker::ExportOneVoice, m_noteList, m_block);
    work.run();
}
