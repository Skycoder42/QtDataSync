#ifndef AUTHENTICATOR_H
#define AUTHENTICATOR_H

#include "QtDataSync/qdatasync_global.h"

#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>

#include <type_traits>

namespace QtDataSync {

class RemoteConnector;

class Q_DATASYNC_EXPORT Authenticator : public QObject
{
	Q_OBJECT

public:
	explicit Authenticator(QObject *parent = nullptr);

public Q_SLOTS:
	void resetDeviceId();

protected:
	virtual RemoteConnector *connector() = 0;
};

}

#endif // AUTHENTICATOR_H
