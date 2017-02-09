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

void WsRemoteConnector::download(const StateHolder::ChangeKey &key, const QString &value)
{

}

void WsRemoteConnector::upload(const StateHolder::ChangeKey &key, const QString &value, const QJsonObject &object)
{

}

void WsRemoteConnector::remove(const StateHolder::ChangeKey &key, const QString &value)
{

}

void WsRemoteConnector::markUnchanged(const StateHolder::ChangeKey &key, const QString &value)
{

}
