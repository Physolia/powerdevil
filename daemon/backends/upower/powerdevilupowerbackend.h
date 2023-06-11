/*  This file is part of the KDE project
    Copyright (C) 2006 Kevin Ottens <ervin@kde.org>
    Copyright (C) 2008-2010 Dario Freddi <drf@kde.org>
    Copyright (C) 2010 Alejandro Fiestas <alex@eyeos.org>
    Copyright (C) 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#pragma once

#include <powerdevilbackendinterface.h>

#include <QDBusConnection>
#include <QDBusInterface>

#include <map>
#include <memory>

#include "upowerdevice.h"
#include "upower_interface.h"
#include "upower_kbdbacklight_interface.h"
#include "udevqt.h"

#include "powerdevilupowerbackend_export.h"

#define UPOWER_SERVICE "org.freedesktop.UPower"
#define UPOWER_PATH "/org/freedesktop/UPower"
#define UPOWER_IFACE "org.freedesktop.UPower"
#define UPOWER_IFACE_DEVICE "org.freedesktop.UPower.Device"

#define LOGIN1_SERVICE "org.freedesktop.login1"
#define CONSOLEKIT2_SERVICE "org.freedesktop.ConsoleKit"

class QPropertyAnimation;
class QTimer;
class DDCutilBrightness;
class POWERDEVILUPOWERBACKEND_EXPORT PowerDevilUPowerBackend : public PowerDevil::BackendInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(PowerDevilUPowerBackend)
    Q_PLUGIN_METADATA(IID "org.kde.powerdevil.upowerbackend");

public:
    explicit PowerDevilUPowerBackend(QObject* parent = nullptr);
    ~PowerDevilUPowerBackend() override;

    void init() override;

    int screenBrightness() const override;
    int screenBrightnessMax() const override;
    void setScreenBrightness(int value) override;
    bool screenBrightnessAvailable() const override;

    int keyboardBrightness() const override;
    int keyboardBrightnessMax() const override;
    void setKeyboardBrightness(int value) override;
    bool keyboardBrightnessAvailable() const override;

    int screenBrightnessKeyPressed(PowerDevil::BrightnessLogic::BrightnessKeyType type) override;
    int keyboardBrightnessKeyPressed(PowerDevil::BrightnessLogic::BrightnessKeyType type) override;
    KJob *suspend(PowerDevil::BackendInterface::SuspendMethod method) override;

Q_SIGNALS:
    void brightnessSupportQueried(bool available);

private:
    void enumerateDevices();
    void addDevice(const QString &);

private Q_SLOTS:
    void updateDeviceProps();
    void slotDeviceAdded(const QDBusObjectPath & path);
    void slotDeviceRemoved(const QDBusObjectPath & path);
    void slotLogin1PrepareForSleep(bool active);
    void slotScreenBrightnessChanged();
    void onDeviceChanged(const UdevQt::Device &device);
    void onKeyboardBrightnessChanged(int);

    void onPropertiesChanged(const QString &ifaceName, const QVariantMap &changedProps, const QStringList &invalidatedProps);

private:
    void animationValueChanged(const QVariant &value);
    void initWithBrightness(bool brightnessSupport);

    // upower devices
    std::map<QString, std::unique_ptr<UPowerDevice>> m_devices;
    std::unique_ptr<UPowerDevice> m_displayDevice = nullptr;

    // brightness
    int m_cachedScreenBrightness;
    int m_cachedKeyboardBrightness;
    bool m_screenBrightnessAvailable = false;

    DDCutilBrightness *m_ddcBrightnessControl = nullptr;

    OrgFreedesktopUPowerInterface *m_upowerInterface;
    OrgFreedesktopUPowerKbdBacklightInterface *m_kbdBacklight;
    int m_kbdMaxBrightness;
    int m_brightnessMax = 0;
    bool m_keyboardBrightnessAvailable = false;

    QPropertyAnimation *m_brightnessAnimation = nullptr;

    QTimer *m_brightnessAnimationTimer = nullptr;

    // login1 interface
    QPointer<QDBusInterface> m_login1Interface;

    // buttons
    bool m_lidIsPresent;
    bool m_lidIsClosed;
    bool m_onBattery;

    // property if brightness control is leds subsystem
    bool m_isLedBrightnessControl;

    //helper path
    QString m_syspath;
};
