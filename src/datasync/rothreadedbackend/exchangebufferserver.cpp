#include "exchangebufferserver_p.h"
using namespace QtDataSync;

QMutex ExchangeBufferServer::_lock;
QHash<QString, ExchangeBufferServer*> ExchangeBufferServer::_servers;
QMultiHash<QString, QPointer<ExchangeBuffer>> ExchangeBufferServer::_connectCache;

const QString ExchangeBufferServer::UrlScheme()
{
	static const QString urlScheme{QStringLiteral("threaded")};
	return urlScheme;
}

bool ExchangeBufferServer::connectTo(const QUrl &url, ExchangeBuffer *clientBuffer, bool allowCaching)
{
	if(url.scheme() != UrlScheme() || !url.isValid()) {
		qCCritical(rothreadedbackend).noquote() << "Unsupported URL-Scheme:" << url.scheme();
		return false;
	}

	QMutexLocker _{&_lock};
	auto server = _servers.value(url.path());
	if(!server) {
		if(allowCaching) {
			qCInfo(rothreadedbackend) << "No threaded server found for" << url.path()
									  << "- queuing up connection to be continued as soon as a server registers for that url";
			_connectCache.insert(url.path(), clientBuffer);
			return true;
		} else {
			qCWarning(rothreadedbackend).noquote() << "No threaded server found for:" << url.path();
			return false;
		}
	} else {
		return QMetaObject::invokeMethod(server, "addConnection", Qt::QueuedConnection,
										 Q_ARG(ExchangeBuffer*, clientBuffer));
	}
}

ExchangeBufferServer::ExchangeBufferServer(QRemoteObjectHostBase *host) :
	QObject{host},
	_host{host}
{}

bool ExchangeBufferServer::registerInstance(const QUrl &url)
{
	if(_listenAddress.isValid()) {
		QMutexLocker _{&_lock};
		_servers.remove(_listenAddress.path());
	}

	if(url.scheme() != UrlScheme() || !url.isValid()) {
		qCCritical(rothreadedbackend).noquote() << "Unsupported URL-Scheme:" << url.scheme();
		return false;
	} else {
		QMutexLocker _{&_lock};
		if(_servers.contains(url.path())) {
			qCWarning(rothreadedbackend).noquote() << "A different server is already listening for:" << url.path();
			return false;
		} else {
			_servers.insert(url.path(), this);
			_listenAddress = url;

			// handle pending connections
			const auto pending = _connectCache.values(url.path());
			_connectCache.remove(url.path());
			for(const auto &client : pending) {
				if(client) {
					QMetaObject::invokeMethod(this, "addConnection", Qt::QueuedConnection,
											  Q_ARG(ExchangeBuffer*, client));
				}
			}
			if(!pending.isEmpty()) {
				qCDebug(rothreadedbackend).noquote() << "Picked up" << pending.size()
										   << "cached connections for:"
										   << url.password();
			}

			return true;
		}
	}
}

void ExchangeBufferServer::addConnection(ExchangeBuffer *clientBuffer)
{
	// connect the buffers
	auto hostBuffer = new ExchangeBuffer{_host};
	if(!hostBuffer->connectTo(clientBuffer)) {
		qCWarning(rothreadedbackend).noquote() << "Failed to connect to buffer with error:"
											   << hostBuffer->errorString();
		hostBuffer->deleteLater();
		return;
	}

	// add to the host
	_host->addHostSideConnection(hostBuffer);
}
