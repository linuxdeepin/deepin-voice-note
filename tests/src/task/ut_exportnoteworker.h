#ifndef UT_EXPORTNOTEWORKER_H
#define UT_EXPORTNOTEWORKER_H

#include "gtest/gtest.h"
#include <QList>

struct VNoteItem;
struct VNoteBlock;

class ut_exportnoteworker_test :  public ::testing::Test
{
public:
    ut_exportnoteworker_test();
protected:
    virtual void SetUp() override;
    virtual void TearDown() override;
 public:
    QList<VNoteItem *>m_noteList;
    VNoteBlock *m_block = nullptr;
};

#endif // UT_EXPORTNOTEWORKER_H
