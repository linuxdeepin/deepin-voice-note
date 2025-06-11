// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "web_engine_handler.h"
#include "jscontent.h"
#include "metadataparser.h"
#include "vnoteitem.h"
#include "actionmanager.h"
#include "vtextspeechandtrmanager.h"
#include "voice_player_handler.h"
#include "voice_to_text_handler.h"

#include <QApplication>
#include <QCursor>
#include <QDBusInterface>
#include <QDBusPendingReply>
#include <QKeyEvent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QCollator>
#include <QFile>
#include <QTimer>
#include <QCursor>
#include <QMimeData>
// 条件编译：QWebEngineContextMenuRequest 只在 Qt6 中存在
#ifndef USE_QT5
#include <QWebEngineContextMenuRequest>
#endif
#include <QFileDialog>
#include <QStandardPaths>
// 条件编译：根据 Qt 版本包含不同的音频设备头文件
#ifdef USE_QT5
#include <QAudioDeviceInfo>
#include <QAudio>
#else
#include <QMediaDevices>
#include <QAudioDevice>
#endif

#include <dguiapplicationhelper.h>

// 获取字号接口
#ifdef OS_BUILD_V23
#define DEEPIN_DAEMON_APPEARANCE_SERVICE "org.deepin.dde.Appearance1"
#define DEEPIN_DAEMON_APPEARANCE_PATH "/org/deepin/dde/Appearance1"
#define DEEPIN_DAEMON_APPEARANCE_INTERFACE "org.deepin.dde.Appearance1"
#else
#define DEEPIN_DAEMON_APPEARANCE_SERVICE "com.deepin.daemon.Appearance"
#define DEEPIN_DAEMON_APPEARANCE_PATH "/com/deepin/daemon/Appearance"
#define DEEPIN_DAEMON_APPEARANCE_INTERFACE "com.deepin.daemon.Appearance"
#endif

DGUI_USE_NAMESPACE

/*!
 * \class WebEngineHandler
 * \brief 用于 WebEngineView 和 JsContent 交互处理，
 *  部分 C++ 代码与界面交互的功能移至此类处理。
 */

WebEngineHandler::WebEngineHandler(QObject *parent)
    : QObject{parent}
    , m_voicePlayerHandler(new VoicePlayerHandler(this))
    , m_voiceToTextHandler(new VoiceToTextHandler(this))
{
    initFontsInformation();
    connectWebContent();

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &WebEngineHandler::onThemeChanged);

    connect(ActionManager::instance(), &ActionManager::actionTriggered, this, &WebEngineHandler::onMenuClicked);

    connect(m_voiceToTextHandler, &VoiceToTextHandler::audioLengthLimit, this, [this]() {
        Q_EMIT requestMessageDialog(VNoteMessageDialogHandler::AsrTimeLimit);
    });
    connect(m_voiceToTextHandler, &VoiceToTextHandler::noNetworkConnection, this, [this]() {
            Q_EMIT requestMessageDialog(VNoteMessageDialogHandler::NoNetwork);
    });
    connect(m_voicePlayerHandler, &VoicePlayerHandler::playStatusChanged, this, [=](VoicePlayerHandler::PlayState state){
        if (state == VoicePlayerHandler::PlayState::End) {
            Q_EMIT playingVoice(false);
        } else {
            Q_EMIT playingVoice(true);
        }
    });
}

QObject *WebEngineHandler::target() const
{
    return m_targetWebEngine;
}

void WebEngineHandler::setTarget(QObject *targetWebEngine)
{
    if (targetWebEngine != m_targetWebEngine) {
        m_targetWebEngine = targetWebEngine;
        Q_EMIT targetChanged();
    }
}

/**
   @return 通过 WebEngineView (qml) 调用 js 函数 \a function ，
    通过事件循环等待 onCallJsResult() 接收返回值。
 */
