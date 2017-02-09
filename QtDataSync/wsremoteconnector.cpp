#include "wsremoteconnector.h"
using namespace QtDataSync;

WsRemoteConnector::WsRemoteConnector(QObject *parent) :
	RemoteConnector(parent)
{}

void WsRemoteConnector::initialize()
{

}

void WsRemoteConnector::finalize()
{

}

Authenticator *WsRemoteConnector::createAuthenticator(QObject *parent)
{

}

void WsRemoteConnector::download(const QByteArray &typeName, const QString &key, const QString &value)
{

}

void WsRemoteConnector::upload(const QByteArray &typeName, const QString &key, const QString &value, const QJsonObject &object)
{

}

void WsRemoteConnector::remove(const QByteArray &typeName, const QString &key, const QString &value)
{

}

void WsRemoteConnector::markUnchanged(const QByteArray &typeName, const QString &key, const QString &value)
{

}
