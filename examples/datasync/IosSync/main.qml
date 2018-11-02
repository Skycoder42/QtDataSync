import QtQuick 2.10
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import QtQuick.Window 2.10

Window {
	visible: true
	width: 640
	height: 480
	title: qsTr("Hello World")

	ColumnLayout {
		anchors.centerIn: parent

		CheckBox {
			id: cBox
			text: qsTr("Background fetch enabled")
		}
	}
}
