#include "splashview.h"

#include <QVBoxLayout>

#include <DLabel>
#include <DWaterProgress>

DWIDGET_USE_NAMESPACE

SplashView::SplashView(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(true);

    auto water = new DWaterProgress(this);
    water->setValue(50);
    water->setTextVisible(false);
    water->start();

    auto label = new DLabel(tr("Loading..."));
    label->setObjectName("LoadWidgetLabel");
    label->setForegroundRole(DPalette::TextTitle);

    auto vbox = new QVBoxLayout;
    setLayout(vbox);

    vbox->addStretch();
    vbox->addWidget(water, 0, Qt::AlignCenter);
    vbox->addSpacing(10);
    vbox->addWidget(label, 0, Qt::AlignCenter);
    vbox->addStretch();
}