QVariant WebEngineHandler::callJsSynchronous(const QString &function)
{
    if (m_callJsLoop.isRunning()) {
        qCritical() << "reentrant call js function!";
        return {};
    }

    m_callJsResult.clear();
    if (target()) {
        Q_EMIT requesetCallJsSynchronous(function);
        m_callJsLoop.exec();
    }

    return m_callJsResult;
}

/**
   @brief 等待 callJsSynchronous() js 函数调用完成
 */
void WebEngineHandler::onCallJsResult(const QVariant &result)
{
    m_callJsResult = result;
    m_callJsLoop.exit();
}

/**
 * @brief 初始化字体列表信息
 */
void WebEngineHandler::initFontsInformation()
{
    qDebug() << "Initializing fonts information";
    // 初始化字体服务Dus接口
    m_appearanceDBusInterface = new QDBusInterface(DEEPIN_DAEMON_APPEARANCE_SERVICE,
                                                   DEEPIN_DAEMON_APPEARANCE_PATH,
                                                   DEEPIN_DAEMON_APPEARANCE_INTERFACE,
                                                   QDBusConnection::sessionBus());
    if (m_appearanceDBusInterface->isValid()) {
        qInfo() << "Font service initialized successfully. Service:" << DEEPIN_DAEMON_APPEARANCE_SERVICE 
                << "Path:" << DEEPIN_DAEMON_APPEARANCE_PATH 
                << "Interface:" << DEEPIN_DAEMON_APPEARANCE_INTERFACE;
        
        QString defaultfont = m_appearanceDBusInterface->property("StandardFont").value<QString>();
        // 获取字体列表
        QDBusPendingReply<QString> font = m_appearanceDBusInterface->call("List", "standardfont");

        QJsonArray array = QJsonDocument::fromJson(font.value().toLocal8Bit().data()).array();
        QStringList list;
        for (int i = 0; i != array.size(); i++) {
            list << array.at(i).toString();
        }
        QList<QVariant> arg;
        arg << "standardfont" << list;
        // 获取带翻译的字体列表
        QDBusPendingReply<QString> font1 = m_appearanceDBusInterface->callWithArgumentList(QDBus::AutoDetect, "Show", arg);

        QJsonArray arrayValue = QJsonDocument::fromJson(font1.value().toLocal8Bit().data()).array();

        // 列表格式转换
        for (int i = 0; i != arrayValue.size(); i++) {
            QJsonObject object = arrayValue.at(i).toObject();
            object.insert("type", QJsonValue("standardfont"));
            m_fontList << object["Name"].toString();
            if (defaultfont == object["Id"].toString()) {
                // 根据id 获取带翻译的默认字体
                m_defaultFont = object["Name"].toString();
            }
        }

        qInfo() << "Default font with translation:" << m_defaultFont;
        // sort for display name
        std::sort(m_fontList.begin(), m_fontList.end(), [=](const QString &obj1, const QString &obj2) {
            QCollator qc;
            return qc.compare(obj1, obj2) < 0;
        });
        qDebug() << "Font list sorted, total fonts:" << m_fontList.size();
    } else {
        qWarning() << "Font service initialization failed. Service:" << DEEPIN_DAEMON_APPEARANCE_SERVICE << "not found";
    }
}

/*!
 * \brief 连接 web 内容处理器
 */
void WebEngineHandler::connectWebContent()
{
    connect(JsContent::instance(), &JsContent::getfontinfo, this, [this]() {
        Q_EMIT JsContent::instance()->callJsSetFontList(m_fontList, m_defaultFont);
    });

    connect(JsContent::instance(), &JsContent::loadFinsh, this, [this]() {
        Q_EMIT loadRichText();
        onThemeChanged();
    });

    connect(JsContent::instance(), &JsContent::popupMenu, this, &WebEngineHandler::onSaveMenuParam);
    connect(JsContent::instance(), &JsContent::textPaste, this, &WebEngineHandler::onPaste);
    connect(JsContent::instance(), &JsContent::viewPictrue, this, &WebEngineHandler::viewPicture);
}

/*!
 * \brief 处理右键菜单请求 \a request , 完成后抛出 requestShowMenu() 信号
 */
