// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webenginehandler.h"
#include "jscontent.h"
#include "metadataparser.h"
#include "vnoteitem.h"
#include "actionmanager.h"
#include "vtextspeechandtrmanager.h"

#include <QCursor>
#include <QDBusInterface>
#include <QDBusPendingReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QCollator>
#include <QFile>
#include <QTimer>
#include <QCursor>
#include <QWebEngineContextMenuRequest>

#include <dguiapplicationhelper.h>

// 获取字号接口
#ifdef OS_BUILD_V23
#    define DEEPIN_DAEMON_APPEARANCE_SERVICE "org.deepin.dde.Appearance1"
#    define DEEPIN_DAEMON_APPEARANCE_PATH "/org/deepin/dde/Appearance1"
#    define DEEPIN_DAEMON_APPEARANCE_INTERFACE "org.deepin.dde.Appearance1"
#else
#    define DEEPIN_DAEMON_APPEARANCE_SERVICE "com.deepin.daemon.Appearance"
#    define DEEPIN_DAEMON_APPEARANCE_PATH "/com/deepin/daemon/Appearance"
#    define DEEPIN_DAEMON_APPEARANCE_INTERFACE "com.deepin.daemon.Appearance"
#endif

DGUI_USE_NAMESPACE

/*!
 * \class WebEngineHandler
 * \brief 用于 WebEngineView 和 JsContent 交互处理，
 *  部分 C++ 代码与界面交互的功能移至此类处理。
 */

WebEngineHandler::WebEngineHandler(QObject *parent)
    : QObject { parent }
{
    initFontsInformation();
    connectWebContent();

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, &WebEngineHandler::onThemeChanged);
}

/**
 * @brief 初始化字体列表信息
 */
void WebEngineHandler::initFontsInformation()
{
    // 初始化字体服务Dus接口
    m_appearanceDBusInterface = new QDBusInterface(DEEPIN_DAEMON_APPEARANCE_SERVICE,
                                                   DEEPIN_DAEMON_APPEARANCE_PATH,
                                                   DEEPIN_DAEMON_APPEARANCE_INTERFACE,
                                                   QDBusConnection::sessionBus());
    if (m_appearanceDBusInterface->isValid()) {
        qInfo() << "字体服务初始化成功！字体服务： " << DEEPIN_DAEMON_APPEARANCE_SERVICE
                << " 地址：" << DEEPIN_DAEMON_APPEARANCE_PATH
                << " 接口：" << DEEPIN_DAEMON_APPEARANCE_INTERFACE;
        // 获取默认字体
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

        qInfo() << "带翻译的默认字体: " << m_defaultFont;
        // sort for display name
        std::sort(m_fontList.begin(), m_fontList.end(), [=](const QString &obj1, const QString &obj2) {
            QCollator qc;
            return qc.compare(obj1, obj2) < 0;
        });
    } else {
        qWarning() << "初始化失败！字体服务 (" << DEEPIN_DAEMON_APPEARANCE_SERVICE << ") 不存在";
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
        // Test
        DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::LightType);
        onThemeChanged();
    });

    connect(JsContent::instance(), &JsContent::popupMenu, this, &WebEngineHandler::onSaveMenuParam);
}

/*!
 * \brief 处理右键菜单请求 \a request , 完成后抛出 requestShowMenu() 信号
 */
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

/**
 * @brief 系统主题发生改变处理
 */
void WebEngineHandler::onThemeChanged()
{
    DGuiApplicationHelper *dAppHelper = DGuiApplicationHelper::instance();
    DPalette dp = dAppHelper->applicationPalette();
    // 获取系统高亮色
    QString activeHightColor = dp.color(DPalette::Active, DPalette::Highlight).name();
    QString disableHightColor = dp.color(DPalette::Disabled, DPalette::Highlight).name();
    // TODO
    // page()->setBackgroundColor(dp.base().color());
    // 获取系统主题类型
    DGuiApplicationHelper::ColorType theme = dAppHelper->themeType();
    emit JsContent::instance()->callJsSetTheme(theme, activeHightColor, disableHightColor, dp.base().color().name());
}

