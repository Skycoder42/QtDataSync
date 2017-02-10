#include "wsremoteconnector.h"

#include <QDebug>
#include <QJsonObject>
using namespace QtDataSync;

WsRemoteConnector::WsRemoteConnector(QObject *parent) :
	RemoteConnector(parent)
{}

void WsRemoteConnector::initialize()
{
	qDebug(Q_FUNC_INFO);
	emit remoteStateChanged(true, {});
}

void WsRemoteConnector::finalize()
{
	qDebug(Q_FUNC_INFO);
}

Authenticator *WsRemoteConnector::createAuthenticator(QObject *parent)
{
	qDebug(Q_FUNC_INFO);
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
