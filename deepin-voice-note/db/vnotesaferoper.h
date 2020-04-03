#ifndef VNOTESAFEROPER_H
#define VNOTESAFEROPER_H

#include "common/datatypedef.h"

class DbVisitor;

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

    static const QStringList saferColumnsName;

    enum {
        id = 0,
        folder_id,
        note_id,
        path,
        state,
        meta_data,
        create_time,
    };

    friend class SaferQryDbVisitor;
};

#endif // VNOTESAFEROPER_H
