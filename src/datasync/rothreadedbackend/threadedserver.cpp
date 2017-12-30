#include "threadedserver_p.h"
using namespace QtDataSync;

const QString ThreadedServer::UrlScheme(QStringLiteral("threaded"));

ThreadedServer::ThreadedServer(QObject *parent) :
	QConnectionAbstractServer(parent)
{}

bool ThreadedServer::hasPendingConnections() const
{
	return false;
}

QUrl ThreadedServer::address() const
{
	return QUrl();
}

bool ThreadedServer::listen(const QUrl &address)
{
	if(address.scheme() != UrlScheme)
		return false;
	else
		return false;
}

QAbstractSocket::SocketError ThreadedServer::serverError() const
{
	return QAbstractSocket::UnknownSocketError;
}

void ThreadedServer::close()
{

}

ServerIoDevice *ThreadedServer::configureNewConnection()
{
	return nullptr;
}



ThreadedServerIoDevice::ThreadedServerIoDevice(QObject *parent) :
	ServerIoDevice(parent)
{}

QIODevice *ThreadedServerIoDevice::connection() const
{
	return nullptr;
}

void ThreadedServerIoDevice::doClose()
{

}
