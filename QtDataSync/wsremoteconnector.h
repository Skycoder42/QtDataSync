#ifndef WSREMOTECONNECTOR_H
#define WSREMOTECONNECTOR_H

#include "remoteconnector.h"

#include <QObject>
#include <QSettings>
#include <QWebSocket>

namespace QtDataSync {

class WsRemoteConnector : public RemoteConnector
{
	Q_OBJECT
public:
	static const QString keyRemoteUrl;
	static const QString keyHeadersGroup;
	static const QString keyVerifyPeer;
	static const QString keyUserIdentity;

	explicit WsRemoteConnector(QObject *parent = nullptr);

	void initialize(const QDir &storageDir) override;
	void finalize(const QDir &storageDir) override;

	Authenticator *createAuthenticator(const QDir &storageDir, QObject *parent) override;

public slots:
	void reconnect();

	void download(const ObjectKey &key, const QByteArray &keyProperty) override;
	void upload(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) override;
	void remove(const ObjectKey &key, const QByteArray &keyProperty) override;
	void markUnchanged(const ObjectKey &key, const QByteArray &keyProperty) override;

	void resetDeviceId() override;

private:
	QWebSocket *socket;
	QSettings *settings;

	bool connecting;
};

}

#endif // WSREMOTECONNECTOR_H
