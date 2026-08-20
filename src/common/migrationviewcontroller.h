// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MIGRATIONVIEWCONTROLLER_H
#define MIGRATIONVIEWCONTROLLER_H

#include "importolddata/dbmigration/migrationorchestrator.h"  // ProgressSnapshot / MigrationState
#include "importolddata/dbmigration/migrationstate.h"

#include <QObject>
#include <QPointer>
#include <QString>

// 升级进度界面控制器（QML 单例）。
//
// 消费编排层信号（progressChanged/stageChanged/terminalInfo/aborted），
// 映射为 QML 只读属性供 UpgradeView 绑定；提供取消入口与终态放行入口。
// 仅消费信号，不跨线程调 progressSnapshot()（M1），不反向控制迁移内部逻辑。
//
// 生命周期：start() 由 main.cpp 在 DB 就绪后单行调用；编排器由 startIfNeeded() 返回，
// 控制器以 QPointer 持有，终态/aborted 后 disconnect + 清空（编排器由 deleteLater 回收）。
class MigrationViewController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool migrationActive READ migrationActive NOTIFY migrationActiveChanged)
    Q_PROPERTY(QString stage READ stage NOTIFY stageChanged)
    Q_PROPERTY(int processed READ processed NOTIFY processedChanged)
    Q_PROPERTY(int total READ total NOTIFY totalChanged)
    Q_PROPERTY(int success READ success NOTIFY successChanged)
    Q_PROPERTY(int fail READ fail NOTIFY failChanged)
    Q_PROPERTY(QString terminalState READ terminalState NOTIFY terminalStateChanged)
    Q_PROPERTY(QString backupPath READ backupPath NOTIFY backupPathChanged)
    Q_PROPERTY(QString reportPath READ reportPath NOTIFY reportPathChanged)
    Q_PROPERTY(bool cancelling READ cancelling NOTIFY cancellingChanged)

public:
    static MigrationViewController *instance();

    bool migrationActive() const { return m_migrationActive; }
    QString stage() const { return m_stage; }
    int processed() const { return m_processed; }
    int total() const { return m_total; }
    int success() const { return m_success; }
    int fail() const { return m_fail; }
    QString terminalState() const { return m_terminalState; }
    QString backupPath() const { return m_backupPath; }
    QString reportPath() const { return m_reportPath; }
    bool cancelling() const { return m_cancelling; }

    // 启动入口：注册跨线程 metatype（M2）、据状态机判定是否需迁移、拉起后台编排器。
    void start();

public slots:
    // 取消入口（QML 调用）：转发到编排器（原子置位，线程安全）并置 cancelling。
    Q_INVOKABLE void requestCancel();
    // 终态放行入口（QML "进入应用" 按钮）：收起终态视图。
    Q_INVOKABLE void enterApp();

    // 编排器信号→属性映射槽（由 startIfNeeded 以 Qt::QueuedConnection 连接）。
    void onProgressChanged(const MigrationOrchestrator::ProgressSnapshot &snapshot);
    void onStageChanged(MigrationState stage);
    void onTerminalInfo(MigrationState finalState, const QString &backupPath, const QString &reportPath);
    void onAborted();

signals:
    void migrationActiveChanged();
    void stageChanged();
    void processedChanged();
    void totalChanged();
    void successChanged();
    void failChanged();
    void terminalStateChanged();
    void backupPathChanged();
    void reportPathChanged();
    void cancellingChanged();

private:
    explicit MigrationViewController(QObject *parent = nullptr);

    void setMigrationActive(bool active);
    void setStage(const QString &stage);
    void setProcessed(int value);
    void setTotal(int value);
    void setSuccess(int value);
    void setFail(int value);
    void setTerminalState(const QString &state);
    void setBackupPath(const QString &path);
    void setReportPath(const QString &path);
    void setCancelling(bool value);

    // 终态/aborted 后断开编排器信号并清空 QPointer（编排器由 deleteLater 回收）。
    void releaseOrchestrator();

    bool m_migrationActive = false;
    QString m_stage;
    int m_processed = 0;
    int m_total = 0;
    int m_success = 0;
    int m_fail = 0;
    QString m_terminalState;
    QString m_backupPath;
    QString m_reportPath;
    bool m_cancelling = false;

    QPointer<MigrationOrchestrator> m_orchestrator;
};

#endif // MIGRATIONVIEWCONTROLLER_H
