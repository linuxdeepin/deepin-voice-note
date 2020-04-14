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
    task/loadfolderworker.cpp \
    task/loadnoteitemsworker.cpp \
    task/loadiconsworker.cpp \
    views/middleview.cpp \
    views/middleviewdelegate.cpp \
    views/textnoteedit.cpp \
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
    common/metadataparser.cpp \
    common/actionmanager.cpp \
    common/standarditemcommon.cpp \
    views/leftviewdelegate.cpp \
    views/splashview.cpp \
    widgets/vnoteplaywidget.cpp \
    views/middleviewsortfilter.cpp \
    views/controller/textnoteeditprivate.cpp \
    views/controller/voicenoteitemprivate.cpp \
    views/controller/rightviewprivate.cpp \
    dbus/dbusvariant.cpp \
    dbus/dbuslogin1manager.cpp \
    views/textnoteitem.cpp \
    task/exportnoteworker.cpp \
    common/opsstateinterface.cpp \
    widgets/vnote2siconbutton.cpp \
    common/vnotedatasafefymanager.cpp \
    task/vntask.cpp \
    common/vntaskworker.cpp \
    task/loadsafeteydataworker.cpp \
    db/vnotesaferoper.cpp \
    task/vnmainwnddelayinittask.cpp \
    task/vndatasafertask.cpp

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
    task/loadfolderworker.h \
    task/loadnoteitemsworker.h \
    task/loadiconsworker.h \
    views/middleview.h \
    views/middleviewdelegate.h \
    views/textnoteedit.h \
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
    common/metadataparser.h \
    common/actionmanager.h \
    common/standarditemcommon.h \
    views/leftviewdelegate.h \
    views/splashview.h \
    widgets/vnoteplaywidget.h \
    views/middleviewsortfilter.h \
    views/controller/textnoteeditprivate.h \
    views/controller/voicenoteitemprivate.h \
    views/controller/rightviewprivate.h \
    dbus/dbuslogin1manager.h \
    dbus/dbusvariant.h \
    views/textnoteitem.h \
    task/exportnoteworker.h \
    common/opsstateinterface.h \
    widgets/vnote2siconbutton.h \
    common/vnotedatasafefymanager.h \
    task/vntask.h \
    common/vntaskworker.h \
    task/loadsafeteydataworker.h \
    db/vnotesaferoper.h \
    task/vnmainwnddelayinittask.h \
    task/vndatasafertask.h

TRANSLATIONS += \
    translations/deepin-voice-note.ts

# Default rules for deployment.
isEmpty(BINDIR):BINDIR=/usr/bin
isEmpty(APPDIR):APPDIR=/usr/share/applications
isEmpty(DSRDIR):DSRDIR=/usr/share/$$TARGET/
isEmpty(APPCONFIGDIR):APPCONFIGDIR=/usr/share/$$TARGET/

target.path = $$INSTROOT$$BINDIR

desktop.path = $$INSTROOT$$APPDIR
desktop.files = deepin-voice-notes.desktop

#TODO:
#    Integrate the config file to the app package.
#Now only have audio device check configuration.The
#audio check config may be override by /etc/<app>/xx.conf
app_config.path = $$INSTROOT$$APPCONFIGDIR
app_config.files = deepin-voice-note.conf

# Automating generation .qm files from .ts files
!system($$PWD/translate_generation.sh): error("Failed to generate translation")

translations.path = $$DSRDIR/translations
translations.files = $$PWD/translations/*.qm

#icon_files.path = /usr/share/icons/hicolor/scalable/apps
#icon_files.files = $$PWD/image/deepin-voice-note.svg

INSTALLS += target desktop translations icon_files app_config
