#ifndef AUTHENTICATOR_H
#define AUTHENTICATOR_H

#include "qtdatasync_global.h"
#include <QObject>
#include <QPointer>
#include <type_traits>

namespace QtDataSync {

class RemoteConnector;

class QTDATASYNCSHARED_EXPORT Authenticator : public QObject
{
	Q_OBJECT

public:
	explicit Authenticator(QObject *parent = nullptr);

public slots:
	void resetDeviceId();

protected:
	virtual RemoteConnector *connector() = 0;
};

}

#endif // AUTHENTICATOR_H
