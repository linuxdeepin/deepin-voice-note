#ifndef VNOTESAFEROPER_H
#define VNOTESAFEROPER_H

#include "common/datatypedef.h"

class VNoteSaferOper
{
public:
    VNoteSaferOper();
    VNoteSaferOper(const VDataSafer& safer);

    SafetyDatas* loadSafers();

    void addSafer(const VDataSafer& safer);
    void rmSafer(const VDataSafer& safer);
protected:
    VDataSafer m_dataSafer;
};

#endif // VNOTESAFEROPER_H
