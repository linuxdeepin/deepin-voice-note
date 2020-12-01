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
#ifndef WEBENGINEVIEW_H
#define WEBENGINEVIEW_H

#include <QObject>
#include <QTimer>

#include <QWebEngineProfile>
#include <QWebEngineUrlRequestInterceptor>
#include <QtWebChannel/QWebChannel>
#include <QtWebEngineWidgets/QWebEngineView>

#include <DMenu>

DWIDGET_USE_NAMESPACE

struct VNoteItem;
class JsContent;

class WebEngineView : public QWebEngineView
{
    Q_OBJECT
public:
    explicit WebEngineView(QWidget *parent = nullptr);
    void initData(VNoteItem *data, QString reg, bool fouse = false);
    void insertVoiceItem(const QString &voicePath, qint64 voiceSize);
    void updateNote();
    void saveMp3();
signals:
    void contentChanged();
public slots:
protected:
     void contextMenuEvent(QContextMenuEvent*e ) override;
private:
    void init();
    bool m_textChange {false};
    VNoteItem *m_noteData {nullptr};
    QWebChannel *m_channel {nullptr};
    QTimer *m_updateTimer {nullptr};
    DMenu  *m_noteDetailContextMenu{nullptr};
    JsContent *m_jsContent {nullptr};
};

#endif // WEBENGINEVIEW_H
