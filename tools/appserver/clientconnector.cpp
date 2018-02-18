#include "clientconnector.h"
#include <QFile>
#include <QSslKey>
#include <QWebSocket>
#include <QWebSocketCorsAuthenticator>
#include "app.h"

ClientConnector::ClientConnector(DatabaseController *database, QObject *parent) :
	QObject(parent),
	database(database),
	server(nullptr),
	secret(),
	clients()
{
	auto name = qApp->configuration()->value(QStringLiteral("server/name"), QCoreApplication::applicationName()).toString();
	auto mode = qApp->configuration()->value(QStringLiteral("server/wss"), false).toBool() ? QWebSocketServer::SecureMode : QWebSocketServer::NonSecureMode;
	secret = qApp->configuration()->value(QStringLiteral("server/secret")).toString();

	server = new QWebSocketServer(name, mode, this);
	connect(server, &QWebSocketServer::newConnection,
			this, &ClientConnector::newConnection);
	connect(server, &QWebSocketServer::serverError,
			this, &ClientConnector::serverError);
	connect(server, &QWebSocketServer::sslErrors,
			this, &ClientConnector::sslErrors);
	if(!secret.isEmpty()) {
		connect(server, &QWebSocketServer::originAuthenticationRequired,
				this, &ClientConnector::verifySecret);
	}

	connect(database, &DatabaseController::notifyChanged,
			this, &ClientConnector::notifyChanged,
			Qt::QueuedConnection);
}

bool ClientConnector::setupWss()
{
	if(server->secureMode() != QWebSocketServer::SecureMode) {
		qInfo() << "Server running in WS (unsecure) mode";
		return true;
	}

	auto filePath = qApp->configuration()->value(QStringLiteral("server/wss/pfx")).toString();
	filePath = qApp->absolutePath(filePath);

	QSslKey privateKey;
	QSslCertificate localCert;
	QList<QSslCertificate> caCerts;

	QFile file(filePath);
	if(!file.open(QIODevice::ReadOnly)) {
		qCritical() << "Failed to open"
					<< filePath
					<< "with error:"
					<< file.errorString();
		return false;
	}

	if(QSslCertificate::importPkcs12(&file,
								  &privateKey,
								  &localCert,
								  &caCerts,
								  qApp->configuration()->value(QStringLiteral("server/wss/pass")).toString().toUtf8())) {
		auto conf = server->sslConfiguration();
		conf.setLocalCertificate(localCert);
		conf.setPrivateKey(privateKey);
		caCerts.append(conf.caCertificates());
		conf.setCaCertificates(caCerts);
		server->setSslConfiguration(conf);

		qInfo() << "Setup server to run in WSS (secure) mode";
		return true;
	} else {
		qCritical() << "File"
					<< filePath
					<< "is not a valid PKCS#12 file, or the passphrase was wrong";
		return false;
	}
}

bool ClientConnector::listen()
{
	QHostAddress host {
		qApp->configuration()->value(QStringLiteral("server/host"),
									 QHostAddress(QHostAddress::Any).toString())
				.toString()
	};

	auto port = static_cast<quint16>(qApp->configuration()->value(QStringLiteral("server/port"), 4242).toUInt());
	if(server->listen(host, port)) {
		if(port != server->serverPort())
			qInfo() << "Listening on port" << server->serverPort();
		else
			qDebug() << "Listening on port" << port;
		return true;
	} else {
		qCritical() << "Failed to listen as"
					<< host
					<< "on port"
					<< port
					<< "with error:"
					<< server->errorString();
		return false;
	}
}

void ClientConnector::close()
{
	server->close();
	emit disconnectAll();
}

void ClientConnector::setPaused(bool paused)
{
	if(paused) {
		server->pauseAccepting();
		emit disconnectAll();
	} else
		server->resumeAccepting();
}

void ClientConnector::notifyChanged(const QUuid &deviceId)
{
	auto client = clients.value(deviceId);
	if(client)
		client->notifyChanged();
}

void ClientConnector::verifySecret(QWebSocketCorsAuthenticator *authenticator)
{
	if(secret.isNull())
		authenticator->setAllowed(true);
	else
		authenticator->setAllowed(authenticator->origin() == secret);
}

void ClientConnector::newConnection()
{
	while (server->hasPendingConnections()) {
		auto socket = server->nextPendingConnection();
		auto client = new Client(database, socket, this);
		//queued is needed because they are emitted from threads
		connect(client, &Client::connected,
				this, &ClientConnector::clientConnected,
				Qt::QueuedConnection);
		connect(client, &Client::proofRequested,
				this, &ClientConnector::proofRequested,
				Qt::QueuedConnection);
		connect(this, &ClientConnector::disconnectAll,
				client, &Client::dropConnection);
	}
}

void ClientConnector::serverError()
{
	qWarning() << "Server error:"
			   << server->errorString();
}

void ClientConnector::sslErrors(const QList<QSslError> &errors)
{
	for(auto error : errors) {
		qWarning() << "SSL error:"
				   << error.errorString();
	}
}

void ClientConnector::clientConnected(const QUuid &deviceId)
{
	auto client = qobject_cast<Client*>(sender());
	if(!client)
		return;

	clients.insert(deviceId, client);
	connect(client, &Client::forceDisconnect,
			this, &ClientConnector::forceDisconnect,
			Qt::QueuedConnection);
	connect(client, &Client::destroyed, this, [this, deviceId](){
		clients.remove(deviceId);
	});
}

void ClientConnector::proofRequested(const QUuid &partner, const QtDataSync::ProofMessage &message)
{
	auto client = qobject_cast<Client*>(sender());
	if(!client)
		return;

	QPointer<Client> pClient = clients.value(partner);
	if(!pClient)
		client->proofResult(false);
	else {
		auto devId = message.deviceId;
		connect(pClient, &Client::proofDone,
				client, [devId, pClient, client](const QUuid &partner, bool success, const QtDataSync::AcceptMessage &message) {
			if(devId == partner) {
				if(pClient) {
					// remove this connections
					pClient->disconnect(client);
					// once client was added, notify pClient so he can ack the accept
					connect(client, &Client::connected,
							pClient, [pClient](const QUuid &accPartner) {
						pClient->acceptDone(accPartner);
						//no disconnect needed, single time emit
					}, Qt::QueuedConnection);
				}
				// pass message on to client
				client->proofResult(success, message);
			}
		}, Qt::QueuedConnection);
		pClient->sendProof(message);
	}
}

void ClientConnector::forceDisconnect(const QUuid &partner)
{
	auto client = clients.value(partner);
	if(client)
		client->dropConnection();
}
