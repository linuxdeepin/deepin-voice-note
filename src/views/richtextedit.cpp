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
#include "richtextedit.h"
#include "common/jscontent.h"
#include "common/vnoteitem.h"
#include "common/metadataparser.h"
#include "db/vnoteitemoper.h"
#include "common/actionmanager.h"
#include "common/vtextspeechandtrmanager.h"

#include <DFileDialog>
#include <DStandardPaths>
#include <DApplication>

#include <QClipboard>
#include <QVBoxLayout>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QWebEngineContextMenuData>

static const char webPage[] = WEB_PATH "/index.html";

RichTextEdit::RichTextEdit(QWidget *parent)
    : QWidget(parent)
{
    m_jsContent = JsContent::instance();
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(1000);
    connect(m_updateTimer, &QTimer::timeout, this, &RichTextEdit::updateNote);
    connect(m_jsContent, &JsContent::textChange, this, &RichTextEdit::onTextChange);
    connect(m_jsContent, &JsContent::setDataFinsh, this, &RichTextEdit::onSetDataFinsh);
    //初始化编辑区，ut测试中此函数打桩解决无法新建QWebEngineView问题
    initWebView();
    initRightMenu();
    initConnections();
}

void RichTextEdit::initData(VNoteItem *data, const QString &reg, bool fouse)
{
    if (fouse && m_webView) {
        m_webView->setFocus();
    }
    if (data == nullptr) {
        this->setVisible(false);
        return;
    }
    this->setVisible(true);
    m_searchKey = reg;

    if (m_noteData != data) { //笔记切换时设置笔记内容
        m_updateTimer->stop();
        updateNote();
        m_noteData = data;
        if (data->htmlCode.isEmpty()) {
            emit m_jsContent->callJsInitData(data->metaDataRef().toString());
        } else {
            emit m_jsContent->callJsSetHtml(data->htmlCode);
        }
        m_updateTimer->start();
    } else if (m_webView) { //笔记相同时执行搜索
        m_webView->findText(reg);
    }
}

void RichTextEdit::insertVoiceItem(const QString &voicePath, qint64 voiceSize)
{
    VNVoiceBlock data;
    data.ptrVoice->voiceSize = voiceSize;
    data.ptrVoice->voicePath = voicePath;
    data.ptrVoice->createTime = QDateTime::currentDateTime();
    data.ptrVoice->voiceTitle = data.ptrVoice->createTime.toString("yyyyMMdd hh.mm.ss");

    MetaDataParser parse;
    QVariant value;
    parse.makeMetaData(&data, value);
    this->setFocus();
    emit m_jsContent->callJsInsertVoice(value.toString());
}

void RichTextEdit::updateNote()
{
    if (m_noteData && m_webView) {
        if (m_textChange || m_noteData->htmlCode.isEmpty()) {
            QVariant result = m_jsContent->callJsSynchronous(m_webView->page(), QString("getHtml()"));
            if (result.isValid()) {
                m_noteData->htmlCode = result.toString();
                VNoteItemOper noteOps(m_noteData);
                if (!noteOps.updateNote()) {
                    qInfo() << "Save note error";
                }
            }
            m_textChange = false;
        }
    }
}

/**
 * @brief RichTextEdit::getImagePathsByClipboard
 * 从系统剪贴板中获取图片路径
 */
void RichTextEdit::getImagePathsByClipboard()
{
    //获取剪贴板信息
    QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    //存在文件url
    if (mimeData->hasUrls()) {
        QStringList paths;
        for (auto url : mimeData->urls()) {
            paths.push_back(url.path());
        }
        m_jsContent->insertImages(paths);
    } else if (mimeData->hasImage()) {
        QPixmap pixmap = clipboard->pixmap();
        QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/image";
        QDir dir;
        //文件夹是否存在
        if (!dir.exists(dirPath)) {
            dir.mkdir(dirPath); //创建文件夹
        }
        //保存文件，文件名为当前年月日时分秒，后缀为png
        QDateTime currentDateTime = QDateTime::currentDateTime();
        QString fileName = currentDateTime.toString("yyyyMMddhhmmss.png");
        if (pixmap.save(dirPath + "/" + fileName)) {
            m_jsContent->insertImages(QStringList(dirPath + "/" + fileName));
        }
    } else {
        //无图片文件，直接调用web端的粘贴事件
        m_webView->page()->triggerAction(QWebEnginePage::Paste);
    }
}

void RichTextEdit::onTextChange()
{
    m_textChange = true;
}

void RichTextEdit::initWebView()
{
    m_channel = new QWebChannel(this);
    m_channel->registerObject("webobj", m_jsContent);
    m_webView = new WebView(this);
    m_webView->page()->setWebChannel(m_channel);
    QFileInfo info(webPage);
    m_webView->load(QUrl::fromLocalFile(info.absoluteFilePath()));
    m_webView->setBackgroundRole(QPalette::Highlight);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_webView);
    m_webView->installEventFilter(this);
}

