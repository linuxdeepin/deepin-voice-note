#include <gtest/gtest.h>
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

#include "common/vnotedatamanager.h"
#include "common/vnoteforlder.h"
#include "common/vnoteitem.h"
#include "db/vnoteitemoper.h"
#include "db/vnotefolderoper.h"

class GlobalEnvent : public testing::Environment
{
protected:
    virtual void SetUp() override;
    virtual void TearDown() override;
};

void GlobalEnvent::SetUp()
{
    VNoteDataManager *dataManager = VNoteDataManager::instance();
    dataManager->m_qspNoteFoldersMap.reset(new VNOTE_FOLDERS_MAP());
    dataManager->m_qspAllNotesMap.reset(new VNOTE_ALL_NOTES_MAP());
    QString defaultIconPathFmt(":/icons/deepin/builtin/default_folder_icons/%1.svg");

    for (int i = 0; i < 10; i++) {
        QString iconPath = defaultIconPathFmt.arg(i + 1);
        QPixmap bitmap(iconPath);
        VNoteDataManager::m_defaultIcons[IconsType::DefaultIcon].push_back(bitmap);
        VNoteDataManager::m_defaultIcons[IconsType::DefaultGrayIcon].push_back(bitmap);
    }
}

void GlobalEnvent::TearDown()
{
    ;
}

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new GlobalEnvent);
    return RUN_ALL_TESTS();
}
