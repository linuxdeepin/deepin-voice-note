#include "upgradeview.h"

#include <QVBoxLayout>

#include <DApplication>
#include <DFontSizeManager>

UpgradeView::UpgradeView(QWidget *parent) : DWidget(parent)
{
    setAutoFillBackground(true);

    m_waterProgress = new DWaterProgress(this);
    m_waterProgress->start();
    m_waterProgress->setFixedSize(QSize(80,80));

    m_tooltipTextLabel = new DLabel(DApplication::translate("UpgradeView",
                                                            "Importing notes from the old version, please wait..."),
                                    this);
    m_tooltipTextLabel->setForegroundRole(DPalette::TextTitle);
    DFontSizeManager::instance()->bind(m_tooltipTextLabel, DFontSizeManager::T4);
    auto vbox = new QVBoxLayout;
    setLayout(vbox);
    vbox->addStretch();
    vbox->addWidget(m_waterProgress, 0, Qt::AlignCenter);
    vbox->addSpacing(10);
    vbox->addWidget(m_tooltipTextLabel, 0, Qt::AlignCenter);
    vbox->addStretch();
}

void UpgradeView::setProgress(int progress)
{
    m_waterProgress->setValue(progress);
}