/*!
 * \brief 接收 web 弹出菜单类型 \a menuType 及数据 \a json
 */
void WebEngineHandler::onSaveMenuParam(int type, const QVariant &json)
{
    menuType = static_cast<Menu>(type);
    menuJson = json;
}

/*!
 * \brief 处理语音菜单请求 \a request
 */
void WebEngineHandler::processVoiceMenuRequest(QWebEngineContextMenuRequest *request)
{
    voiceBlock.reset(new VNVoiceBlock);
    MetaDataParser dataParser;
    // 解析json数据
    if (!dataParser.parse(menuJson, voiceBlock.get())) {
        return;
    }

    // 语音文件不存在使用弹出提示
    if (!QFile(voiceBlock->voicePath).exists()) {
        // 异步操作，防止阻塞前端事件
        QTimer::singleShot(0, this, [this] {
            // TODO
            // VNoteMessageDialog audioOutLimit(VNoteMessageDialog::VoicePathNoAvail);
            // audioOutLimit.exec();
            // 删除语音文本
            // deleteSelectText();
        });
        return;
    }

    // 如果当前有语音处于转换状态就将语音转文字选项置灰
    ActionManager::instance()->enableAction(ActionManager::VoiceToText, !OpsStateInterface::instance()->isVoice2Text());
    // 请求界面弹出右键菜单
    Q_EMIT requestShowMenu(VoiceMenu, request->position());
}

/*!
 * \brief 处理文本菜单请求 \a request
 */
void WebEngineHandler::processTextMenuRequest(QWebEngineContextMenuRequest *request)
{
    ActionManager::instance()->resetCtxMenu(ActionManager::MenuType::TxtCtxMenu, false);   // 重置菜单选项
    bool isAlSrvAvailabel = OpsStateInterface::instance()->isAiSrvExist();   // 获取语音服务是否可用
    bool TTSisWorking = VTextSpeechAndTrManager::isTextToSpeechInWorking();   // 获取语音服务是否正在朗读
    // 设置语音服务选项状态
    if (isAlSrvAvailabel) {
        if (TTSisWorking) {
            ActionManager::instance()->visibleAction(ActionManager::TxtStopreading, true);
            ActionManager::instance()->visibleAction(ActionManager::TxtSpeech, false);
            ActionManager::instance()->enableAction(ActionManager::TxtStopreading, true);
        } else {
            ActionManager::instance()->visibleAction(ActionManager::TxtStopreading, false);
            ActionManager::instance()->visibleAction(ActionManager::TxtSpeech, true);
        }
    }

    // 获取web端编辑标志
    QWebEngineContextMenuRequest::EditFlags flags = request->editFlags();

    // 设置普通菜单项状态
    // 可全选
    if (flags.testFlag(QWebEngineContextMenuRequest::CanSelectAll)) {
        ActionManager::instance()->enableAction(ActionManager::TxtSelectAll, true);
    }
    // 可复制
    if (flags.testFlag(QWebEngineContextMenuRequest::CanCopy)) {
        ActionManager::instance()->enableAction(ActionManager::TxtCopy, true);
        if (isAlSrvAvailabel) {
            if (VTextSpeechAndTrManager::getTransEnable()) {
                ActionManager::instance()->enableAction(ActionManager::TxtTranslate, true);
            }
            if (!TTSisWorking && VTextSpeechAndTrManager::getTextToSpeechEnable()) {
                ActionManager::instance()->enableAction(ActionManager::TxtSpeech, true);
            }
        }
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
        if (!TTSisWorking && VTextSpeechAndTrManager::getSpeechToTextEnable()) {
            ActionManager::instance()->enableAction(ActionManager::TxtDictation, true);
        }
    }

    // 请求界面弹出右键菜单
    Q_EMIT requestShowMenu(TxtMenu, request->position());
}
