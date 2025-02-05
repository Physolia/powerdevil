/*
 *   SPDX-FileCopyrightText: 2008 Dario Freddi <drf@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "GeneralPage.h"

#include "ErrorOverlay.h"
#include "PowerButtonActionModel.h"

#include <PowerDevilGlobalSettings.h>
#include <powerdevilenums.h>
#include <powerdevilpowermanagement.h>

#include <Solid/Battery>
#include <Solid/Device>
#include <Solid/DeviceInterface>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QDBusServiceWatcher>

#include <KAboutData>
#include <KLocalizedString>
#include <KNotifyConfigWidget>
#include <KPluginFactory>
#include <KSharedConfig>

#include <KAuth/Action>
#include <KAuth/ExecuteJob>
#include <kauth_version.h>

K_PLUGIN_CLASS_WITH_JSON(GeneralPage, "kcm_powerdevilglobalconfig.json")

GeneralPage::GeneralPage(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
    , m_settings(
          new PowerDevil::GlobalSettings(PowerDevil::PowerManagement::instance()->canSuspend(), PowerDevil::PowerManagement::instance()->canHibernate(), this))
{
    setButtons(Apply | Help);

    setupUi(widget());

    fillUi();

    QDBusServiceWatcher *watcher = new QDBusServiceWatcher("org.kde.Solid.PowerManagement",
                                                           QDBusConnection::sessionBus(),
                                                           QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
                                                           this);

    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &GeneralPage::onServiceRegistered);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, &GeneralPage::onServiceUnregistered);

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.Solid.PowerManagement")) {
        onServiceRegistered("org.kde.Solid.PowerManagement");
    } else {
        onServiceUnregistered("org.kde.Solid.PowerManagement");
    }
}

GeneralPage::~GeneralPage()
{
}

void GeneralPage::fillUi()
{
    bool hasPowerSupplyBattery = false;
    bool hasPeripheralBattery = false;

    const auto devices = Solid::Device::listFromType(Solid::DeviceInterface::Battery, QString());
    for (const Solid::Device &device : devices) {
        const Solid::Battery *b = qobject_cast<const Solid::Battery *>(device.asDeviceInterface(Solid::DeviceInterface::Battery));
        if (b->isPowerSupply()) {
            hasPowerSupplyBattery = true;
        } else {
            hasPeripheralBattery = true;
        }
    }

    BatteryCriticalCombo->setModel(new PowerButtonActionModel(BatteryCriticalCombo,
                                                              PowerDevil::PowerManagement::instance(),
                                                              {
                                                                  PowerDevil::PowerButtonAction::NoAction,
                                                                  PowerDevil::PowerButtonAction::SuspendToRam,
                                                                  PowerDevil::PowerButtonAction::SuspendToDisk,
                                                                  PowerDevil::PowerButtonAction::Shutdown,
                                                              }));

    notificationsButton->setIcon(QIcon::fromTheme("preferences-desktop-notification"));

    // modified fields...

    connect(notificationsButton, &QAbstractButton::clicked, this, &GeneralPage::configureNotifications);

    connect(lowSpin, &QSpinBox::valueChanged, this, &KCModule::markAsChanged);
    connect(criticalSpin, &QSpinBox::valueChanged, this, &KCModule::markAsChanged);
    connect(lowPeripheralSpin, &QSpinBox::valueChanged, this, &KCModule::markAsChanged);

    connect(BatteryCriticalCombo, &QComboBox::currentIndexChanged, this, &KCModule::markAsChanged);

    connect(chargeStartThresholdSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &GeneralPage::markAsChanged);
    connect(chargeStopThresholdSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &GeneralPage::onChargeStopThresholdChanged);
    chargeStopThresholdMessage->hide();

    connect(pausePlayersCheckBox, &QCheckBox::stateChanged, this, &KCModule::markAsChanged);

    if (!hasPowerSupplyBattery) {
        BatteryCriticalLabel->hide();
        BatteryCriticalCombo->hide();
        lowLabel->hide();
        lowSpin->hide();
        criticalLabel->hide();
        criticalSpin->hide();
    }

    if (!hasPeripheralBattery) {
        lowPeripheralLabel->hide();
        lowPeripheralSpin->hide();
    }

    if (!hasPowerSupplyBattery && !hasPeripheralBattery) {
        batteryLevelsLabel->hide();
    }
}

void GeneralPage::load()
{
    lowSpin->setValue(m_settings->batteryLowLevel());
    criticalSpin->setValue(m_settings->batteryCriticalLevel());
    lowPeripheralSpin->setValue(m_settings->peripheralBatteryLowLevel());

    BatteryCriticalCombo->setCurrentIndex(BatteryCriticalCombo->findData(m_settings->batteryCriticalAction()));

    pausePlayersCheckBox->setChecked(m_settings->pausePlayersOnSuspend());

    KAuth::Action action(QStringLiteral("org.kde.powerdevil.chargethresholdhelper.getthreshold"));
    action.setHelperId(QStringLiteral("org.kde.powerdevil.chargethresholdhelper"));
    KAuth::ExecuteJob *job = action.execute();
    job->exec();

    if (!job->error()) {
        const auto data = job->data();
        m_chargeStartThreshold = data.value(QStringLiteral("chargeStartThreshold")).toInt();
        chargeStartThresholdSpin->setValue(m_chargeStartThreshold);
        m_chargeStopThreshold = data.value(QStringLiteral("chargeStopThreshold")).toInt();
        chargeStopThresholdSpin->setValue(m_chargeStopThreshold);

        setChargeThresholdSupported(true);
    } else {
        qDebug() << "org.kde.powerdevil.chargethresholdhelper.getthreshold failed" << job->errorText();
        setChargeThresholdSupported(false);
    }

    setNeedsSave(false);
}

void GeneralPage::configureNotifications()
{
    KNotifyConfigWidget::configure(widget(), "powerdevil");
}

void GeneralPage::save()
{
    m_settings->setBatteryLowLevel(lowSpin->value());
    m_settings->setBatteryCriticalLevel(criticalSpin->value());
    m_settings->setPeripheralBatteryLowLevel(lowPeripheralSpin->value());

    m_settings->setBatteryCriticalAction(BatteryCriticalCombo->itemData(BatteryCriticalCombo->currentIndex()).toUInt());

    m_settings->setPausePlayersOnSuspend(pausePlayersCheckBox->checkState() == Qt::Checked);

    m_settings->save();

    if ((m_chargeStartThreshold != -1 && chargeStartThresholdSpin->value() != m_chargeStartThreshold)
        || (m_chargeStopThreshold != -1 && chargeStopThresholdSpin->value() != m_chargeStopThreshold)) {
        KAuth::Action action(QStringLiteral("org.kde.powerdevil.chargethresholdhelper.setthreshold"));
        action.setHelperId(QStringLiteral("org.kde.powerdevil.chargethresholdhelper"));
        action.setArguments({
            {QStringLiteral("chargeStartThreshold"), m_chargeStartThreshold != -1 ? chargeStartThresholdSpin->value() : -1},
            {QStringLiteral("chargeStopThreshold"), m_chargeStopThreshold != -1 ? chargeStopThresholdSpin->value() : -1},
        });
        KAuth::ExecuteJob *job = action.execute();
        job->exec();
        if (!job->error()) {
            m_chargeStartThreshold = m_chargeStartThreshold != -1 ? chargeStartThresholdSpin->value() : -1;
            m_chargeStopThreshold = m_chargeStopThreshold != -1 ? chargeStopThresholdSpin->value() : -1;
        }
    }

    // Notify Daemon
    QDBusMessage call =
        QDBusMessage::createMethodCall("org.kde.Solid.PowerManagement", "/org/kde/Solid/PowerManagement", "org.kde.Solid.PowerManagement", "refreshStatus");

    // Perform call
    QDBusConnection::sessionBus().asyncCall(call);

    // And now we are set with no change
    setNeedsSave(false);
}

void GeneralPage::defaults()
{
    KCModule::defaults();
}

void GeneralPage::setChargeThresholdSupported(bool supported)
{
    batteryThresholdLabel->setVisible(supported);
    batteryThresholdExplanation->setVisible(supported);

    chargeStartThresholdLabel->setVisible(supported && m_chargeStartThreshold != -1);
    chargeStartThresholdSpin->setVisible(supported && m_chargeStartThreshold != -1);

    chargeStopThresholdLabel->setVisible(supported && m_chargeStopThreshold != -1);
    chargeStopThresholdSpin->setVisible(supported && m_chargeStopThreshold != -1);
}

void GeneralPage::onServiceRegistered(const QString &service)
{
    Q_UNUSED(service);

    if (m_errorOverlay) {
        m_errorOverlay->deleteLater();
        m_errorOverlay = nullptr;
    }
}

void GeneralPage::onServiceUnregistered(const QString &service)
{
    Q_UNUSED(service);

    if (m_errorOverlay) {
        m_errorOverlay->deleteLater();
    }

    m_errorOverlay = new ErrorOverlay(widget(), i18n("The Power Management Service appears not to be running."), widget());
}

void GeneralPage::onChargeStopThresholdChanged(int threshold)
{
    if (threshold > m_chargeStopThreshold) {
        // Only show message if there is actually a charging or fully charged battery
        const auto devices = Solid::Device::listFromType(Solid::DeviceInterface::Battery, QString());
        for (const Solid::Device &device : devices) {
            const Solid::Battery *b = qobject_cast<const Solid::Battery *>(device.asDeviceInterface(Solid::DeviceInterface::Battery));
            if (b->chargeState() == Solid::Battery::Charging || b->chargeState() == Solid::Battery::FullyCharged) {
                chargeStopThresholdMessage->animatedShow();
                break;
            }
        }
    } else if (chargeStopThresholdMessage->isVisible()) {
        chargeStopThresholdMessage->animatedHide();
    }

    markAsChanged();
}

#include "GeneralPage.moc"

#include "moc_GeneralPage.cpp"
