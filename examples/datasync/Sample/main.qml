import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13
import QtQuick.Window 2.13
import QtWebView 1.13

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

	Connections {
		target: auth
		onSignInRequested: stackView.push(webViewComponent, {
											  authUrl: authUrl
										  });
		onSignInSuccessful: stackView.pop()
		onSignInFailed: stackView.pop()
	}

	Component {
		id: webViewComponent

		Page {
			id: webViewPage

			property alias authUrl: webView.url

			header: ToolBar {
				RowLayout {
					anchors.fill: parent

					Label {
						text: webView.title ? webView.title : qsTr("Loading…")
						leftPadding: 8
						font.pointSize: webViewPage.font.pointSize * 1.5
						font.bold: true
						elide: Label.ElideRight

						verticalAlignment: Qt.AlignVCenter
						Layout.fillWidth: true
					}

					ToolButton {
						display: ToolButton.IconOnly
						icon.name: webView.loading ? "gtk-cancel" : "view-refresh"
						icon.source: ""
						text: qsTr("Close")

						onClicked: {
							if (webView.loading)
								webView.stop();
							else
								webView.reload();
						}
					}
				}
			}

			WebView {
				id: webView
				anchors.fill: parent
				onLoadingChanged: {
					if (loadRequest.errorString)
						console.error(loadRequest.errorString);
				}

				ProgressBar {
					anchors.left: parent.left
					anchors.right: parent.right
					anchors.top: parent.top
					visible: webView.loading
					from: 0
					to: 100
					value: webView.loadProgress
				}
			}
		}
	}
}