#ifdef USE_QT5
void WebEngineHandler::onContextMenuRequested(QObject *request)
{
    // 从 QObject* 中安全地获取属性
    const QPoint pos = request->property("position").toPoint();

    switch (menuType) {
    case VoiceMenu: {
        processVoiceMenuRequest(request);
        break;
    }
    case PictureMenu: {
        // 图片菜单的逻辑很简单，直接弹出
        Q_EMIT requestShowMenu(menuType, pos);
        break;
    }
    case TxtMenu: {
        // 文本菜单的状态已由QML处理，这里调用C++是为了处理未来可能的特殊逻辑
        processTextMenuRequest(request);
        break;
    }
    default:
        break;
    }
}
#else
void WebEngineHandler::onContextMenuRequested(QWebEngineContextMenuRequest *request)
{
    switch (menuType) {
    case VoiceMenu: {
        processVoiceMenuRequest(request);
    } break;
    case PictureMenu: {
        Q_EMIT requestShowMenu(menuType, request->position());
    } break;
    case TxtMenu: {
        processTextMenuRequest(request);
    } break;
    default:
        break;
    }
}
#endif

/**
   @brief 插入音频文件， \a voicePath 为音频文件路径， \a voiceSize 为音频的长度，单位 ms
 */
void WebEngineHandler::onInsertVoiceItem(const QString &voicePath, quint64 voiceSize)
{
    qDebug() << "Inserting voice item, path:" << voicePath << "size:" << voiceSize << "ms";

    VNVoiceBlock data;
    data.ptrVoice->voiceSize = voiceSize;
    data.ptrVoice->voicePath = voicePath;
    data.ptrVoice->createTime = QDateTime::currentDateTime();
    data.ptrVoice->voiceTitle = data.ptrVoice->createTime.toString("yyyyMMdd hh.mm.ss");

    MetaDataParser parse;
    QVariant value;
    parse.makeMetaData(&data, value);

    // 关闭应用时，需要同步插入语音并进行后台更新
    if (OpsStateInterface::instance()->isAppQuit()) {
        qDebug() << "App is quitting, performing synchronous voice insertion";
        callJsSynchronous(QString("insertVoiceItem('%1')").arg(value.toString()));
        return;
    }
    emit JsContent::instance()->callJsInsertVoice(value.toString());
}

/**
 * @brief 系统主题发生改变处理
 */
void WebEngineHandler::onThemeChanged()
{
    qDebug() << "Theme changed, updating UI";
    DGuiApplicationHelper *dAppHelper = DGuiApplicationHelper::instance();
    DPalette dp = dAppHelper->applicationPalette();
    // 获取系统高亮色
    QString activeHightColor = dp.color(DPalette::Active, DPalette::Highlight).name();
    QString disableHightColor = dp.color(DPalette::Disabled, DPalette::Highlight).name();
    // 获取系统主题类型
    DGuiApplicationHelper::ColorType theme = dAppHelper->themeType();

    QString backgroundColor = DGuiApplicationHelper::LightType == theme ? "#FBFCFD" : "#090A17";
    // 现在背景色主要由 qml 组件和 web 前端 css 共同实现，但在 sw 下保留兼容设置
    emit JsContent::instance()->callJsSetTheme(theme, activeHightColor, disableHightColor, backgroundColor);
}

/**
 * @brief 接收 web 弹出菜单类型 \a menuType 及数据 \a json
 */
void WebEngineHandler::onSaveMenuParam(int type, const QVariant &json)
{
    menuType = static_cast<Menu>(type);
    menuJson = json;
}

/**
   @brief 右键菜单点击时触发，根据不同动作类型 \a kind 执行对应操作
 */
