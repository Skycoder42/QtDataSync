#include "authenticator.h"
#include "remoteconnector.h"

using namespace QtDataSync;

Authenticator::Authenticator(QObject *parent) :
	QObject(parent)
{}

GenericTask<void> Authenticator::resetIdentity(const QVariant &extraData, bool resetLocalStore)
{
	QFutureInterface<QVariant> futureInterface;
	futureInterface.reportStarted();
	QMetaObject::invokeMethod(connector(), "resetUserId", Qt::QueuedConnection,
							  Q_ARG(QFutureInterface<QVariant>, futureInterface),
							  Q_ARG(QVariant, extraData),
							  Q_ARG(bool, resetLocalStore));
	return futureInterface;
}
