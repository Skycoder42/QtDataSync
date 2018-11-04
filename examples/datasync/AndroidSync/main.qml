import QtQuick 2.10
import QtQuick.Window 2.10
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import de.skycoder42.QtDataSync 4.2

Window {
	visible: true
	width: 640
	height: 480
	title: qsTr("Hello World")

	AndroidSyncControl {
		id: syncControl
	}

	ColumnLayout {
		anchors.centerIn: parent

		CheckBox {
			id: cBox
			checked: syncControl.enabled
			onCheckedChanged: syncControl.enabled = cBox.checked

			text: qsTr("Background sync active")
		}

		Button {
			text: qsTr("Sync now")
			onClicked: syncControl.triggerSyncNow()
		}
	}
}