void WebEngineHandler::onMenuClicked(ActionManager::ActionKind kind)
{
    switch (kind) {
        case ActionManager::VoiceAsSave:
            // 另存语音
            if (!saveMP3()) {
                qWarning() << "Failed to save MP3";
                Q_EMIT requestMessageDialog(VNoteMessageDialogHandler::SaveFailed);
            }
            break;
        case ActionManager::VoiceToText:
            m_voiceToTextHandler->setAudioToText(m_voiceBlock);
            break;
        case ActionManager::VoiceDelete:
        case ActionManager::PictureDelete:
        case ActionManager::TxtDelete:
            // 调用 js 删除选中文本
            Q_EMIT JsContent::instance()->callJsDeleteSelection();
            break;
        case ActionManager::VoiceSelectAll:
        case ActionManager::PictureSelectAll:
        case ActionManager::TxtSelectAll:
            // 模拟全选快捷键ctrl+A
            Q_EMIT triggerWebAction(QWebEnginePage::SelectAll);
            break;
        case ActionManager::VoiceCopy:
        case ActionManager::PictureCopy:
        case ActionManager::TxtCopy:
            // 直接调用web端的复制事件
            Q_EMIT triggerWebAction(QWebEnginePage::Copy);
            break;
        case ActionManager::VoiceCut:
        case ActionManager::PictureCut:
        case ActionManager::TxtCut:
            // 直接调用web端的剪切事件
            Q_EMIT triggerWebAction(QWebEnginePage::Cut);
            break;
        case ActionManager::VoicePaste:
        case ActionManager::PicturePaste:
        case ActionManager::TxtPaste:
            // 粘贴事件，从剪贴板获取数据
            onPaste(isVoicePaste());
            break;
        case ActionManager::PictureView:
            // 查看图片
            Q_EMIT viewPicture(menuJson.toString());
            break;
        case ActionManager::PictureSaveAs:
            // 另存图片
            savePictureAs();
            break;
        case ActionManager::TxtSpeech: {
#ifdef USE_QT5
            // Qt5 中使用 QAudioDeviceInfo 检查音频输出设备
            QList<QAudioDeviceInfo> outputDevices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
            if (outputDevices.isEmpty()) {
                qWarning() << "No audio output devices available";
                QString errString = VTextSpeechAndTrManager::instance()->errorString(VTextSpeechAndTrManager::NoOutputDevice);
                if (!errString.isEmpty()) {
                    Q_EMIT popupToast(errString, VTextSpeechAndTrManager::NoOutputDevice);
                }
                break;
            }
#else
            QList<QAudioDevice> outputDevices = QMediaDevices::audioOutputs();
            if (outputDevices.isEmpty()) {
                qWarning() << "No audio output devices available";
                QString errString = VTextSpeechAndTrManager::instance()->errorString(VTextSpeechAndTrManager::NoOutputDevice);
                if (!errString.isEmpty()) {
                    Q_EMIT popupToast(errString, VTextSpeechAndTrManager::NoOutputDevice);
                }
                break;
            }
#endif
            auto status = VTextSpeechAndTrManager::instance()->onTextToSpeech();
            if (VTextSpeechAndTrManager::Success != status) {
                qWarning() << "Text to speech failed with status:" << status;
                QString errString = VTextSpeechAndTrManager::instance()->errorString(status);
                if (!errString.isEmpty()) {
                    Q_EMIT popupToast(errString, status);
                }
            }
            break;
        }
        case ActionManager::TxtStopreading: {
            auto status = VTextSpeechAndTrManager::instance()->onStopTextToSpeech();
            if (VTextSpeechAndTrManager::Success != status) {
                QString errString = VTextSpeechAndTrManager::instance()->errorString(status);
                if (!errString.isEmpty()) {
                    Q_EMIT popupToast(errString, status);
                }
            }
            break;
        }
        case ActionManager::TxtDictation: {
            auto status = VTextSpeechAndTrManager::instance()->onSpeechToText();
            if (VTextSpeechAndTrManager::Success != status) {
                QString errString = VTextSpeechAndTrManager::instance()->errorString(status);
                if (!errString.isEmpty()) {
                    Q_EMIT popupToast(errString, status);
                }
            }
            break;
        }
        // case ActionManager::TxtTranslate:
        //     VTextSpeechAndTrManager::onTextTranslate();
        //     break;
        default:
            break;
    }
}

