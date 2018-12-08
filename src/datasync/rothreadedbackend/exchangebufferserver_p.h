#ifndef EXCHANGEBUFFERSERVER_P_H
#define EXCHANGEBUFFERSERVER_P_H

#include <QtCore/QUrl>
#include <QtCore/QMutex>
#include <QtCore/QPointer>
#include <QtCore/QHash>

#include "qtdatasync_global.h"
#include "exchangebuffer_p.h"
#include "qtrotransportregistry.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ExchangeBufferServer : public QObject
{
	Q_OBJECT

public:
	static const QString UrlScheme();
	static bool connectTo(const QUrl &url,
						  ExchangeBuffer *clientBuffer,
						  bool allowCaching = true);

	explicit ExchangeBufferServer(QRemoteObjectHostBase *host);

	bool registerInstance(const QUrl &url);

private Q_SLOTS:
	void addConnection(ExchangeBuffer *clientBuffer);

private:
	static QMutex _lock;
	static QHash<QString, ExchangeBufferServer*> _servers;
	static QMultiHash<QString, QPointer<ExchangeBuffer>> _connectCache;

	QRemoteObjectHostBase *_host;
	QUrl _listenAddress;
};

}

#endif // EXCHANGEBUFFERSERVER_P_H
