#ifndef QTDATASYNC_AUTHENTICATOR_H
#define QTDATASYNC_AUTHENTICATOR_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/task.h"

#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/qiodevice.h>

#include <functional>
#include <type_traits>

namespace QtDataSync {

class RemoteConnector;

//! The base class for all authenticator objects
class Q_DATASYNC_EXPORT Authenticator : public QObject
{
	Q_OBJECT

public:
	//! Constructor
	explicit Authenticator(QObject *parent = nullptr);

	//! Export all user-related remote data to the given device
	void exportUserData(QIODevice *device) const;
	//! Export all user-related remote data
	QByteArray exportUserData() const;
	//! Import user-related remote data from the given device
	GenericTask<void> importUserData(QIODevice *device);
	//! Import user-related remote data
	GenericTask<void> importUserData(QByteArray data);

protected:
	//! Call this method to reset the users identity.
	GenericTask<void> resetIdentity(const QVariant &extraData = {}, bool resetLocalStore = true);

	virtual void exportUserDataImpl(QIODevice *device) const = 0;
	virtual GenericTask<void> importUserDataImpl(QIODevice *device) = 0;

	//! Returns a reference to the connector this authenticator belongs to
	virtual RemoteConnector *connector() = 0;
};

}

#endif // QTDATASYNC_AUTHENTICATOR_H
