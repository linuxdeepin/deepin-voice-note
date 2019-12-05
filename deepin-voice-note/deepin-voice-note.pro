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
    views/leftview.cpp \
    views/leftviewsortfiltermodel.cpp \
    views/leftviewdelegate.cpp

RESOURCES += deepin-voice-note.qrc

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
    views/leftview.h \
    views/leftviewsortfiltermodel.h \
    views/leftviewdelegate.h

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
