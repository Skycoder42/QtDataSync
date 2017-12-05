#include "remoteconnector_p.h"
#include "logger.h"
using namespace QtDataSync;

#define QTDATASYNC_LOG _logger

const QString RemoteConnector::keyRemoteUrl(QStringLiteral("remoteUrl"));
const QString RemoteConnector::keySharedSecret(QStringLiteral("sharedSecret"));
const QString RemoteConnector::keyHeaders(QStringLiteral("headers"));

RemoteConnector::RemoteConnector(const Defaults &defaults, QObject *parent) :
	QObject(parent),
	_defaults(defaults),
	_logger(_defaults.createLogger("connector", this)),
	_settings(_defaults.createSettings(this, QStringLiteral("connector"))),
	_socket(nullptr),
	_disconnecting(false)
{}

void RemoteConnector::reconnect()
{
	if(_socket) {
		if(_socket->state() == QAbstractSocket::ConnectedState) {
			_disconnecting = true;
			_socket->close();
			emit stateChanged(RemoteReconnecting);
		}
	} else {
		emit stateChanged(RemoteReconnecting);

		//TODO sync settings?
		auto remoteUrl = _settings->value(keyRemoteUrl).toUrl();
		if(!remoteUrl.isValid()) {
			emit stateChanged(RemoteDisconnected);
			return;
		}

		_socket = new QWebSocket(_settings->value(keySharedSecret).toString(),
								 QWebSocketProtocol::VersionLatest,
								 this);

		auto conf = _defaults.property(Defaults::SslConfiguration).value<QSslConfiguration>();
		if(!conf.isNull())
			_socket->setSslConfiguration(conf);

		connect(_socket, &QWebSocket::connected,
				this, &RemoteConnector::connected);
		connect(_socket, &QWebSocket::binaryMessageReceived,
				this, &RemoteConnector::binaryMessageReceived);
		connect(_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
				this, &RemoteConnector::error);
		connect(_socket, &QWebSocket::sslErrors,
				this, &RemoteConnector::sslErrors);
		connect(_socket, &QWebSocket::disconnected,
				this, &RemoteConnector::disconnected,
				Qt::QueuedConnection);

		QNetworkRequest request(remoteUrl);
		request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
		request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
		request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);
		request.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, true);

		_settings->beginGroup(keyHeaders);
		auto keys = _settings->childKeys();
		foreach(auto key, keys)
			request.setRawHeader(key.toUtf8(), _settings->value(key).toByteArray());
		_settings->endGroup();

		_socket->open(request);
	}
}

void RemoteConnector::reloadState()
{

}

void RemoteConnector::connected()
{
	emit stateChanged(RemoteLoadingState);
}

void RemoteConnector::disconnected()
{
	if(!_disconnecting) {
		logWarning().noquote() << "Unexpected disconnect from server with exit code"
							   << _socket->closeCode()
							   << "and reason:"
							   << _socket->closeReason();
		//TODO retry
//		auto delta = retry();
//		logDebug() << "Retrying to connect to server in"
//				   << delta / 1000
//				   << "seconds";
	} else
		_disconnecting = false;

	_socket->deleteLater();
	_socket = nullptr;

	//always "disable" remote for the state changer
	emit stateChanged(RemoteDisconnected);
}

void RemoteConnector::binaryMessageReceived(const QByteArray &message)
{

}

void RemoteConnector::error()
{
	//TODO retryIndex
//	if(retryIndex == 0) {
		logWarning().noquote() << "Server connection socket error:"
							   << _socket->errorString();
//	} else {
//		logDebug().noquote() << "Repeated server connection socket error:"
//							 << _socket->errorString();
//	}

	tryClose();
}

void RemoteConnector::sslErrors(const QList<QSslError> &errors)
{
	auto shouldClose = true;
	foreach(auto error, errors) {
		if(error.error() == QSslError::SelfSignedCertificate ||
		   error.error() == QSslError::SelfSignedCertificateInChain)
			shouldClose = shouldClose &&
						  (_defaults.property(Defaults::SslConfiguration)
						   .value<QSslConfiguration>()
						   .peerVerifyMode() >= QSslSocket::VerifyPeer);
		//TODO retryIndex
//		if(retryIndex == 0) {
			logWarning().noquote() << "Server connection SSL error:"
								   << error.errorString();
//		} else {
//			logDebug().noquote() << "Repeated server connection SSL error:"
//								 << error.errorString();
//		}
	}

	if(shouldClose)
		tryClose();
}

void RemoteConnector::tryClose()
{
	logDebug() << Q_FUNC_INFO << "socket state:" << _socket->state();
	if(_socket->state() == QAbstractSocket::ConnectedState) {
		_disconnecting = true;
		_socket->close();
	} else
		emit stateChanged(RemoteDisconnected);
}

bool RemoteConnector::checkState(QAbstractSocket::SocketState state)
{
	return _socket->state() == state;
}
