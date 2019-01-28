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
import QtQuick.Controls 1.4
import org.kde.kirigami 2.4 as Kirigami
import Fancontrol.Qml 1.0 as Fancontrol
import "units.js" as Units


Rectangle {
    id: root

    property Item background: parent
    readonly property real centerX: x + width / 2
    readonly property real centerY: y + height / 2
    readonly property point center: Qt.point(centerX, centerY)
    property int temp: 0
    property int pwm: 0
    property alias drag: pwmMouse.drag
    property int size: Kirigami.Units.smallSpacing * 2
    property string unit: Fancontrol.Base.unit
    property var locale: Qt.locale()

    signal positionChanged()

    width: size
    height: size
    radius: size / 2
    border.width: pwmMouse.containsMouse || drag.active ? 1 : 0

    onXChanged: positionChanged()
    onYChanged: positionChanged()

    Drag.dragType: Drag.Automatic

    MouseArea {
        id: pwmMouse

        anchors.fill: parent
        hoverEnabled: root.enabled ? true : false
        cursorShape: containsPress || drag.active ? Qt.DragMoveCursor : Qt.PointingHandCursor
        drag.target: root
        drag.axis: Drag.XAndYAxis
        drag.smoothed: false
        drag.minimumX: - root.width/2
        drag.maximumX: background.width - root.width/2
        drag.minimumY: - root.height/2
        drag.maximumY: background.height - root.height/2
    }

    Rectangle {
        id: tooltip
        x: parent.width
        y: - height
        width: Math.max(pwm.width, temp.width)
        height: pwm.height + temp.height
        radius: 4
        color: Qt.rgba(parent.color.r, parent.color.g, parent.color.b, 0.5)
        visible: root.enabled && (pwmMouse.containsMouse || drag.active)

        Column {
            Label {
                id: pwm
                font.pixelSize: root.size * 1.5
                text: Number(Math.round(root.pwm / 2.55)).toLocaleString(locale, 'f', 1) + i18n('%')
            }
            Label {
                id: temp
                font.pixelSize: root.size * 1.5
                text: Math.round(Units.fromCelsius(root.temp, unit)) + i18n(unit)
            }
        }
    }
}
