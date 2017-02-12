#include "clientconnector.h"
#include <QFile>
#include <QSslKey>
#include <QWebSocket>

ClientConnector::ClientConnector(const QString &name, bool wss, QObject *parent) :
	QObject(parent),
	server(new QWebSocketServer(name, wss ? QWebSocketServer::SecureMode : QWebSocketServer::NonSecureMode, this)),
	clients()
{
	connect(server, &QWebSocketServer::newConnection,
			this, &ClientConnector::newConnection);
	connect(server, &QWebSocketServer::serverError,
			this, &ClientConnector::serverError);
	connect(server, &QWebSocketServer::sslErrors,
			this, &ClientConnector::sslErrors);
}

bool ClientConnector::setupWss(const QString &p12File, const QString &passphrase)
{
	QSslKey privateKey;
	QSslCertificate localCert;
	QList<QSslCertificate> caCerts;

	QFile file(p12File);
	if(!file.open(QIODevice::ReadOnly))
		return false;

	if(QSslCertificate::importPkcs12(&file,
								  &privateKey,
								  &localCert,
								  &caCerts,
								  passphrase.toUtf8())) {
		auto conf = server->sslConfiguration();
		conf.setLocalCertificate(localCert);
		conf.setPrivateKey(privateKey);
		caCerts.append(conf.caCertificates());
		conf.setCaCertificates(caCerts);
		server->setSslConfiguration(conf);
		return true;
	} else
		return false;
}

bool ClientConnector::listen(const QHostAddress &hostAddress, quint16 port)
{
	if(server->listen(hostAddress, port)) {
		qInfo() << "Listening on port" << server->serverPort();
		return true;
	} else {
		qCritical() << "Failed to create server with error:"
					<< server->errorString();
		return false;
	}
}

void ClientConnector::newConnection()
{
	while (server->hasPendingConnections()) {
		auto socket = server->nextPendingConnection();
		auto client = new Client(socket, this);
		connect(client, &Client::connected, this, [=](QUuid devId){
			clients.insert(devId, client);
			qDebug() << "connected" << devId;
			connect(client, &Client::destroyed, this, [=](){
				clients.remove(devId);
				qDebug() << "disconnected" << devId;
			});
		});
	}
}

void ClientConnector::serverError()
{
	qWarning() << "Server error"
			   << server->errorString();
}

void ClientConnector::sslErrors(const QList<QSslError> &errors)
{
	foreach(auto error, errors) {
		qWarning() << "SSL errors"
				   << error.errorString();
	}
}