/**
   @brief 拷贝剪贴内容，\a isVoice 标识音频数据，仅在语音记事本内使用，因此使用js前端的拷贝
    处理，其他类型(图片/文本)可通过剪贴版导入。
 */
void WebEngineHandler::onPaste(bool isVoice)
{
    qDebug() << "Paste operation requested, isVoice:" << isVoice;
    if (isVoice) {
        Q_EMIT triggerWebAction(QWebEnginePage::Paste);
        return;
    }

    // 获取剪贴板信息
    QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    // 存在文件url
    if (mimeData->hasUrls()) {
        QStringList paths;
        for (auto url : mimeData->urls()) {
            paths.push_back(url.path());
        }
        qDebug() << "Pasting URLs:" << paths;
        JsContent::instance()->insertImages(paths);
    } else if (mimeData->hasImage()) {
        qDebug() << "Pasting image from clipboard";
        JsContent::instance()->insertImages(qvariant_cast<QImage>(mimeData->imageData()));
    } else {
        // 无图片文件，直接调用web端的粘贴事件
        qDebug() << "Pasting text content";
        Q_EMIT triggerWebAction(QWebEnginePage::Paste);
    }
}

/*!
 * \brief 处理语音菜单请求 \a request
 */
#ifdef USE_QT5
void WebEngineHandler::processVoiceMenuRequest(QObject *request)
{
    m_voiceBlock.reset(new VNVoiceBlock);
    MetaDataParser dataParser;
    // 解析json数据
    if (!dataParser.parse(menuJson, m_voiceBlock.get())) {
        return;
    }

    // 语音文件不存在使用弹出提示
    if (!QFile(m_voiceBlock->voicePath).exists()) {
        // 异步操作，防止阻塞前端事件
        QTimer::singleShot(0, this, [this] {
            Q_EMIT requestMessageDialog(VNoteMessageDialogHandler::VoicePathNoAvail);
            // 调用 js 删除删除语音文本
            Q_EMIT JsContent::instance()->callJsDeleteSelection();
        });
        return;
    }

    // 如果当前有语音处于转换状态就将语音转文字选项置灰 (此逻辑已移至QML onAboutToShow，此处为兜底)
    ActionManager::instance()->enableAction(ActionManager::VoiceToText, !OpsStateInterface::instance()->isVoice2Text());
    // 请求界面弹出右键菜单
    const QPoint pos = request->property("position").toPoint();
    Q_EMIT requestShowMenu(VoiceMenu, pos);
}
#else
void WebEngineHandler::processVoiceMenuRequest(QWebEngineContextMenuRequest *request)
{
    m_voiceBlock.reset(new VNVoiceBlock);
    MetaDataParser dataParser;
    // 解析json数据
    if (!dataParser.parse(menuJson, m_voiceBlock.get())) {
        return;
    }

    // 语音文件不存在使用弹出提示
    if (!QFile(m_voiceBlock->voicePath).exists()) {
        // 异步操作，防止阻塞前端事件
        QTimer::singleShot(0, this, [this] {
            Q_EMIT requestMessageDialog(VNoteMessageDialogHandler::VoicePathNoAvail);
            // 调用 js 删除删除语音文本
            Q_EMIT JsContent::instance()->callJsDeleteSelection();
        });
        return;
    }

    // 如果当前有语音处于转换状态就将语音转文字选项置灰
    ActionManager::instance()->enableAction(ActionManager::VoiceToText, !OpsStateInterface::instance()->isVoice2Text());
    // 请求界面弹出右键菜单
    Q_EMIT requestShowMenu(VoiceMenu, request->position());
}
#endif

/*!
 * \brief 处理文本菜单请求 \a request
 */
