#include "authenticator.h"
#include "remoteconnector.h"

using namespace QtDataSync;

Authenticator::Authenticator(QObject *parent) :
	QObject(parent)
{}

GenericTask<void> Authenticator::resetIdentity(const QVariant &extraData)
{
	QFutureInterface<QVariant> futureInterface;
	QMetaObject::invokeMethod(connector(), "resetUserId", Qt::QueuedConnection,
							  Q_ARG(QFutureInterface<QVariant>, futureInterface),
							  Q_ARG(QVariant, extraData));
	return futureInterface;
}
