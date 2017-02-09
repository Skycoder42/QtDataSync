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
	void download(const ObjectKey &key, const QByteArray &keyProperty) override;
	void upload(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) override;
	void remove(const ObjectKey &key, const QByteArray &keyProperty) override;
	void markUnchanged(const ObjectKey &key, const QByteArray &keyProperty) override;
};

}

#endif // WSREMOTECONNECTOR_H