#ifdef USE_QT5
void WebEngineHandler::processTextMenuRequest(QObject *request)
{
    // 此处的逻辑是"兜底"，主要逻辑已由QML实现，以确保在所有情况下状态正确
    const int intFlags = request->property("editFlags").toInt();
    auto flags = static_cast<QWebEngineContextMenuData::EditFlags>(intFlags);

    ActionManager::instance()->resetCtxMenu(ActionManager::MenuType::TxtCtxMenu, false); // 重置菜单选项

    ActionManager::instance()->visibleAction(ActionManager::TxtStopreading, false);

    // 设置普通菜单项状态
    if (flags.testFlag(QWebEngineContextMenuData::CanSelectAll)) {
        ActionManager::instance()->enableAction(ActionManager::TxtSelectAll, true);
    }
    if (flags.testFlag(QWebEngineContextMenuData::CanCopy)) {
        ActionManager::instance()->enableAction(ActionManager::TxtCopy, true);
        ActionManager::instance()->enableAction(ActionManager::TxtSpeech, true);
    }
    if (flags.testFlag(QWebEngineContextMenuData::CanCut)) {
        ActionManager::instance()->enableAction(ActionManager::TxtCut, true);
    }
    if (flags.testFlag(QWebEngineContextMenuData::CanDelete)) {
        ActionManager::instance()->enableAction(ActionManager::TxtDelete, true);
    }
    if (flags.testFlag(QWebEngineContextMenuData::CanPaste)) {
        ActionManager::instance()->enableAction(ActionManager::TxtPaste, true);
        ActionManager::instance()->enableAction(ActionManager::TxtDictation, true);
    }

    const QPoint pos = request->property("position").toPoint();
    Q_EMIT requestShowMenu(TxtMenu, pos);
}
#else
void WebEngineHandler::processTextMenuRequest(QWebEngineContextMenuRequest *request)
{
    ActionManager::instance()->resetCtxMenu(ActionManager::MenuType::TxtCtxMenu, false); // 重置菜单选项

    // TASK-37707: Disable now
    ActionManager::instance()->visibleAction(ActionManager::TxtStopreading, false);

    auto flags = request->editFlags();

    // 设置普通菜单项状态
    // 可全选
    if (flags.testFlag(QWebEngineContextMenuRequest::CanSelectAll)) {
        ActionManager::instance()->enableAction(ActionManager::TxtSelectAll, true);
    }
    // 可复制
    if (flags.testFlag(QWebEngineContextMenuRequest::CanCopy)) {
        ActionManager::instance()->enableAction(ActionManager::TxtCopy, true);
        ActionManager::instance()->enableAction(ActionManager::TxtSpeech, true);
    }
    // 可剪切
    if (flags.testFlag(QWebEngineContextMenuRequest::CanCut)) {
        ActionManager::instance()->enableAction(ActionManager::TxtCut, true);
    }
    // 可删除
    if (flags.testFlag(QWebEngineContextMenuRequest::CanDelete)) {
        ActionManager::instance()->enableAction(ActionManager::TxtDelete, true);
    }
    // 可粘贴
    if (flags.testFlag(QWebEngineContextMenuRequest::CanPaste)) {
        ActionManager::instance()->enableAction(ActionManager::TxtPaste, true);
        ActionManager::instance()->enableAction(ActionManager::TxtDictation, true);
    }

    // 请求界面弹出右键菜单
    Q_EMIT requestShowMenu(TxtMenu, request->position());
}
#endif

/*!
 * \brief 处理图片菜单请求
 */
#ifdef USE_QT5
// Qt5 下图片菜单直接由 onContextMenuRequested 发出信号，无需单独的 process 函数
#else
void WebEngineHandler::processPictureMenuRequest(QWebEngineContextMenuRequest *request)
{
    ActionManager::instance()->resetCtxMenu(ActionManager::MenuType::PictureCtxMenu, true);

    auto flags = request->editFlags();

    // 设置普通菜单项状态
    // 可全选
    if (flags.testFlag(QWebEngineContextMenuRequest::CanSelectAll)) {
        ActionManager::instance()->enableAction(ActionManager::PictureSelectAll, true);
    }
    // 可复制
    if (flags.testFlag(QWebEngineContextMenuRequest::CanCopy)) {
        ActionManager::instance()->enableAction(ActionManager::PictureCopy, true);
    }
    // 可剪切
    if (flags.testFlag(QWebEngineContextMenuRequest::CanCut)) {
        ActionManager::instance()->enableAction(ActionManager::PictureCut, true);
    }
    // 可删除
    if (flags.testFlag(QWebEngineContextMenuRequest::CanDelete)) {
        ActionManager::instance()->enableAction(ActionManager::PictureDelete, true);
    }
    // 可粘贴
    if (flags.testFlag(QWebEngineContextMenuRequest::CanPaste)) {
        ActionManager::instance()->enableAction(ActionManager::PicturePaste, true);
    }

    Q_EMIT requestShowMenu(PictureMenu, request->position());
}
#endif

