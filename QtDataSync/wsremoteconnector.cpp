#include "wsauthenticator.h"
#include "wsremoteconnector.h"

#include <QDebug>
#include <QJsonObject>
using namespace QtDataSync;

const QString WsRemoteConnector::keyUserIdentity(QStringLiteral("RemoteConnector/userIdentity"));

WsRemoteConnector::WsRemoteConnector(QObject *parent) :
	RemoteConnector(parent)
{}

void WsRemoteConnector::initialize(const QDir &storageDir)
{
	RemoteConnector::initialize(storageDir);
	reconnect();
}

void WsRemoteConnector::finalize(const QDir &storageDir)
{
	RemoteConnector::finalize(storageDir);
}

Authenticator *WsRemoteConnector::createAuthenticator(QObject *parent)
{
	return new WsAuthenticator(this, storageDir(), parent);
}

void WsRemoteConnector::reconnect()
{
	emit remoteStateChanged(true, {});
}

void WsRemoteConnector::download(const ObjectKey &key, const QByteArray &keyProperty)
{
	qDebug() << Q_FUNC_INFO << key;
	emit operationDone(QJsonObject());
}

void WsRemoteConnector::upload(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty)
{
	qDebug() << Q_FUNC_INFO << key;
	emit operationDone();
}

void WsRemoteConnector::remove(const ObjectKey &key, const QByteArray &keyProperty)
{
	qDebug() << Q_FUNC_INFO << key;
	emit operationDone();
}

void WsRemoteConnector::markUnchanged(const ObjectKey &key, const QByteArray &keyProperty)
{
	qDebug() << Q_FUNC_INFO << key;
	emit operationDone();

}

void WsRemoteConnector::resetDeviceId()
{
	RemoteConnector::resetDeviceId();
	reconnect();
}
