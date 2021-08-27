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
#include "webrichtexteditor.h"
#include "common/jscontent.h"
#include "common/actionmanager.h"
#include "common/vnoteitem.h"
#include "common/metadataparser.h"
#include "common/vtextspeechandtrmanager.h"
#include "dialog/imageviewerdialog.h"

#include "db/vnoteitemoper.h"

#include <DFileDialog>
#include <DGuiApplicationHelper>

#include <QEventLoop>
#include <QClipboard>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QWebEngineContextMenuData>
#include <QApplication>
#include <QStandardPaths>

static const char webPage[] = WEB_PATH "/index.html";

WebRichTextEditor::WebRichTextEditor(QWidget *parent)
    : QWebEngineView(parent)
{
    initWebView();
    initRightMenu();
    initUpdateTimer();
}

WebRichTextEditor::~WebRichTextEditor()
{
    if (imgView != nullptr) {
        delete imgView;
    }
}

void WebRichTextEditor::initWebView()
{
    QWebChannel *channel = new QWebChannel(this);
    JsContent *content = JsContent::instance();
    channel->registerObject("webobj", content);
    page()->setWebChannel(channel);
    QFileInfo info(webPage);
    load(QUrl::fromLocalFile(info.absoluteFilePath()));
    this->installEventFilter(this);

    connect(content, &JsContent::textChange, this, &WebRichTextEditor::onTextChange);
    connect(content, &JsContent::setDataFinsh, this, &WebRichTextEditor::onSetDataFinsh);
    connect(content, &JsContent::popupMenu, this, &WebRichTextEditor::saveMenuParam);
    connect(content, &JsContent::textPaste, this, &WebRichTextEditor::onPaste);
    connect(content, &JsContent::loadFinsh, this, &WebRichTextEditor::onThemeChanged);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::paletteTypeChanged,
            this, &WebRichTextEditor::onThemeChanged);
}

void WebRichTextEditor::initRightMenu()
{
    m_pictureRightMenu = ActionManager::Instance()->pictureContextMenu();
    m_voiceRightMenu = ActionManager::Instance()->voiceContextMenu();
    m_txtRightMenu = ActionManager::Instance()->txtContextMenu();
    connect(m_pictureRightMenu, &DMenu::triggered,
            this, &WebRichTextEditor::onMenuActionClicked);
    connect(m_voiceRightMenu, &DMenu::triggered,
            this, &WebRichTextEditor::onMenuActionClicked);
    connect(m_txtRightMenu, &DMenu::triggered,
            this, &WebRichTextEditor::onMenuActionClicked);
}

void WebRichTextEditor::initUpdateTimer()
{
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(1000);
    connect(m_updateTimer, &QTimer::timeout, this, &WebRichTextEditor::updateNote);
}

void WebRichTextEditor::initData(VNoteItem *data, const QString &reg, bool fouse)
{
    if (fouse) {
        setFocus();
    }
    if (data == nullptr) {
        this->setVisible(false);
        //无数据时先保存之前数据
        updateNote();
        //解绑当前数据
        unboundCurrentNoteData();
        return;
    }
    this->setVisible(true);
    m_searchKey = reg;

    if (m_noteData != data) { //笔记切换时设置笔记内容
        m_updateTimer->stop();
        updateNote();
        m_noteData = data;
        if (data->htmlCode.isEmpty()) {
            emit JsContent::instance()->callJsInitData(data->metaDataRef().toString());
        } else {
            emit JsContent::instance()->callJsSetHtml(data->htmlCode);
        }
        m_updateTimer->start();
    } else { //笔记相同时执行搜索
        findText(reg);
    }
}

void WebRichTextEditor::insertVoiceItem(const QString &voicePath, qint64 voiceSize)
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
    //关闭应用时，需要同步插入语音并进行后台更新
    if (OpsStateInterface::instance()->isAppQuit()) {
        JsContent::instance()->callJsSynchronous(page(), QString("insertVoiceItem('%1')").arg(value.toString()));
        m_textChange = true;
        update();
        return;
    }
    emit JsContent::instance()->callJsInsertVoice(value.toString());
}

