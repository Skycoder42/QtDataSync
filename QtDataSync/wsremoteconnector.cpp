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

void WsRemoteConnector::download(const ObjectKey &key, const QByteArray &keyProperty)
{

}

void WsRemoteConnector::upload(const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty)
{

}

void WsRemoteConnector::remove(const ObjectKey &key, const QByteArray &keyProperty)
{

}

void WsRemoteConnector::markUnchanged(const ObjectKey &key, const QByteArray &keyProperty)
{

}
