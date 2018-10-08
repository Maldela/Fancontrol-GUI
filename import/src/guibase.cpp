/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2015  Malte Veerman <malte.veerman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include "guibase.h"

#include "config.h"
#include "hwmon.h"

#include <QtCore/QLocale>
#include <QtCore/QDebug>

#include <KI18n/KLocalizedString>


namespace Fancontrol
{

GUIBase::GUIBase(QObject *parent) : QObject(parent),
    m_config(Config::instance()),

#ifndef NO_SYSTEMD
    m_com(new SystemdCommunicator(this)),
#endif

    m_loader(new Loader(this)),
    m_configValid(false),
    m_configChanged(false),
    m_pwmFanModel(new PwmFanModel(this)),
    m_tempModel(new TempModel(this))
{
    connect(this, &GUIBase::unitChanged, m_tempModel, &TempModel::setUnit);
    connect(m_loader, &Loader::needsSaveChanged, this, &GUIBase::needsApplyChanged);

#ifndef NO_SYSTEMD
    connect(m_loader, &Loader::requestSetServiceActive, m_com, &SystemdCommunicator::setServiceActive);
    connect(m_com, &SystemdCommunicator::needsApplyChanged, this, &GUIBase::needsApplyChanged);
#endif

    const auto locale = QLocale::system();
    const auto system = locale.measurementSystem();
    m_unit = (system == QLocale::ImperialUSSystem) ? QStringLiteral("°F") : QStringLiteral("°C");
    emit unitChanged(m_unit);

    m_loader->parseHwmons();

    const auto hwmons = m_loader->hwmons();
    for (const auto &hwmon : hwmons)
    {
        m_pwmFanModel->addPwmFans(hwmon->pwmFans());
        m_tempModel->addTemps(hwmon->temps());
    }
}

void GUIBase::load()
{
    m_config->load();
    m_configValid = m_loader->load(configUrl());

#ifndef NO_SYSTEMD
    m_com->setServiceName(serviceName());
    m_com->reset();
#endif

    emit serviceNameChanged();
    emit minTempChanged();
    emit maxTempChanged();
    emit configUrlChanged();
}

qreal GUIBase::maxTemp() const
{
    return m_config->findItem(QStringLiteral("MaxTemp"))->property().toReal();
}

qreal GUIBase::minTemp() const
{
    return m_config->findItem(QStringLiteral("MinTemp"))->property().toReal();
}

QString GUIBase::serviceName() const
{
    return m_config->findItem(QStringLiteral("ServiceName"))->property().toString();
}

QUrl GUIBase::configUrl() const
{
    return QUrl(m_config->findItem(QStringLiteral("ConfigUrl"))->property().toString());
}

void GUIBase::setMaxTemp(qreal temp)
{
    if (temp != maxTemp())
    {
        m_config->findItem(QStringLiteral("MaxTemp"))->setProperty(temp);
        emit maxTempChanged();

        m_configChanged = true;
        emit needsApplyChanged();
    }
}

void GUIBase::setMinTemp(qreal temp)
{
    if (temp != minTemp())
    {
        m_config->findItem(QStringLiteral("MinTemp"))->setProperty(temp);
        emit minTempChanged();

        m_configChanged = true;
        emit needsApplyChanged();
    }
}

void GUIBase::setServiceName(const QString& name)
{
    if(name != serviceName())
    {
        m_config->findItem(QStringLiteral("ServiceName"))->setProperty(name);

#ifndef NO_SYSTEMD
        m_com->setServiceName(name);
#endif

        emit serviceNameChanged();

        m_configChanged = true;
        emit needsApplyChanged();
    }
}

void GUIBase::setConfigUrl(const QUrl &url)
{
    if (url != configUrl())
    {
        m_configValid = m_loader->load(url);

        m_config->findItem(QStringLiteral("ConfigUrl"))->setProperty(url.toString());
        emit configUrlChanged();

        m_configChanged = true;
        emit needsApplyChanged();
    }
}

bool GUIBase::needsApply() const
{
#ifndef NO_SYSTEMD
    return m_loader->needsSave() || m_configChanged || m_com->needsApply();
#else
    return m_loader->needsSave() || m_configChanged;
#endif
}

bool GUIBase::hasSystemdCommunicator() const
{
#ifndef NO_SYSTEMD
    return true;
#else
    return false;
#endif
}

void GUIBase::apply()
{
    qInfo() << i18n("Applying changes");

    bool configChanged = m_loader->save(configUrl());
    m_config->save();
    m_configChanged = false;

#ifndef NO_SYSTEMD
    m_com->apply(configChanged);
#endif

    emit needsApplyChanged();
}

void GUIBase::reset()
{
    qInfo() << i18n("Resetting changes");

    m_config->load();
    emit serviceNameChanged();
    emit minTempChanged();
    emit maxTempChanged();
    emit configUrlChanged();
    m_configChanged = false;

    if (m_loader->needsSave() || configUrl() != m_loader->configUrl())
        m_loader->load(configUrl());

#ifndef NO_SYSTEMD
    m_com->setServiceName(serviceName());
    m_com->reset();
#endif

    emit needsApplyChanged();
}

void GUIBase::handleError(const QString &error, bool critical)
{
    if (error.isEmpty() || error == m_error)
        return;

    m_error = error;
    emit errorChanged();

    if (critical)
    {
        qCritical() << error;
        emit criticalError();
    }
    else
        qWarning() << error;
}

void GUIBase::handleInfo(const QString &info)
{
    if (info.isEmpty())
        return;

    else
        qInfo() << info;
}


}
