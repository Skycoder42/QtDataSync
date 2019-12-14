import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Window 2.13

ApplicationWindow {
	visible: true
	width: 640
	height: 480
	title: qsTr("Hello World")

	StackView {
		id: stackView
		anchors.fill: parent

		initialItem: Item {
			Label {
				anchors.centerIn: parent
				text: qsTr("Nothing to see here…")
			}
		}
	}

	property bool authDone: false
	Connections {
		target: auth
		onSignInRequested: stackView.push(authComponent, {
											  authUrl: authUrl
										  });
		onSignInSuccessful: {
			authDone = true;
			stackView.pop();
		}

		onSignInFailed:  {
			authDone = true;
			stackView.pop();
			errorDialog.showError(qsTr("Authentication failed"), errorMessage);
		}
	}

	Component {
		id: authComponent

		AuthDialog {
			onAbortSignIn: {
				if (!authDone)
					auth.abortSignIn();
			}

			StackView.onDeactivating: {
				if (!authDone)
					auth.abortSignIn();
			}
		}
	}

	Dialog {
		id: errorDialog
		title: qsTr("Error")
		standardButtons: Dialog.Ok
		modal: true
		anchors.centerIn: Overlay.overlay

		function showError(title, errorString) {
			errorDialog.title = title;
			errorLabel.text = errorString;
			open();
		}

		Label {
			id: errorLabel
			anchors.fill: parent
		}
	}
}
