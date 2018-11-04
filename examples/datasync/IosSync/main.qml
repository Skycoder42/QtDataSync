import QtQuick 2.10
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import QtQuick.Window 2.10
import de.skycoder42.QtDataSync 4.2

Window {
	visible: true
	width: 640
	height: 480
	title: qsTr("Hello World")

	property IosSyncDelegate syncDelegate: IosSyncSingleton.delegate

	ColumnLayout {
		anchors.centerIn: parent

		CheckBox {
			id: cBox
			checked: syncDelegate.enabled
			onCheckedChanged: syncDelegate.enabled = cBox.checked

			text: qsTr("Background sync active")
		}
	}
}
