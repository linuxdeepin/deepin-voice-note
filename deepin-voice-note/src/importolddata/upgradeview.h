#ifndef UPGRADEVIEW_H
#define UPGRADEVIEW_H

#include <DWidget>
#include <DLabel>
#include <DWaterProgress>
DWIDGET_USE_NAMESPACE

class UpgradeView : public DWidget
{
    Q_OBJECT
public:
    explicit UpgradeView(QWidget *parent = nullptr);

signals:

public slots:
    void setProgress(int progress);
private:
    DWaterProgress *m_waterProgress {nullptr};
    DLabel         *m_tooltipTextLabel {nullptr};
};

#endif // UPGRADEVIEW_H
