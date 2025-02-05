/*  This file is part of the KDE project
 *    SPDX-FileCopyrightText: 2023 Quang Ngô <ngoquang2708@gmail.com>
 *
 *    SPDX-License-Identifier: LGPL-2.0-only
 *
 */

#include "ddcutildisplay.h"

#define BRIGHTNESS_VCP_FEATURE_CODE 0x10

#ifdef WITH_DDCUTIL
DDCutilDisplay::DDCutilDisplay(DDCA_Display_Info displayInfo, DDCA_Display_Handle displayHandle)
    : m_displayInfo(displayInfo)
    , m_displayHandle(displayHandle)
    , m_brightness(-1)
    , m_maxBrightness(-1)
    , m_supportsBrightness(false)
{
    Q_ASSERT(displayHandle);

    DDCA_Non_Table_Vcp_Value value;

    if (ddca_get_non_table_vcp_value(m_displayHandle, BRIGHTNESS_VCP_FEATURE_CODE, &value) == DDCRC_OK) {
        m_brightness = value.sh << 8 | value.sl;
        m_maxBrightness = value.mh << 8 | value.ml;
        m_supportsBrightness = true;
    }
}
#endif

DDCutilDisplay::~DDCutilDisplay()
{
#ifdef WITH_DDCUTIL
    ddca_close_display(m_displayHandle);
#endif
}

QString DDCutilDisplay::label() const
{
#ifdef WITH_DDCUTIL
    return m_displayInfo.model_name;
#else
    return QString();
#endif
}

int DDCutilDisplay::brightness()
{
    QReadLocker lock(&m_lock);

    return m_brightness;
}

int DDCutilDisplay::maxBrightness()
{
    QReadLocker lock(&m_lock);

    return m_maxBrightness;
}

void DDCutilDisplay::setBrightness(int value)
{
    QWriteLocker lock(&m_lock);

#ifdef WITH_DDCUTIL
    uint8_t sh = value >> 8 & 0xff;
    uint8_t sl = value & 0xff;

    if (ddca_set_non_table_vcp_value(m_displayHandle, BRIGHTNESS_VCP_FEATURE_CODE, sh, sl) == DDCRC_OK) {
        m_brightness = value;
    }
#endif
}

bool DDCutilDisplay::supportsBrightness() const
{
    return m_supportsBrightness;
}

#include "moc_ddcutildisplay.cpp"
