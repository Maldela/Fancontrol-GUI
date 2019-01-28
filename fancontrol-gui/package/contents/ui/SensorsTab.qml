/*
 * Copyright (C) 2015  Malte Veerman <malte.veerman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */


import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.4 as Kirigami
import Fancontrol.Qml 1.0 as Fancontrol


RowLayout {
    id: root

    property int padding: 10
    property QtObject loader: Fancontrol.Base.loader

    anchors.fill: parent
    anchors.margins: Kirigami.Units.smallSpacing

    Repeater {
        model: loader.hwmons.length

        Rectangle {
            property QtObject hwmon: loader.hwmons[index]

            Layout.preferredWidth: root.width / loader.hwmons.length - root.spacing
            Layout.maximumWidth: 500
            Layout.fillHeight: true
            color: palette.light
            border.width: 1
            border.color: "black"
            radius: Kirigami.Units.smallSpacing
            clip: true

            Column {
                id: column
                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: hwmon.name
                    font.pointSize: 12
                    horizontalAlignment: Text.horizontalCenter
                }

                Repeater {
                    model: hwmon.fans.length

                    RowLayout {
                        width: parent.width

                        Label {
                            anchors.leftMargin: root.padding
                            Layout.maximumWidth: parent.width - rpmValue.width - root.padding*2
                            Layout.alignment: Qt.AlignLeft
                            clip: true
                            text: "Fan " + (index+1) + ":"
                        }
                        Label {
                            id: rpmValue
                            Layout.alignment: Qt.AlignRight
                            anchors.rightMargin: root.padding
                            text: hwmon.fans[index].rpm + " " + i18n("rpm")
                        }
                    }
                }
                Repeater {
                    model: hwmon.temps.length

                    RowLayout {
                        width: parent.width

                        Label {
                            anchors.leftMargin: root.padding
                            text: hwmon.temps[index].name + ": "
                            Layout.maximumWidth: parent.width - tempValue.width - root.padding*2
                            Layout.alignment: Qt.AlignLeft
                            clip: true
                        }
                        Label {
                            id: tempValue
                            Layout.alignment: Qt.AlignRight
                            anchors.rightMargin: root.padding
                            text: Units.fromCelsius(hwmon.temps[index].value, Fancontrol.Base.unit) + " " + i18n(Fancontrol.Base.unit)
                        }
                    }
                }
            }
        }
    }

    SystemPalette {
        id: palette
    }
}
