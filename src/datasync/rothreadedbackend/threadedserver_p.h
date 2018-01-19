#ifndef THREADEDSERVER_P_H
#define THREADEDSERVER_P_H

#include <QtCore/QUrl>
#include <QtCore/QMutex>

#include <QtRemoteObjects/QtROServerFactory>

#include "qtdatasync_global.h"
#include "exchangebuffer_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ThreadedServerIoDevice : public ServerIoDevice
{
	Q_OBJECT

public:
	explicit ThreadedServerIoDevice(ExchangeBuffer *buffer, QObject *parent = nullptr);
	~ThreadedServerIoDevice();

	QIODevice *connection() const override;

protected:
	void doClose() override;

private:
	ExchangeBuffer *_buffer;
};

class Q_DATASYNC_EXPORT ThreadedServer : public QConnectionAbstractServer
{
	Q_OBJECT

public:
	static const QString UrlScheme();

	static bool connectTo(const QUrl &url, ExchangeBuffer *clientBuffer);

	explicit ThreadedServer(QObject *parent = nullptr);
	~ThreadedServer();

	bool hasPendingConnections() const override;
	QUrl address() const override;
	bool listen(const QUrl &address) override;
	QAbstractSocket::SocketError serverError() const override;
	void close() override;

protected:
	ServerIoDevice *configureNewConnection() override;

private Q_SLOTS:
	void addConnection(ExchangeBuffer *buffer);

private:
	static QMutex _lock;
	static QHash<QString, ThreadedServer*> _servers;

	QUrl _listenAddress;
	QQueue<ExchangeBuffer*> _pending;
	QAbstractSocket::SocketError _lastError;
};

}

#endif // THREADEDSERVER_P_H
