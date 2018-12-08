#include "clientconnector.h"
#include <QFile>
#include <QSslKey>
#include <QWebSocket>
#include <QWebSocketCorsAuthenticator>
#include "datasyncservice.h"

ClientConnector::ClientConnector(DatabaseController *database, QObject *parent) :
	QObject{parent},
	database{database}
{
	recreateServer();
	connect(database, &DatabaseController::notifyChanged,
			this, &ClientConnector::notifyChanged,
			Qt::QueuedConnection);
}

void ClientConnector::recreateServer()
{
	//stuff that always needs to be done
	emit disconnectAll();
	secret = qService->configuration()->value(QStringLiteral("server/secret")).toString();

	// stop here if activated
	if(isActivated) {
		qWarning() << "An activated service cannot restart the websocket server."
				   << "Reloading will continue with the running instance (changes to \"server/name\" and \"server/wss\" will be ignored";
		return;
	}

	// (re)create the server
	if(server) {
		server->close();
		server->deleteLater();
		server = nullptr;
	}

	server = new QWebSocketServer{
		qService->configuration()->value(QStringLiteral("server/name"), QCoreApplication::applicationName()).toString(),
		qService->configuration()->value(QStringLiteral("server/wss"), false).toBool() ? QWebSocketServer::SecureMode : QWebSocketServer::NonSecureMode,
		this
	};
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
}

bool ClientConnector::setupWss()
{
	if(server->secureMode() != QWebSocketServer::SecureMode) {
		qInfo() << "Server running in WS (unsecure) mode";
		return true;
	}

	auto filePath = qService->configuration()->value(QStringLiteral("server/wss/pfx")).toString();
	filePath = qService->absolutePath(filePath);

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
								  qService->configuration()->value(QStringLiteral("server/wss/pass")).toString().toUtf8())) {
		auto conf = QSslConfiguration::defaultConfiguration();
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
	if(isActivated) {
		qDebug() << "Still listening on port" << server->serverPort();
		return true;
	}

	quint16 port = 0;
	auto dSocket = qService->getSocket();
	if(dSocket != -1) {
		server->setNativeDescriptor(dSocket);
		if(!server->isListening()) {
			qCritical() << "Failed to listen on activated socket" << dSocket
						<< "with error:" << server->errorString();
			return false;
		}
		// mark as activated server
		isActivated = true;
	} else {
		QHostAddress host {
			qService->configuration()->value(QStringLiteral("server/host"),
										 QHostAddress(QHostAddress::Any).toString())
					.toString()
		};
		port = static_cast<quint16>(qService->configuration()->value(QStringLiteral("server/port"), 4242).toUInt());
		if(!server->listen(host, port)) {
			qCritical() << "Failed to listen on interface" << host << "and port" << port
						<< "with error:" << server->errorString();
			return false;
		}
	}

	if(port != server->serverPort())
		qInfo() << "Listening on port" << server->serverPort();
	else
		qDebug() << "Listening on port" << port;
	return true;
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

void ClientConnector::notifyChanged(QUuid deviceId)
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
	for(const auto &error : errors) {
		qWarning() << "SSL error:"
				   << error.errorString();
	}
}

void ClientConnector::clientConnected(QUuid deviceId)
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

void ClientConnector::proofRequested(QUuid partner, const QtDataSync::ProofMessage &message)
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
				client, [devId, pClient, client](QUuid cPartner, bool success, const QtDataSync::AcceptMessage &cMessage) {
			if(devId == cPartner) {
				if(pClient) {
					// remove this connections
					pClient->disconnect(client);
					// once client was added, notify pClient so he can ack the accept
					connect(client, &Client::connected,
							pClient, [pClient](QUuid accPartner) {
						pClient->acceptDone(accPartner);
						//no disconnect needed, single time emit
					}, Qt::QueuedConnection);
				}
				// pass message on to client
				client->proofResult(success, cMessage);
			}
		}, Qt::QueuedConnection);
		pClient->sendProof(message);
	}
}

void ClientConnector::forceDisconnect(QUuid partner)
{
	auto client = clients.value(partner);
	if(client)
		client->dropConnection();
}
