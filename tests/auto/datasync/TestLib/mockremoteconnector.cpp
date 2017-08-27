#include "mockremoteconnector.h"
#include <QTimer>
using namespace QtDataSync;

MockRemoteConnector::MockRemoteConnector(QObject *parent) :
	RemoteConnector(parent),
	mutex(QMutex::Recursive),
	enabled(false),
	connected(false),
	failCount(0),
	reloadTimeout(100),
	pseudoStore(),
	pseudoState()
{}

QtDataSync::Authenticator *MockRemoteConnector::createAuthenticator(QtDataSync::Defaults *, QObject *parent)
{
	return new MockAuthenticator(this, parent);
}

void MockRemoteConnector::initialize(Defaults *, Encryptor *)
{
	reloadRemoteState();
}

bool MockRemoteConnector::isSyncEnabled() const
{
	return connected;
}

bool MockRemoteConnector::setSyncEnabled(bool syncEnabled)
{
	if(syncEnabled != connected) {
		connected = syncEnabled;
		if(!connected)
			emit remoteStateChanged(RemoteDisconnected);
		else
			reloadRemoteState();
		return true;
	} else
		return false;
}

void MockRemoteConnector::updateConnected(bool resync)
{
	QMetaObject::invokeMethod(this, resync ? "requestResync" : "reloadRemoteState", Qt::BlockingQueuedConnection);
}

void MockRemoteConnector::doChangeEmit()
{
	QMetaObject::invokeMethod(this, "doChangeEmitImpl", Qt::BlockingQueuedConnection);
}

void MockRemoteConnector::reloadRemoteState()
{
	QMutexLocker _(&mutex);

	if(connected) {
		emit remoteStateChanged(RemoteLoadingState);
		QTimer::singleShot(reloadTimeout, this, [=](){
			QMutexLocker _(&mutex);
			if(!enabled)
				emit remoteStateChanged(RemoteReady);
			else if(failCount > 0) {
				emit remoteStateChanged(RemoteDisconnected);
				emit authenticationFailed(QString::number(--failCount));
			}  else {
				emit remoteStateChanged(RemoteReady, pseudoState);
				emit clearAuthenticationError();
			}
		});
	}
}

void MockRemoteConnector::requestResync()
{
	QMutexLocker _(&mutex);

	emit remoteStateChanged(RemoteDisconnected);
	if(connected) {
		emit performLocalReset(false);
		emit remoteStateChanged(RemoteLoadingState);
		QTimer::singleShot(reloadTimeout, this, [=](){
			QMutexLocker _(&mutex);
			if(!enabled)
				emit remoteStateChanged(RemoteReady);
			else if(failCount > 0) {
				emit remoteStateChanged(RemoteDisconnected);
				emit authenticationFailed(QString::number(--failCount));
			} else {
				emit remoteStateChanged(RemoteReady, pseudoState);
				emit clearAuthenticationError();
			}
		});
	}
}

void MockRemoteConnector::download(const QtDataSync::ObjectKey &key, const QByteArray &)
{
	QMutexLocker _(&mutex);
	if(!enabled)
		emit operationDone(QJsonValue::Null);
	else if(failCount > 0 || !connected)
		emit operationFailed(QString::number(failCount--));
	else {
		if(pseudoStore.contains(key))
			emit operationDone(pseudoStore.value(key));
		else
			emit operationDone(QJsonValue::Null);
	}
}

void MockRemoteConnector::upload(const QtDataSync::ObjectKey &key, const QJsonObject &object, const QByteArray &)
{
	QMutexLocker _(&mutex);
	if(!enabled)
		emit operationDone(QJsonValue::Undefined);
	else if(failCount > 0 || !connected)
		emit operationFailed(QString::number(failCount--));
	else {
		pseudoStore.insert(key, object);
		pseudoState.remove(key);
		emit operationDone(QJsonValue::Undefined);
	}
}

void MockRemoteConnector::remove(const QtDataSync::ObjectKey &key, const QByteArray &)
{
	QMutexLocker _(&mutex);
	if(!enabled)
		emit operationDone(QJsonValue::Undefined);
	else if(failCount > 0 || !connected)
		emit operationFailed(QString::number(failCount--));
	else {
		pseudoStore.remove(key);
		pseudoState.remove(key);
		emit operationDone(QJsonValue::Undefined);
	}
}

void MockRemoteConnector::markUnchanged(const QtDataSync::ObjectKey &key, const QByteArray &)
{
	QMutexLocker _(&mutex);
	if(!enabled)
		emit operationDone(QJsonValue::Undefined);
	else if(failCount > 0 || !connected)
		emit operationFailed(QString::number(failCount--));
	else {
		pseudoState.remove(key);
		emit operationDone(QJsonValue::Undefined);
	}
}

void MockRemoteConnector::importUserData(const QByteArray &data, QFutureInterface<QVariant> d)
{
	QMutexLocker _(&mutex);
	if(!enabled)
		d.reportException(QtDataSync::DataSyncException("remote connector not enabled"));
	else {
		pseudoUserData = data;
		d.reportResult(QVariant());
	}
	d.reportFinished();
}

void MockRemoteConnector::resetUserData(const QVariant &, const QByteArray &)
{
	QMutexLocker _(&mutex);
}

void MockRemoteConnector::doChangeEmitImpl()
{
	QMutexLocker _(&mutex);
	foreach(auto v, emitList)
		emit remoteDataChanged(v, pseudoState.value(v, StateHolder::Unchanged));
	emitList.clear();
}



MockAuthenticator::MockAuthenticator(MockRemoteConnector *connector, QObject *parent) :
	Authenticator(parent),
	_con(connector)
{}

void MockAuthenticator::exportUserDataImpl(QIODevice *device) const
{
	device->write(_con->pseudoUserData);
}

GenericTask<void> MockAuthenticator::importUserDataImpl(QIODevice *device)
{
	QFutureInterface<QVariant> d;
	d.reportStarted();
	QMetaObject::invokeMethod(_con, "importUserData", Qt::QueuedConnection,
							  Q_ARG(QByteArray, device->readAll()),
							  Q_ARG(QFutureInterface<QVariant>, d));
	return GenericTask<void>(d);
}

RemoteConnector *MockAuthenticator::connector()
{
	return _con;
}
