/*
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

#ifndef NO_SYSTEMD
    m_com(new SystemdCommunicator(this)),
#endif

    m_loader(new Loader(this)),
    m_configValid(false),
    m_pwmFanModel(new PwmFanModel(this)),
    m_tempModel(new TempModel(this)),
    m_profileModel(new QStringListModel(this))
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

GUIBase::~GUIBase()
{
    Config::instance()->save();
}

void GUIBase::load()
{
    Config::instance()->load();
    m_configValid = m_loader->load(configUrl());

    auto profileNames = Config::instance()->findItem(QStringLiteral("ProfileNames"))->property().toStringList();
    m_profileModel->setStringList(profileNames);

    Config::instance()->setCurrentGroup(QStringLiteral("preferences"));
    int currentProfile = Config::instance()->findItem(QStringLiteral("CurrentProfile"))->property().toInt();
    emit profileChanged(currentProfile);

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
    Config::instance()->setCurrentGroup(QStringLiteral("preferences"));
    return Config::instance()->findItem(QStringLiteral("MaxTemp"))->property().toReal();
}

qreal GUIBase::minTemp() const
{
    Config::instance()->setCurrentGroup(QStringLiteral("preferences"));
    return Config::instance()->findItem(QStringLiteral("MinTemp"))->property().toReal();
}

QString GUIBase::serviceName() const
{
    Config::instance()->setCurrentGroup(QStringLiteral("preferences"));
    return Config::instance()->findItem(QStringLiteral("ServiceName"))->property().toString();
}

QUrl GUIBase::configUrl() const
{
    Config::instance()->setCurrentGroup(QStringLiteral("preferences"));
    return QUrl(Config::instance()->findItem(QStringLiteral("ConfigUrl"))->property().toString());
}

bool GUIBase::showTray() const
{
    Config::instance()->setCurrentGroup(QStringLiteral("preferences"));
    return Config::instance()->findItem(QStringLiteral("ShowTray"))->property().toBool();
}

bool GUIBase::startMinimized() const
{
    Config::instance()->setCurrentGroup(QStringLiteral("preferences"));
    return Config::instance()->findItem(QStringLiteral("StartMinimized"))->property().toBool();
}

void GUIBase::setMaxTemp(qreal temp)
{
    if (temp != maxTemp())
    {
        Config::instance()->setCurrentGroup(QStringLiteral("preferences"));
        Config::instance()->findItem(QStringLiteral("MaxTemp"))->setProperty(temp);
        emit maxTempChanged();
    }
}

void GUIBase::setMinTemp(qreal temp)
{
    if (temp != minTemp())
    {
        Config::instance()->setCurrentGroup(QStringLiteral("preferences"));
        Config::instance()->findItem(QStringLiteral("MinTemp"))->setProperty(temp);
        emit minTempChanged();
    }
}

void GUIBase::setServiceName(const QString& name)
{
    if (name != serviceName())
    {
        Config::instance()->setCurrentGroup(QStringLiteral("preferences"));
        Config::instance()->findItem(QStringLiteral("ServiceName"))->setProperty(name);

#ifndef NO_SYSTEMD
        m_com->setServiceName(name);
#endif

        emit serviceNameChanged();
    }
}

void GUIBase::setConfigUrl(const QUrl &url)
{
    if (url != configUrl())
    {
        m_configValid = m_loader->load(url);

        Config::instance()->setCurrentGroup(QStringLiteral("preferences"));
        Config::instance()->findItem(QStringLiteral("ConfigUrl"))->setProperty(url.toString());
        emit configUrlChanged();
    }
}

void GUIBase::setShowTray(bool show)
{
    if (showTray() == show)
        return;

    Config::instance()->setCurrentGroup(QStringLiteral("preferences"));
    Config::instance()->findItem(QStringLiteral("ShowTray"))->setProperty(show);
    emit showTrayChanged();
}

void GUIBase::setStartMinimized(bool sm)
{
    if (startMinimized() == sm)
        return;

    Config::instance()->setCurrentGroup(QStringLiteral("preferences"));
    Config::instance()->findItem(QStringLiteral("StartMinimized"))->setProperty(sm);
    emit startMinimizedChanged();
}

bool GUIBase::needsApply() const
{
#ifndef NO_SYSTEMD
    return m_loader->needsSave() || m_com->needsApply();
#else
    return m_loader->needsSave();
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

#ifndef NO_SYSTEMD
    m_com->apply(configChanged);
#endif

    emit needsApplyChanged();
}

void GUIBase::reset()
{
    qInfo() << i18n("Resetting changes");

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

void GUIBase::applyProfile(const QString& profile)
{
    if (!Config::instance()->findItem(QStringLiteral("ProfileNames"))->property().toStringList().contains(profile))
    {
        handleError(i18n("Unable to apply unknown profile: %1", profile));
        return;
    }

    int index = Config::instance()->findItem(QStringLiteral("ProfileNames"))->property().toStringList().indexOf(profile);

    applyProfile(index);
}

void GUIBase::applyProfile(int index)
{
    auto profileNames = Config::instance()->findItem(QStringLiteral("ProfileNames"))->property().toStringList();

    if (index < 0 || index >= profileNames.size())
        return;

    auto newConfig = Config::instance()->findItem(QStringLiteral("Profiles"))->property().toStringList().value(index);

    if (newConfig.isEmpty())
    {
        handleError(i18n("Unable to read data for profile: %1", index));
        profileNames.removeAt(index);
        Config::instance()->findItem(QStringLiteral("ProfileNames"))->setProperty(profileNames);
        return;
    }

    if (m_loader->config() == newConfig)
        return;

    m_loader->load(newConfig);

    Config::instance()->findItem(QStringLiteral("CurrentProfile"))->setProperty(index);
    emit profileChanged(index);
}

void GUIBase::saveProfile(const QString& profile, bool updateModel)
{
    auto profileNames = Config::instance()->findItem(QStringLiteral("ProfileNames"))->property().toStringList();
    int index = profileNames.indexOf(profile);

    if (index < 0)
    {
        index = profileNames.size();

        auto profileNames = Config::instance()->findItem(QStringLiteral("ProfileNames"))->property().toStringList();
        Config::instance()->findItem(QStringLiteral("ProfileNames"))->setProperty(profileNames << profile);

        if (updateModel)
            m_profileModel->insertRow(index);
    }

    auto profiles = Config::instance()->findItem(QStringLiteral("Profiles"))->property().toStringList();
    profiles.insert(index, m_loader->config());
    Config::instance()->findItem(QStringLiteral("Profiles"))->setProperty(profiles);

    if (updateModel)
        m_profileModel->setData(m_profileModel->index(index, 0), profile);
}

void GUIBase::deleteProfile(const QString& profile, bool updateModel)
{
    int index = Config::instance()->findItem(QStringLiteral("ProfileNames"))->property().toStringList().indexOf(profile);

    deleteProfile(index, updateModel);
}

void GUIBase::deleteProfile(int index, bool updateModel)
{
    auto profileNames = Config::instance()->findItem(QStringLiteral("ProfileNames"))->property().toStringList();

    if (index < 0 || index >= profileNames.size())
        return;

    profileNames.removeAt(index);
    Config::instance()->findItem(QStringLiteral("ProfileNames"))->setProperty(profileNames);
    auto profiles = Config::instance()->findItem(QStringLiteral("Profiles"))->property().toStringList();
    profileNames.removeAt(index);
    Config::instance()->findItem(QStringLiteral("Profiles"))->setProperty(profiles);

    if (updateModel)
        m_profileModel->removeRow(index);
}

}
