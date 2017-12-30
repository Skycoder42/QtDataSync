#ifndef THREADEDSERVER_P_H
#define THREADEDSERVER_P_H

#include <QtCore/QUrl>

#include <QtRemoteObjects/QtROServerFactory>

#include "qtdatasync_global.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ThreadedServerIoDevice : public ServerIoDevice
{
	Q_OBJECT

public:
	explicit ThreadedServerIoDevice(QObject *parent = nullptr);

	QIODevice *connection() const override;

protected:
	void doClose() override;
};

class Q_DATASYNC_EXPORT ThreadedServer : public QConnectionAbstractServer
{
	Q_OBJECT

public:
	static const QString UrlScheme;

	explicit ThreadedServer(QObject *parent = nullptr);

	bool hasPendingConnections() const override;
	QUrl address() const override;
	bool listen(const QUrl &address) override;
	QAbstractSocket::SocketError serverError() const override;
	void close() override;

protected:
	ServerIoDevice *configureNewConnection() override;
};

}

#endif // THREADEDSERVER_P_H
