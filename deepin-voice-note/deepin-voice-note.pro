QT += core gui sql dbus multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = deepin-voice-note
TEMPLATE = app
CONFIG += c++11 link_pkgconfig
PKGCONFIG += dtkwidget dtkcore dframeworkdbus

SOURCES += \
        main.cpp \
    views/vnotemainwindow.cpp \
    vnoteapplication.cpp \
    common/vnotedatamanager.cpp \
    common/utils.cpp \
    db/vnotedbmanager.cpp \
    common/vnoteforlder.cpp \
    common/vnoteitem.cpp \
    db/vnotefolderoper.cpp \
    db/vnoteitemoper.cpp \
    common/loadfolderworker.cpp \
    common/loadnoteitemsworker.cpp \
    common/loadiconsworker.cpp \
    views/middleview.cpp \
    views/middleviewsortfiltermodel.cpp \
    views/middleviewdelegate.cpp \
    views/textnoteedit.cpp \
    views/textnoteitem.cpp \
    views/myrecodebuttons.cpp \
    views/rightnotelist.cpp \
    views/rightview.cpp \
    db/dbvisitor.cpp \
    common/datatypedef.cpp \
    widgets/vnoteiconbutton.cpp \
    dialog/vnotebasedialog.cpp \
    widgets/vnwaveform.cpp \
    common/vnoteaudiomanager.cpp \
    dialog/vnotemessagedialog.cpp \
    widgets/vnoterecordwidget.cpp \
    views/voicenoteitem.cpp \
    views/vnoterecordbar.cpp \
    common/vnotea2tmanager.cpp \
    common/vnoteaudiodevicewatcher.cpp \
    views/homepage.cpp \
    views/leftview.cpp \
    views/folderdelegate.cpp \
    views/foldertree.cpp \
    views/noteitemdelegate.cpp \
    views/notelistview.cpp

RESOURCES += \
    images.qrc

HEADERS += \
    views/vnotemainwindow.h \
    vnoteapplication.h \
    globaldef.h \
    common/vnotedatamanager.h \
    common/utils.h \
    db/vnotedbmanager.h \
    common/vnoteforlder.h \
    common/vnoteitem.h \
    db/vnotefolderoper.h \
    db/vnoteitemoper.h \
    common/datatypedef.h \
    common/loadfolderworker.h \
    common/loadnoteitemsworker.h \
    common/loadiconsworker.h \
    views/middleview.h \
    views/middleviewsortfiltermodel.h \
    views/middleviewdelegate.h \
    views/textnoteedit.h \
    views/textnoteitem.h \
    views/myrecodebuttons.h \
    views/rightnotelist.h \
    views/rightview.h \
    db/dbvisitor.h \
    widgets/vnoteiconbutton.h \
    dialog/vnotebasedialog.h \
    widgets/vnwaveform.h \
    common/vnoteaudiomanager.h \
    dialog/vnotemessagedialog.h \
    widgets/vnoterecordwidget.h \
    views/voicenoteitem.h \
    views/vnoterecordbar.h \
    common/vnotea2tmanager.h \
    common/vnoteaudiodevicewatcher.h \
    views/homepage.h \
    views/leftview.h \
    views/folderdelegate.h \
    views/foldertree.h \
    views/noteitemdelegate.h \
    views/notelistview.h

TRANSLATIONS += \
    translations/deepin-voice-note.ts

# Default rules for deployment.
isEmpty(BINDIR):BINDIR=/usr/bin
isEmpty(APPDIR):APPDIR=/usr/share/applications
isEmpty(DSRDIR):DSRDIR=/usr/share/deepin-voice-note

target.path = $$INSTROOT$$BINDIR

desktop.path = $$INSTROOT$$APPDIR
desktop.files =  deepin-voice-note.desktop

# Automating generation .qm files from .ts files
!system($$PWD/translate_generation.sh): error("Failed to generate translation")

translations.path = /usr/share/deepin-voice-note/translations
translations.files = $$PWD/translations/*.qm

icon_files.path = /usr/share/icons/hicolor/scalable/apps
icon_files.files = $$PWD/image/deepin-voice-note.svg

INSTALLS += target desktop translations icon_files