/**
 * @brief RichTextEdit::initRightMenu
 * 初始化右键菜单
 */
void RichTextEdit::initRightMenu()
{
    m_pictureRightMenu = ActionManager::Instance()->pictureContextMenu();
    m_voiceRightMenu = ActionManager::Instance()->voiceContextMenu();
    m_txtRightMenu = ActionManager::Instance()->txtContextMenu();
}

/**
 * @brief RichTextEdit::initConnections
 * 初始化信号连接
 */
void RichTextEdit::initConnections()
{
    connect(m_jsContent, &JsContent::popupMenu, this, &RichTextEdit::saveMenuParam);
    connect(m_webView, &WebView::sendMenuEvent, this, &RichTextEdit::webContextMenuEvent);
    connect(ActionManager::Instance()->voiceContextMenu(), &DMenu::triggered,
            this, &RichTextEdit::onMenuActionClicked);
    connect(ActionManager::Instance()->pictureContextMenu(), &DMenu::triggered,
            this, &RichTextEdit::onMenuActionClicked);
    connect(ActionManager::Instance()->txtContextMenu(), &DMenu::triggered,
            this, &RichTextEdit::onMenuActionClicked);
}

/**
 * @brief VNoteMainWindow::onImgInsertClicked
 * 图片插入点击事件响应
 */
void RichTextEdit::onImgInsertClicked()
{
    QStringList filePaths = DFileDialog::getOpenFileNames(
        this,
        "Please choose an image file",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        "Image file(*.jpg *.png *.bmp)");

    m_jsContent->insertImages(filePaths);
}

/**
 * @brief VNoteMainWindow::onPaste
 * 粘贴事件
 */
void RichTextEdit::onPaste()
{
    getImagePathsByClipboard();
}

/**
 * @brief saveMenuParam 接收web弹出菜单类型及数据
 * @param type  菜单类型
 * @param x 弹出位置横坐标
 * @param y 弹出位置纵坐标
 * @param json  内容
 */
void RichTextEdit::saveMenuParam(int type, int x, int y, const QVariant &json)
{
    m_menuType = static_cast<Menu>(type);
    m_menuPoint.setX(x);
    m_menuPoint.setY(y);
    m_menuJson = json;
}

/**
 * @brief RichTextEdit::eventFilter
 * 拦截web界面的Drop事件处理
 * @param o
 * @param e
 * @return
 */
bool RichTextEdit::eventFilter(QObject *o, QEvent *e)
{
    if (o == m_webView) {
        //web控件的Drop事件
        if (e->type() == QEvent::Drop) {
            QDropEvent *event = static_cast<QDropEvent *>(e);
            // 获取文件路径
            if (event->mimeData()->hasUrls()) {
                QStringList paths;
                //获取文件路径
                for (auto url : event->mimeData()->urls()) {
                    paths.push_back(url.path());
                }
                m_jsContent->insertImages(paths); //向web端发送路径
            }
            return true;
        }
    }
    return QWidget::eventFilter(o, e);
}

void RichTextEdit::onSetDataFinsh()
{
    //只有编辑区内容加载完成才能搜索
    if (!m_searchKey.isEmpty()) {
        m_webView->findText(m_searchKey);
    }
}

/**
 * @brief web端右键响应
 */
void RichTextEdit::webContextMenuEvent(QContextMenuEvent *e)
{
    Q_UNUSED(e);
    switch (m_menuType) {
    case PictureMenu:
        //图片菜单
        showPictureMenu();
        break;
    case VoiceMenu:
        //语音菜单
        showVoiceMenu();
        break;
    case TxtMenu:
        //文字菜单
        showTxtMenu();
        break;
    default:
        break;
    }
}

/**
 * @brief 文字菜单弹出前设置
 */
