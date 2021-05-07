#include "ut_vndatasafertask.h"
#include "task/vndatasafertask.h"

ut_vndatasafertask_test::ut_vndatasafertask_test()
{
}

TEST_F(ut_vndatasafertask_test, run)
{
    VDataSafer param;
    param.id = 10;
    param.folder_id = 0;
    param.note_id = 0;
    param.path = "/tmp/";
    param.createTime = QDateTime::currentDateTime();
    param.saferType = VDataSafer::Safe;
    VNDataSaferTask taskSafe(param);
    taskSafe.run();
    param.saferType = VDataSafer::Unsafe;
    VNDataSaferTask taskUnSafe(param);
    taskUnSafe.run();
}
