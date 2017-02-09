#ifndef WSREMOTECONNECTOR_H
#define WSREMOTECONNECTOR_H

#include "remoteconnector.h"

#include <QObject>

namespace QtDataSync {

class WsRemoteConnector : public RemoteConnector
{
	Q_OBJECT
public:
	explicit WsRemoteConnector(QObject *parent = nullptr);

	void initialize() override;
	void finalize() override;

	Authenticator *createAuthenticator(QObject *parent) override;

public slots:
	void download(const QByteArray &typeName, const QString &key, const QString &value) override;
	void upload(const QByteArray &typeName, const QString &key, const QString &value, const QJsonObject &object) override;
	void remove(const QByteArray &typeName, const QString &key, const QString &value) override;
	void markUnchanged(const QByteArray &typeName, const QString &key, const QString &value) override;
};

}

#endif // WSREMOTECONNECTOR_H
