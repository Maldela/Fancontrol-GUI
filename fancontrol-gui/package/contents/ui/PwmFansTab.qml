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
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.4 as Kirigami
import Fancontrol.Qml 1.0 as Fancontrol


Item {
    property QtObject loader: Fancontrol.Base.loader
    property QtObject systemdCom: Fancontrol.Base.hasSystemdCommunicator() ? Fancontrol.Base.systemdCom : null
    property QtObject pwmFanModel: Fancontrol.Base.pwmFanModel
    property QtObject tempModel: Fancontrol.Base.tempModel
    property QtObject profileModel: Fancontrol.Base.profileModel
    property var pwmFans: pwmFanModel.fans

    id: root
    anchors.fill: parent
    anchors.margins: Kirigami.Units.smallSpacing

    RowLayout {
        id: profileRow

        anchors.top: parent.top
        width: parent.width
        height: childrenRect.height

        Label {
            text: i18n("Profile:")
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            renderType: Text.NativeRendering
        }
        ComboBox {
            id: profileComboBox

            property string saveText: editText.length > 0 ? editText : currentText

            editable: true
            model: profileModel
            textRole: "display"
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

            onActivated: Fancontrol.Base.applyProfile(index)

            Connections {
                target: Fancontrol.Base
                onProfileChanged: profileComboBox.currentIndex = profile
            }
        }
        Button {
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            action: saveProfileAction
        }
        Button {
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            action: deleteProfileAction
        }
    }

    RowLayout {
        id: fanRow

        anchors.top: profileRow.bottom
        width: parent.width
        height: childrenRect.height
        visible: pwmFans.length > 0

        Label {
            text: i18n("Fan:")
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            renderType: Text.NativeRendering
        }
        ComboBox {
            id: fanComboBox

            model: pwmFanModel
            textRole: "display"
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        }
        Button {
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            action: detectFansAction
        }
    }

    Loader {
        width: parent.width
        anchors.top: fanRow.bottom
        anchors.bottom: parent.bottom
        anchors.topMargin: parent.anchors.margins
        active: pwmFans.length > fanComboBox.currentIndex && fanComboBox.currentIndex >= 0

        sourceComponent: Fancontrol.FanItem {
            fan: pwmFans[fanComboBox.currentIndex]
            tempModel: root.tempModel
            systemdCom: root.systemdCom
        }
    }

    ColumnLayout {
        id: noFansInfo

        anchors.centerIn: parent
        spacing: Kirigami.Units.smallSpacing * 2
        visible: pwmFans.length === 0

        Label {
            Layout.alignment: Qt.AlignCenter
            text: i18n("There are no pwm capable fans in your system.")
            font.pointSize: 14
            font.bold: true
        }

        Button {
            Layout.alignment: Qt.AlignCenter
            action: detectFansAction
        }
    }

    Action {
        id: saveProfileAction

        text: i18n("Save profile")
        iconName: "document-save"
        onTriggered: Fancontrol.Base.saveProfile(profileComboBox.saveText)
    }
    Action {
        id: deleteProfileAction

        text: i18n("Delete profile")
        iconName: "edit-delete"
        onTriggered: Fancontrol.Base.deleteProfile(profileComboBox.currentIndex)
    }
    Action {
        id: detectFansAction

        text: loader.sensorsDetected ? i18n("Detect fans again") : i18n("Detect fans")
        iconName: "dialog-password"
        onTriggered: loader.detectSensors()
    }
}
