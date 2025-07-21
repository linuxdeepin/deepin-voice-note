// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VNOTEA2TMANAGER_H
#define VNOTEA2TMANAGER_H

#include <QObject>
#include <QDBusInterface>
#include <QScopedPointer>
#include <QTimer>

struct asrMsg {
    QString code;
    QString descInfo;
    qint32 failType;
    qint32 status;
    QString text;
};

class VNoteA2TManager : public QObject
{
    Q_OBJECT
public:
    explicit VNoteA2TManager(QObject *parent = nullptr);
    /*
     * Reference AIService接口及错误码定义.doc for detail
     *
     *    @filePath     (Needed) Max 5 hours audio file
     *    @fileDuration (Needed) Audio file length in ms
     *    @srcLanguage  (Option) Only support cn, en.Default language=cn
     *    @targetLanguage (Option) Same as srcLanguage
     *
     * Eg: language=cn，targetLanguage=en
     * */
    void startAsr(QString filePath, qint64 fileDuration,
                  QString srcLanguage = "", QString targetLanguage = "");

    void stopAsr();

    //Reference AIService接口及错误码定义.doc for detail
    enum ErrorCode {
        Success = 0,
        UploadAudioFileFail, //音频上传失败
        AudioDecodeFail, //音频转码失败
        AudioRecognizeFail, //音频识别失败
        AudioExceedeLimit, //音频时长超限
        AudioVerifyFailed, //音频校验失败
        AudioMuteFile, //静音文件
        AudioOther, //其他
        NetworkError, //网络错误
        DontCareError,
        TimeoutError, //超时错误
    };

signals:
    //转文字失败
    void asrError(ErrorCode errorCode);
    //转文字成功
    void asrSuccess(const QString &text);
public slots:
    //转写过程中，信息处理
    void onNotify(const QString &msg);
    //超时处理
    void onTimeout();

protected:
    //转写信息解析
    void asrJsonParser(const QString &msg, asrMsg &asrData);
    //获取错误类型
    ErrorCode getErrorCode(const asrMsg &asrData);
    //初始化语音转写模块，初始化相关dbus连接
    int initSession();
    //检查并初始化新接口
    bool tryInitNewInterface();
    //检查并初始化旧接口
    bool tryInitOldInterface();

protected:
    //XunFei message code string
    const QString CODE_SUCCESS {"000000"};
    const QString CODE_NETWORK {"900003"};
    const QString CODE_TIMEOUT {"900004"}; // 超时错误码

    //XunFei state code
    enum State {
        XF_fail = -1,
        XF_Start = 0,
        XF_Process = 3,
        XF_finish = 4,
    };

    //XunFei failtype code
    enum FailType {
        XF_normal = 0,
        XF_upload,
        XF_decode,
        XF_recognize,
        XF_limit,
        XF_verify,
        XF_mute,
        XF_other = 99,
    };

    // 超时时间常量 (毫秒)
    static const int ASR_TIMEOUT_MS = 60000; // 1分钟超时

    // 新接口相关
    QScopedPointer<QDBusInterface> m_newAsrInterface;
    
    // 旧接口相关
    QScopedPointer<QDBusInterface> m_session;
    QScopedPointer<QDBusInterface> m_asrInterface;
    
    // 接口类型标识
    bool m_useNewInterface;
    
    // 超时定时器
    QTimer m_timeoutTimer;
};

#endif // VNOTEA2TMANAGER_H