void WebRichTextEditor::updateNote()
{
    if (m_noteData) {
        if (m_textChange) {
            QVariant result = JsContent::instance()->callJsSynchronous(page(), QString("getHtml()"));
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

void WebRichTextEditor::searchText(const QString &searchKey)
{
    findText(searchKey, QWebEnginePage::FindFlags(), [=](const bool &result) {
        if (result == false) {
            emit currentSearchEmpty();
        }
    });
}

void WebRichTextEditor::unboundCurrentNoteData()
{
    //停止更新定时器
    m_updateTimer->stop();
    //绑定数据设置为空
    m_noteData = nullptr;
}

void WebRichTextEditor::onTextChange()
{
    m_textChange = true;
}

void WebRichTextEditor::saveMenuParam(int type, int x, int y, const QVariant &json)
{
    m_menuType = static_cast<Menu>(type);
    m_menuPoint.setX(x);
    m_menuPoint.setY(y);
    m_menuJson = json;
}

void WebRichTextEditor::onSetDataFinsh()
{
    //只有编辑区内容加载完成才能搜索
    if (!m_searchKey.isEmpty()) {
        findText(m_searchKey);
    }
}

void WebRichTextEditor::showTxtMenu()
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
    QWebEngineContextMenuData::EditFlags flags = page()->contextMenuData().editFlags();
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
void WebRichTextEditor::showPictureMenu()
{
    m_pictureRightMenu->popup(mapToGlobal(m_menuPoint));
}

/**
 * @brief 语音菜单显示
 */
void WebRichTextEditor::showVoiceMenu()
{
    ActionManager::Instance()->visibleAction(ActionManager::VoiceToText, !OpsStateInterface::instance()->isVoice2Text());
    m_voiceRightMenu->popup(mapToGlobal(m_menuPoint));
}

void WebRichTextEditor::onMenuActionClicked(QAction *action)
{
    ActionManager::ActionKind kind = ActionManager::Instance()->getActionKind(action);
    switch (kind) {
    case ActionManager::VoiceAsSave:
        break;
    case ActionManager::VoiceToText:
        emit asrStart(m_menuJson); //通知主窗口进行转写服务
        break;
    case ActionManager::VoiceDelete:
    case ActionManager::PictureDelete:
    case ActionManager::TxtDelete:
        deleteSelectText();
        break;
    case ActionManager::VoiceSelectAll:
    case ActionManager::PictureSelectAll:
    case ActionManager::TxtSelectAll:
        //直接调用web端的全选事件
        page()->triggerAction(QWebEnginePage::SelectAll);
        break;
    case ActionManager::VoiceCopy:
    case ActionManager::PictureCopy:
    case ActionManager::TxtCopy:
        //直接调用web端的复制事件
        page()->triggerAction(QWebEnginePage::Copy);
        break;
    case ActionManager::VoiceCut:
    case ActionManager::PictureCut:
    case ActionManager::TxtCut:
        //直接调用web端的剪切事件
        page()->triggerAction(QWebEnginePage::Cut);
        break;
    case ActionManager::VoicePaste:
    case ActionManager::PicturePaste:
    case ActionManager::TxtPaste:
        //粘贴事件，从剪贴板获取数据
        onPaste();
        break;
    case ActionManager::PictureView:
        //查看图片
        viewPicture(m_menuJson.toString());
        break;
    case ActionManager::PictureSaveAs:
        //另存图片
        savePictureAs();
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

/**
 * @brief WebRichTextEditor::savePictureAs
 * 另存图片
 */
void WebRichTextEditor::savePictureAs()
{
    QString originalPath = m_menuJson.toString(); //获取原图片路径
    QFileInfo fileInfo(originalPath);

    QString filter = "*." + fileInfo.suffix();
    QString dir = QString("%1/%2")
                      .arg(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation))
                      .arg(fileInfo.baseName());
    //获取需要保存的文件位置，默认路径为用户图片文件夹，默认文件名为原文件名
    QString newPath = DFileDialog::getSaveFileName(this, "", dir, filter);
    if (newPath.isEmpty()) {
        return;
    }
    QFile::copy(originalPath, newPath); //复制文件
}

/**
 * @brief WebRichTextEditor::viewPicture
 * 查看图片
 * @param filePath 图片路径
 */
void WebRichTextEditor::viewPicture(const QString &filePath)
{
    if (imgView == nullptr) {
        imgView = new ImageViewerDialog(this);
    }
    //加载图片并显示
    imgView->open(filePath);
}

void WebRichTextEditor::onPaste()
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
        JsContent::instance()->insertImages(paths);
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
            JsContent::instance()->insertImages(QStringList(dirPath + "/" + fileName));
        }
    } else {
        //无图片文件，直接调用web端的粘贴事件
        page()->triggerAction(QWebEnginePage::Paste);
    }
}

void WebRichTextEditor::contextMenuEvent(QContextMenuEvent *e)
{
    Q_UNUSED(e)
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

void WebRichTextEditor::dropEvent(QDropEvent *event)
{
    // 获取文件路径
    if (event->mimeData()->hasUrls()) {
        QStringList paths;
        //获取文件路径
        for (auto url : event->mimeData()->urls()) {
            paths.push_back(url.path());
        }
        JsContent::instance()->insertImages(paths); //向web端发送路径
    }
}

void WebRichTextEditor::deleteSelectText()
{
    QObjectList children = page()->view()->children();
    for (auto obj : children) {
        //内部渲染代理窗口为RenderWidgetHostViewQtDelegateWidget，发送delete按键，触发内部的delete按键
        if (qobject_cast<QWidget *>(obj)) { //widget类型即为渲染代理窗口
            QKeyEvent event(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
            QApplication::sendEvent(obj, &event);
        }
    }
}

void WebRichTextEditor::onThemeChanged()
{
    DGuiApplicationHelper::ColorType theme =
        DGuiApplicationHelper::instance()->themeType();
    emit JsContent::instance()->callJsSetTheme(theme);
}
