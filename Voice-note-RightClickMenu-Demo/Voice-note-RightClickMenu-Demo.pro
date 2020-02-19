QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Voice-note-RightClickMenu-Demo
TEMPLATE = app
CONFIG += c++11 link_pkgconfig
PKGCONFIG += dtkwidget

SOURCES += \
        main.cpp \
    dnoterightclickmainwindow.cpp \
    VoiceNoteRightClcikMenu/dnotercmenu.cpp \
    VoiceNoteRightClcikMenu/ddirmenu.cpp \
    VoiceNoteRightClcikMenu/dvoicemenu.cpp

RESOURCES +=         resources.qrc

HEADERS += \
    dnoterightclickmainwindow.h \
    VoiceNoteRightClcikMenu/dnotercmenu.h \
    VoiceNoteRightClcikMenu/ddirmenu.h \
    VoiceNoteRightClcikMenu/dvoicemenu.h

TRANSLATIONS += ./rcmenu_zh.ts
translations.path = /usr/share/deepin-voice-note/translations
translations.files += ./rcmenu_zh.qm
