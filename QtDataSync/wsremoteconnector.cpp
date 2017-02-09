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

void WsRemoteConnector::download(quint64 id, const QByteArray &typeName, const QString &key, const QString &value)
{

}

void WsRemoteConnector::upload(quint64 id, const QByteArray &typeName, const QString &key, const QString &value, const QJsonObject &object)
{

}

void WsRemoteConnector::remove(quint64 id, const QByteArray &typeName, const QString &key, const QString &value)
{

}

void WsRemoteConnector::markUnchanged(quint64 id, const QByteArray &typeName, const QString &key, const QString &value)
{

}
