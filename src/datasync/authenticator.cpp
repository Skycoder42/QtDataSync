#include "authenticator.h"
#include "remoteconnector.h"

#include <QtCore/QBuffer>

using namespace QtDataSync;

Authenticator::Authenticator(QObject *parent) :
	QObject(parent)
{}

void Authenticator::exportUserData(QIODevice *device) const
{
	exportUserDataImpl(device);
}

QByteArray Authenticator::exportUserData() const
{
	QBuffer buffer;
	buffer.open(QIODevice::WriteOnly);
	exportUserDataImpl(&buffer);
	auto res = buffer.data();
	buffer.close();
	return res;
}

GenericTask<void> Authenticator::importUserData(QIODevice *device)
{
	return importUserDataImpl(device);
}

GenericTask<void> Authenticator::importUserData(QByteArray data)
{
	QBuffer buffer(&data);
	buffer.open(QIODevice::ReadOnly);
	auto res = importUserDataImpl(&buffer);
	buffer.close();
	return res;
}

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
