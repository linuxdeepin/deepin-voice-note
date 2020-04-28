QT += core gui sql dbus multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = deepin-voice-note
TEMPLATE = app
CONFIG += c++11 link_pkgconfig
PKGCONFIG += dtkwidget dtkcore dframeworkdbus

SOURCES += \
        src/main.cpp \
    src/views/vnotemainwindow.cpp \
    src/vnoteapplication.cpp \
    src/common/vnotedatamanager.cpp \
    src/common/utils.cpp \
    src/db/vnotedbmanager.cpp \
    src/common/vnoteforlder.cpp \
    src/common/vnoteitem.cpp \
    src/db/vnotefolderoper.cpp \
    src/db/vnoteitemoper.cpp \
    src/task/loadfolderworker.cpp \
    src/task/loadnoteitemsworker.cpp \
    src/task/loadiconsworker.cpp \
    src/views/middleview.cpp \
    src/views/middleviewdelegate.cpp \
    src/views/textnoteedit.cpp \
    src/views/rightview.cpp \
    src/db/dbvisitor.cpp \
    src/common/datatypedef.cpp \
    src/widgets/vnoteiconbutton.cpp \
    src/dialog/vnotebasedialog.cpp \
    src/widgets/vnwaveform.cpp \
    src/common/vnoteaudiomanager.cpp \
    src/dialog/vnotemessagedialog.cpp \
    src/widgets/vnoterecordwidget.cpp \
    src/views/voicenoteitem.cpp \
    src/views/vnoterecordbar.cpp \
    src/common/vnotea2tmanager.cpp \
    src/common/vnoteaudiodevicewatcher.cpp \
    src/views/homepage.cpp \
    src/views/leftview.cpp \
    src/common/metadataparser.cpp \
    src/common/actionmanager.cpp \
    src/common/standarditemcommon.cpp \
    src/views/leftviewdelegate.cpp \
    src/views/splashview.cpp \
    src/widgets/vnoteplaywidget.cpp \
    src/views/middleviewsortfilter.cpp \
    src/views/controller/textnoteeditprivate.cpp \
    src/views/controller/voicenoteitemprivate.cpp \
    src/views/controller/rightviewprivate.cpp \
    src/dbus/dbusvariant.cpp \
    src/dbus/dbuslogin1manager.cpp \
    src/views/textnoteitem.cpp \
    src/task/exportnoteworker.cpp \
    src/common/opsstateinterface.cpp \
    src/widgets/vnote2siconbutton.cpp \
    src/common/vnotedatasafefymanager.cpp \
    src/task/vntask.cpp \
    src/common/vntaskworker.cpp \
    src/task/loadsafeteydataworker.cpp \
    src/db/vnotesaferoper.cpp \
    src/task/vnmainwnddelayinittask.cpp \
    src/task/vndatasafertask.cpp \
    src/views/leftviewsortfilter.cpp \
    src/importolddata/olddbvisistors.cpp \
    src/importolddata/vnoteolddatamanager.cpp \
    src/importolddata/olddataloadwokers.cpp \
    src/importolddata/upgradeview.cpp

RESOURCES += \
    images.qrc

HEADERS += \
    src/views/vnotemainwindow.h \
    src/vnoteapplication.h \
    src/globaldef.h \
    src/common/vnotedatamanager.h \
    src/common/utils.h \
    src/db/vnotedbmanager.h \
    src/common/vnoteforlder.h \
    src/common/vnoteitem.h \
    src/db/vnotefolderoper.h \
    src/db/vnoteitemoper.h \
    src/common/datatypedef.h \
    src/task/loadfolderworker.h \
    src/task/loadnoteitemsworker.h \
    src/task/loadiconsworker.h \
    src/views/middleview.h \
    src/views/middleviewdelegate.h \
    src/views/textnoteedit.h \
    src/views/rightview.h \
    src/db/dbvisitor.h \
    src/widgets/vnoteiconbutton.h \
    src/dialog/vnotebasedialog.h \
    src/widgets/vnwaveform.h \
    src/common/vnoteaudiomanager.h \
    src/dialog/vnotemessagedialog.h \
    src/widgets/vnoterecordwidget.h \
    src/views/voicenoteitem.h \
    src/views/vnoterecordbar.h \
    src/common/vnotea2tmanager.h \
    src/common/vnoteaudiodevicewatcher.h \
    src/views/homepage.h \
    src/views/leftview.h \
    src/common/metadataparser.h \
    src/common/actionmanager.h \
    src/common/standarditemcommon.h \
    src/views/leftviewdelegate.h \
    src/views/splashview.h \
    src/widgets/vnoteplaywidget.h \
    src/views/middleviewsortfilter.h \
    src/views/controller/textnoteeditprivate.h \
    src/views/controller/voicenoteitemprivate.h \
    src/views/controller/rightviewprivate.h \
    src/dbus/dbuslogin1manager.h \
    src/dbus/dbusvariant.h \
    src/views/textnoteitem.h \
    src/task/exportnoteworker.h \
    src/common/opsstateinterface.h \
    src/widgets/vnote2siconbutton.h \
    src/common/vnotedatasafefymanager.h \
    src/task/vntask.h \
    src/common/vntaskworker.h \
    src/task/loadsafeteydataworker.h \
    src/db/vnotesaferoper.h \
    src/task/vnmainwnddelayinittask.h \
    src/task/vndatasafertask.h \
    src/views/leftviewsortfilter.h \
    src/importolddata/olddbvisistors.h \
    src/importolddata/vnoteolddatamanager.h \
    src/importolddata/olddataloadwokers.h \
    src/importolddata/upgradeview.h

INCLUDEPATH += \
    src/

TRANSLATIONS += \
    translations/deepin-voice-note.ts

# Default rules for deployment.
isEmpty(BINDIR):BINDIR=/usr/bin
isEmpty(APPDIR):APPDIR=/usr/share/applications
isEmpty(DSRDIR):DSRDIR=/usr/share/$$TARGET/
isEmpty(APPCONFIGDIR):APPCONFIGDIR=/usr/share/$$TARGET/

target.path = $$INSTROOT$$BINDIR

desktop.path = $$INSTROOT$$APPDIR
desktop.files = deepin-voice-note.desktop

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
