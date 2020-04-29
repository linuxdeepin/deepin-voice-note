#ifndef UPGRADEVIEW_H
#define UPGRADEVIEW_H

#include <QShowEvent>
#include <QWidget>
#include <QSettings>

#include <DLabel>
#include <DWaterProgress>

DWIDGET_USE_NAMESPACE

class UpgradeView : public QWidget
{
    Q_OBJECT
public:
    explicit UpgradeView(QWidget *parent = nullptr);

    void startUpgrade();
signals:
    void upgradeDone();
public slots:
    void setProgress(int progress);
    void onDataReady();
    void onUpgradeFinish();
protected:
    void initConnections();
    void initAppSetting();
private:
    DWaterProgress *m_waterProgress {nullptr};
    DLabel         *m_tooltipTextLabel {nullptr};

    //App setting
    QSharedPointer<QSettings> m_qspSetting {nullptr};
};

#endif // UPGRADEVIEW_H
