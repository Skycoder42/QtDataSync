#ifndef THREADEDCLIENT_P_H
#define THREADEDCLIENT_P_H

#include <QtCore/QUrl>
#include <QtCore/QTimer>

#include <QtRemoteObjects/private/qconnectionfactories_p.h>

#include "qtdatasync_global.h"

#include "exchangebuffer_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ThreadedClientIoDevice : public ClientIoDevice
{
	Q_OBJECT

public:
	explicit ThreadedClientIoDevice(QObject *parent = nullptr);
	~ThreadedClientIoDevice();

	void connectToServer() override;
	bool isOpen() const override;
	QIODevice *connection() const override;

protected:
	void doDisconnectFromServer() override;
	void doClose() override;

private Q_SLOTS:
	void deviceConnected();
	void deviceClosed();
	void connectTimeout();

private:
	ExchangeBuffer *_buffer;
	QTimer *_connectTimer;
};

}

#endif // THREADEDCLIENT_P_H