void RichTextEdit::showTxtMenu()
{
    ActionManager::Instance()->resetCtxMenu(ActionManager::MenuType::TxtCtxMenu, false); //重置菜单选项
    bool isAlSrvAvailabel = OpsStateInterface::instance()->isAiSrvExist(); //获取语音服务是否可用
    bool TTSisWorking = VTextSpeechAndTrManager::isTextToSpeechInWorking(); //获取语音服务是否正在朗读
    //设置语音服务选项状态
    if (isAlSrvAvailabel) {
        if (TTSisWorking) {
            ActionManager::Instance()->visibleAction(ActionManager::TxtStopreading, true);
            ActionManager::Instance()->visibleAction(ActionManager::TxtSpeech, false);
            ActionManager::Instance()->enableAction(ActionManager::TxtStopreading, true);
        } else {
            ActionManager::Instance()->visibleAction(ActionManager::TxtStopreading, false);
            ActionManager::Instance()->visibleAction(ActionManager::TxtSpeech, true);
        }
    }
    //获取web端编辑标志
    QWebEngineContextMenuData::EditFlags flags = m_webView->page()->contextMenuData().editFlags();
    //设置普通菜单项状态
    //可全选
    if (flags.testFlag(QWebEngineContextMenuData::CanSelectAll)) {
        ActionManager::Instance()->enableAction(ActionManager::TxtSelectAll, true);
    }
    //可复制
    if (flags.testFlag(QWebEngineContextMenuData::CanCopy)) {
        ActionManager::Instance()->enableAction(ActionManager::TxtCopy, true);
        if (isAlSrvAvailabel) {
            if (VTextSpeechAndTrManager::getTransEnable()) {
                ActionManager::Instance()->enableAction(ActionManager::TxtTranslate, true);
            }
            if (!TTSisWorking && VTextSpeechAndTrManager::getTextToSpeechEnable()) {
                ActionManager::Instance()->enableAction(ActionManager::TxtSpeech, true);
            }
        }
    }
    //可剪切
    if (flags.testFlag(QWebEngineContextMenuData::CanCut)) {
        ActionManager::Instance()->enableAction(ActionManager::TxtCut, true);
    }
    //可删除
    if (flags.testFlag(QWebEngineContextMenuData::CanDelete)) {
        ActionManager::Instance()->enableAction(ActionManager::TxtDelete, true);
    }
    //可粘贴
    if (flags.testFlag(QWebEngineContextMenuData::CanPaste)) {
        ActionManager::Instance()->enableAction(ActionManager::TxtPaste, true);
        if (!TTSisWorking && VTextSpeechAndTrManager::getSpeechToTextEnable()) {
            ActionManager::Instance()->enableAction(ActionManager::TxtDictation, true);
        }
    }
    m_txtRightMenu->popup(mapToGlobal(m_menuPoint));
}

/**
 * @brief 图片菜单显示
 */
void RichTextEdit::showPictureMenu()
{
    m_pictureRightMenu->popup(mapToGlobal(m_menuPoint));
}

/**
 * @brief 语音菜单显示
 */
void RichTextEdit::showVoiceMenu()
{
    ActionManager::Instance()->visibleAction(ActionManager::VoiceToText, !OpsStateInterface::instance()->isVoice2Text());
    m_voiceRightMenu->popup(mapToGlobal(m_menuPoint));
}

/**
 * @brief 右键菜单项点击响应
 * @param action
 */
void RichTextEdit::onMenuActionClicked(QAction *action)
{
    ActionManager::ActionKind kind = ActionManager::Instance()->getActionKind(action);
    switch (kind) {
    case ActionManager::VoiceAsSave:
        break;
    case ActionManager::VoiceToText:
        break;
    case ActionManager::VoiceDelete:
    case ActionManager::PictureDelete:
    case ActionManager::TxtDelete:
        //直接调用web端的退格事件
        m_webView->page()->triggerAction(QWebEnginePage::Back);
        break;
    case ActionManager::VoiceSelectAll:
    case ActionManager::PictureSelectAll:
    case ActionManager::TxtSelectAll:
        //直接调用web端的全选事件
        m_webView->page()->triggerAction(QWebEnginePage::SelectAll);
        break;
    case ActionManager::VoiceCopy:
    case ActionManager::PictureCopy:
    case ActionManager::TxtCopy:
        //直接调用web端的复制事件
        m_webView->page()->triggerAction(QWebEnginePage::Copy);
        break;
    case ActionManager::VoiceCut:
    case ActionManager::PictureCut:
    case ActionManager::TxtCut:
        //直接调用web端的剪切事件
        m_webView->page()->triggerAction(QWebEnginePage::Cut);
        break;
    case ActionManager::VoicePaste:
    case ActionManager::PicturePaste:
    case ActionManager::TxtPaste:
        //粘贴事件，从剪贴板获取数据
        getImagePathsByClipboard();
        break;
    case ActionManager::PictureView:
        break;
    case ActionManager::PictureSaveAs:
        break;
    case ActionManager::TxtSpeech:
        VTextSpeechAndTrManager::onTextToSpeech();
        break;
    case ActionManager::TxtStopreading:
        VTextSpeechAndTrManager::onStopTextToSpeech();
        break;
    case ActionManager::TxtDictation:
        VTextSpeechAndTrManager::onSpeechToText();
        break;
    case ActionManager::TxtTranslate:
        VTextSpeechAndTrManager::onTextTranslate();
        break;
    default:
        break;
    }
}

bool RichTextEdit::findText(const QString &searchKey)
{
    bool ret = false;
    if (m_webView) {
        //同步获取搜索结果
        QEventLoop synLoop;
        m_webView->findText(searchKey, QWebEnginePage::FindFlags(), [&](const bool &result) {
            ret = result;
            synLoop.quit();
        });
        synLoop.exec();
    }
    return ret;
}

void RichTextEdit::unboundCurrentNoteData()
{
    //停止更新定时器
    m_updateTimer->stop();
    //绑定数据设置为空
    m_noteData = nullptr;
}
