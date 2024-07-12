// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webenginehandler.h"
#include "jscontent.h"

#include <QCursor>
#include <QDBusInterface>
#include <QDBusPendingReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QCollator>

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

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, &WebEngineHandler::onThemeChanged);
    connect(JsContent::instance(), &JsContent::getfontinfo, this, [this]() {
        Q_EMIT JsContent::instance()->callJsSetFontList(m_fontList, m_defaultFont);
    });

    connect(JsContent::instance(), &JsContent::loadFinsh, this, [this]() {
        // Test
        DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::LightType);
        onThemeChanged();
    });
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
