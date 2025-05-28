#include "webrichetextmanager.h"
#include "jscontent.h"
#include "db/vnoteitemoper.h"
#include "metadataparser.h"

#include <QTimer>
#include <QEventLoop>

WebRichTextManager::WebRichTextManager(QObject *parent)
: QObject(parent)
{
    initUpdateTimer();
    initConnect();
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

void WebRichTextManager::initConnect()
{
    connect(JsContent::instance(), &JsContent::textChange, this, [=](){
        m_textChange = true;
        emit noteTextChanged();
    });

    connect(JsContent::instance(), &JsContent::scrollTopChange, this, [=](bool isTop){
        emit scrollChange(isTop);
    });
}

void WebRichTextManager::setData(VNoteItem *data, const QString reg)
{
    qDebug() << "Setting rich text data";
    m_updateTimer->stop();
    //设置富文本内容
    if (data->htmlCode.isEmpty()) {
        qDebug() << "Initializing with metadata";
        emit JsContent::instance()->callJsInitData(data->metaDataRef().toString());
    } else {
        qDebug() << "Setting HTML content";
        emit JsContent::instance()->callJsSetHtml(data->htmlCode);
    }
    m_updateTimer->start();
    emit updateSearch();
}

void WebRichTextManager::initUpdateTimer()
{
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(1000);
    connect(m_updateTimer, &QTimer::timeout, this, &WebRichTextManager::updateNote);
}

void WebRichTextManager::updateNote()
{
    if (m_textChange) {
        emit needUpdateNote();
    }
}

void WebRichTextManager::onUpdateNoteWithResult(VNoteItem *data, const QString &result)
{
    qDebug() << "Updating note with result";
    data->htmlCode = result;
    VNoteItemOper noteOps(data);
    if (!noteOps.updateNote()) {
        qWarning() << "Failed to save note";
    } else {
        qDebug() << "Note saved successfully";
    }
    m_textChange = false;
    emit finishedUpdateNote();
}

void WebRichTextManager::insertVoiceItem(const QString &voicePath, qint64 voiceSize)
{
    qDebug() << "Inserting voice item, path:" << voicePath << "size:" << voiceSize;
    VNVoiceBlock data;
    data.ptrVoice->voiceSize = voiceSize;
    data.ptrVoice->voicePath = voicePath;
    data.ptrVoice->createTime = QDateTime::currentDateTime();
    data.ptrVoice->voiceTitle = data.ptrVoice->createTime.toString("yyyyMMdd hh.mm.ss");

    MetaDataParser parse;
    QVariant value;
    parse.makeMetaData(&data, value);

    emit JsContent::instance()->callJsInsertVoice(value.toString());
    qDebug() << "Voice item inserted successfully";
}

/**
 * @brief onLoadFinsh
 * 与web端通信建立完成
 */
void WebRichTextManager::onLoadFinsh()
{
    qDebug() << "Web communication established";
    //再次设置笔记内容
    if (m_noteData && !m_loadFinshSign) {
        if (m_noteData->htmlCode.isEmpty()) {
            qDebug() << "Initializing with metadata after load";
            emit JsContent::instance()->callJsInitData(m_noteData->metaDataRef().toString());
        } else {
            qDebug() << "Setting HTML content after load";
            emit JsContent::instance()->callJsSetHtml(m_noteData->htmlCode);
        }
    }
    m_loadFinshSign = true;
}

void WebRichTextManager::clearJSContent()
{
    qDebug() << "Clearing JS content";
    connect(JsContent::instance(), &JsContent::setDataFinsh, this, &WebRichTextManager::onSetDataFinsh);
    emit JsContent::instance()->callJsSetHtml("");

    // 开启100ms事件循环，保证js页面内容被刷新
    QEventLoop eveLoop;
    QTimer::singleShot(100, &eveLoop, SLOT(quit()));
    eveLoop.exec();
    disconnect(JsContent::instance(), &JsContent::setDataFinsh, this, &WebRichTextManager::onSetDataFinsh);
    qDebug() << "JS content cleared";
}

void WebRichTextManager::onSetDataFinsh()
{
}
