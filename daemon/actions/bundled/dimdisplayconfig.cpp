/*
 *   SPDX-FileCopyrightText: 2010 Dario Freddi <drf@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dimdisplayconfig.h"

#include <PowerDevilProfileSettings.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>

#include <KConfig>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

K_PLUGIN_CLASS(PowerDevil::BundledActions::DimDisplayConfig)

namespace PowerDevil::BundledActions
{
DimDisplayConfig::DimDisplayConfig(QObject *parent)
    : ActionConfig(parent)
{
}

void DimDisplayConfig::save()
{
    profileSettings()->setDimDisplayIdleTimeoutSec(m_spinBox->value() * 60);
}

void DimDisplayConfig::load()
{
    m_spinBox->setValue(profileSettings()->dimDisplayIdleTimeoutSec() / 60);
}

bool DimDisplayConfig::enabledInProfileSettings() const
{
    return profileSettings()->dimDisplayWhenIdle();
}

void DimDisplayConfig::setEnabledInProfileSettings(bool enabled)
{
    profileSettings()->setDimDisplayWhenIdle(enabled);
}

QList<QPair<QString, QWidget *>> DimDisplayConfig::buildUi()
{
    m_spinBox = new QSpinBox;
    m_spinBox->setMaximumWidth(150);
    m_spinBox->setMinimum(1);
    m_spinBox->setMaximum(360);
    m_spinBox->setSuffix(i18n(" min"));

    QWidget *tempWidget = new QWidget;
    QHBoxLayout *hlay = new QHBoxLayout;
    tempWidget->setLayout(hlay);

    hlay->addWidget(m_spinBox);
    hlay->addStretch();

    QList<QPair<QString, QWidget *>> retlist;
    retlist.append(qMakePair(i18n("After"), tempWidget));

    connect(m_spinBox, SIGNAL(valueChanged(int)), this, SLOT(setChanged()));

    return retlist;
}

}

#include "dimdisplayconfig.moc"

#include "moc_dimdisplayconfig.cpp"
