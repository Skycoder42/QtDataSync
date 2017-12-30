#ifndef THREADEDCLIENT_P_H
#define THREADEDCLIENT_P_H

#include <QtCore/QUrl>

#include <QtRemoteObjects/QtROServerFactory>

#include "qtdatasync_global.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT ThreadedClientIoDevice : public ClientIoDevice
{
	Q_OBJECT

public:
	explicit ThreadedClientIoDevice(QObject *parent = nullptr);

	void connectToServer() override;
	bool isOpen() override;
	QIODevice *connection() override;

protected:
	void doClose() override;
};

}

#endif // THREADEDCLIENT_P_H