/**
   @return 返回当前拷贝数据是否包含录音
 */
bool WebEngineHandler::isVoicePaste()
{
    // 调用web前端接口
    return callJsSynchronous("returnCopyFlag()").toBool();
}

bool WebEngineHandler::saveMP3()
{
    if (!m_voiceBlock) {
        qWarning() << "No voice block available for saving";
        return false;
    }
    QString defaultName = QStandardPaths::writableLocation(QStandardPaths::MusicLocation) + "/" + m_voiceBlock.get()->voiceTitle + ".mp3";
    qDebug() << "Saving MP3 with default name:" << defaultName;
    QString fileName = QFileDialog::getSaveFileName(0, tr("save as MP3"), defaultName, "*.mp3");
    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName);
        QDir dir = fileInfo.absoluteDir();
        if (!dir.exists()) {
            qWarning() << "Target directory does not exist:" << dir.path();
            return false;
        }
        QFile tmpFile(m_voiceBlock.get()->voicePath);
        return tmpFile.copy(fileName);
    }
    return true;
}

void WebEngineHandler::savePictureAs()
{
    QString originalPath = menuJson.toString();  // 获取原图片路径
    saveAsFile(originalPath, QStandardPaths::writableLocation(QStandardPaths::PicturesLocation), "image");
}

QString WebEngineHandler::saveAsFile(const QString &originalPath, QString dirPath, const QString &defalutName)
{
    // 存储文件夹默认为桌面
    qDebug() << "Saving file, original path:" << originalPath << "default name:" << defalutName;
    if (dirPath.isEmpty()) {
        dirPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }

    QFileInfo fileInfo(originalPath);
    QString filter = "*." + fileInfo.suffix();
    QString baseName = defalutName.isEmpty() ? fileInfo.baseName() : defalutName;
    QString dir = QString("%1/%2").arg(dirPath).arg(baseName + "." + fileInfo.suffix());
    // 获取需要保存的文件位置，默认路径为用户图片文件夹，默认文件名为原文件名
    QString newPath = QFileDialog::getSaveFileName(0, "", dir, filter);
    if (newPath.isEmpty()) {
        qDebug() << "Save operation cancelled by user";
        return "";
    }
    // 添加文件后缀
    if (!newPath.endsWith(fileInfo.suffix())) {
        newPath += ("." + fileInfo.suffix());
    }

    QFileInfo info(newPath);
    if (!QFileInfo(info.dir().path()).isWritable()) {
        qWarning() << "No write permission for directory:" << info.dir().path();
        emit requestMessageDialog(VNoteMessageDialogHandler::NoPermission);
        return "";
    }
    if (info.exists()) {
        // 文件已存在，删除原文件
        if (!info.isWritable()) {
            qWarning() << "No write permission for file:" << newPath;
            emit requestMessageDialog(VNoteMessageDialogHandler::NoPermission);
            return "";
        }
        QFile::remove(newPath);
    }

    // 复制文件
    if (!QFile::copy(originalPath, newPath)) {
        qCritical() << "File copy failed from:" << originalPath << "to:" << newPath;
        emit requestMessageDialog(VNoteMessageDialogHandler::SaveFailed);
        return "";
    }
    qInfo() << "File saved successfully to:" << newPath;
    return newPath;
}
