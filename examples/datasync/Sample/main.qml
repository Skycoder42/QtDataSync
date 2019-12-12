import QtQuick 2.13
import QtQuick.Window 2.13
import QtWebView 1.13

Window {
	visible: true
	width: 640
	height: 480
	title: qsTr("Hello World")

	WebView {
		id: webView
		anchors.fill: parent
		onLoadingChanged: {
			if (loadRequest.errorString)
				console.error(loadRequest.errorString);
		}

		Connections {
			target: auth
			onSignInRequested: webView.url = authUrl
		}
	}
}
