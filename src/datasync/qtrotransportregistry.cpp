#include "qtrotransportregistry.h"
#include <QtCore/QReadWriteLock>
#include <QtCore/QHash>
#include <QtCore/QPair>
#include <QtCore/QSharedPointer>
#include <QtCore/QGlobalStatic>
using namespace QtDataSync;
using namespace QtDataSync::QtRoTransportRegistry;

namespace {

struct QtRoRegister
{
	QReadWriteLock lock;
	QHash<QString, QPair<QSharedPointer<Server>, QSharedPointer<Client>>> instances;
};

}

Q_GLOBAL_STATIC(QtRoRegister, qtRoRegister)

void QtDataSync::QtRoTransportRegistry::registerTransport(const QString &urlScheme, Server *server, Client *client)
{
	QWriteLocker lock{&qtRoRegister->lock};
	qtRoRegister->instances.insert(urlScheme, {QSharedPointer<Server>{server}, QSharedPointer<Client>{client}});
}

bool QtDataSync::QtRoTransportRegistry::connectHostBaseNode(const QUrl &url, QRemoteObjectHostBase *node)
{
	QReadLocker lock{&qtRoRegister->lock};
	auto server = qtRoRegister->instances.value(url.scheme()).first;
	lock.unlock();

	if(server)
		return server->prepareHostNode(url, node);
	else
		return false;
}

void QtDataSync::QtRoTransportRegistry::connectHostNode(const QUrl &url, QRemoteObjectHost *node)
{
	if(connectHostBaseNode(url, node))
		node->setHostUrl(url, QRemoteObjectHostBase::AllowExternalRegistration);
	else
		node->setHostUrl(url);
}

bool QtDataSync::QtRoTransportRegistry::connectClientNode(const QUrl &url, QRemoteObjectNode *node)
{
	QReadLocker lock{&qtRoRegister->lock};
	auto client = qtRoRegister->instances.value(url.scheme()).second;
	lock.unlock();

	if(client)
		return client->prepareClientNode(url, node);
	else
		return node->connectToNode(url);
}

Server::~Server() = default;

Client::~Client() = default;

// ------------- GENERIC IMPLEMENTATION -------------

__private::FuncServer::FuncServer(std::function<bool (QUrl, QRemoteObjectHostBase *)> &&fn) :
	_fn{std::move(fn)}
{}

bool __private::FuncServer::prepareHostNode(const QUrl &url, QRemoteObjectHostBase *host)
{
	return _fn(url, host);
}

__private::FuncClient::FuncClient(std::function<bool (QUrl, QRemoteObjectNode *)> &&fn) :
	_fn{std::move(fn)}
{}

bool __private::FuncClient::prepareClientNode(const QUrl &url, QRemoteObjectNode *node)
{
	return _fn(url, node);
}
