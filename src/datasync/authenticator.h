#ifndef QTDATASYNC_AUTHENTICATOR_H
#define QTDATASYNC_AUTHENTICATOR_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/task.h"

#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>

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

protected:
	//! Call this method to reset the users identity.
	GenericTask<void> resetIdentity(const QVariant &extraData = {}, bool resetLocalStore = true);

	//! Returns a reference to the connector this authenticator belongs to
	virtual RemoteConnector *connector() = 0;
};

}

#endif // QTDATASYNC_AUTHENTICATOR_H
