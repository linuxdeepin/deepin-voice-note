// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SEARCHDOCUMENTEXTRACTOR_H
#define SEARCHDOCUMENTEXTRACTOR_H

#include "searchdocument.h"

struct VNoteItem;

class ISearchDocumentExtractor
{
public:
    virtual ~ISearchDocumentExtractor() = default;
    virtual bool canExtract(const VNoteItem *note) const = 0;
    virtual SearchDocument extract(const VNoteItem *note) const = 0;
};

class SearchDocumentExtractor
{
public:
    static SearchDocument extract(const VNoteItem *note);
};

#endif // SEARCHDOCUMENTEXTRACTOR_H
