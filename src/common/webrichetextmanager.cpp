#include "webrichetextmanager.h"
#include "jscontent.h"
#include "db/vnoteitemoper.h"

#include <QTimer>
#include <QEventLoop>

WebRichTextManager::WebRichTextManager(QObject *parent)
: QObject(parent)
{

}

void WebRichTextManager::initData(VNoteItem *data, const QString reg, bool focus)
{
    //重置鼠标点击位置
    m_mouseClickPos = QPoint(-1, -1);
    m_setFocus = focus;
    //富文本设置异步操作，解决笔记列表不实时刷新
    QTimer::singleShot(50, this, [ = ] {
        setData(data, reg);
    });
}

void WebRichTextManager::setData(VNoteItem *data, const QString reg)
{
    //设置富文本内容
    if (data->htmlCode.isEmpty()) {
        emit JsContent::instance()->callJsInitData(data->metaDataRef().toString());
    } else {
        emit JsContent::instance()->callJsSetHtml(data->htmlCode);
    }
}

void WebRichTextManager::updateNote()
{
    // if (m_noteData) {
    //     if (m_textChange) {
    //         QVariant result = JsContent::instance()->callJsSynchronous(page(), QString("getHtml()"));
    //         if (result.isValid()) {
    //             m_noteData->htmlCode = result.toString();
    //             VNoteItemOper noteOps(m_noteData);
    //             if (!noteOps.updateNote()) {
    //                 qInfo() << "Save note error";
    //             }
    //         }
    //         m_textChange = false;
    //     }
    // }
}

/**
 * @brief onLoadFinsh
 * 与web端通信建立完成
 */
void WebRichTextManager::onLoadFinsh()
{
    //再次设置笔记内容
    if (m_noteData && !m_loadFinshSign) {
        if (m_noteData->htmlCode.isEmpty()) {
            emit JsContent::instance()->callJsInitData(m_noteData->metaDataRef().toString());
        } else {
            emit JsContent::instance()->callJsSetHtml(m_noteData->htmlCode);
        }
    }
    m_loadFinshSign = true;
}

void WebRichTextManager::clearJSContent()
{
    // if (this->isVisible()) {
        connect(JsContent::instance(), &JsContent::setDataFinsh, this, &WebRichTextManager::onSetDataFinsh);
        emit JsContent::instance()->callJsSetHtml("");

        // 开启100ms事件循环，保证js页面内容被刷新
        QEventLoop eveLoop;
        QTimer::singleShot(100, &eveLoop, SLOT(quit()));
        eveLoop.exec();
        disconnect(JsContent::instance(), &JsContent::setDataFinsh, this, &WebRichTextManager::onSetDataFinsh);
    // }
}

void WebRichTextManager::onSetDataFinsh()
{

}