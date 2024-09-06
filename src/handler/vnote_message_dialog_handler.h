// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VNOTEMESSAGEDIALOGHANDLER_H
#define VNOTEMESSAGEDIALOGHANDLER_H

#include <QObject>
#include <QVariant>

class VNoteMessageDialogHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MessageType messageType READ messageType WRITE setMessageType NOTIFY messageTypeChanged FINAL)
    Q_PROPERTY(QVariant messageData READ messageData WRITE setMessageData NOTIFY messageDataChanged FINAL)
    Q_PROPERTY(QString mainMessage READ mainMessage NOTIFY mainMessageChanged FINAL)
    Q_PROPERTY(QString detailMessage READ detailMessage NOTIFY detailMessageChanged FINAL)
    Q_PROPERTY(QString warnConfirm READ warnConfirm NOTIFY warnConfirmChanged FINAL)
    Q_PROPERTY(bool singleButton READ singleButton NOTIFY singleButtonChanged FINAL)

public:
    enum MessageType {
        None,
        DeleteNote,
        AbortRecord,
        DeleteFolder,
        AsrTimeLimit,
        AborteAsr,
        VolumeTooLow,
        CutNote,
        SaveFailed,        // 保存失败
        NoPermission,      // 无权限
        VoicePathNoAvail,  // 语音路径无效
    };
    Q_ENUM(MessageType)

    explicit VNoteMessageDialogHandler(QObject *parent = nullptr);

    MessageType messageType() const;
    void setMessageType(MessageType type);
    Q_SIGNAL void messageTypeChanged();

    QVariant messageData() const;
    void setMessageData(const QVariant &data);
    Q_SIGNAL void messageDataChanged();

    QString mainMessage() const;
    Q_SIGNAL void mainMessageChanged();

    QString detailMessage() const;
    Q_SIGNAL void detailMessageChanged();

    QString warnConfirm() const;
    Q_SIGNAL void warnConfirmChanged();

    bool singleButton() const;
    Q_SIGNAL void singleButtonChanged();

private:
    void initMessage();
    QString initMainMessage() const;
    QString initDetailMessage() const;
    QString initWarnConfirm() const;
    bool initSingleButton() const;

private:
    MessageType m_messageType { None };
    QVariant m_messageData;
    QString m_mainMessage;
    QString m_detailMessage;
    QString m_warnConfirm;
    bool m_singleButton { false };
};

#endif  // VNOTEMESSAGEDIALOGHANDLER_H
