#include "mockremoteconnector.h"
#include <QTimer>
using namespace QtDataSync;

MockRemoteConnector::MockRemoteConnector(QObject *parent) :
	RemoteConnector(parent),
	mutex(QMutex::Recursive),
	enabled(false),
	connected(false),
	failCount(0),
	reloadTimeout(1000),
	pseudoStore(),
	pseudoState()
{}

QtDataSync::Authenticator *MockRemoteConnector::createAuthenticator(QtDataSync::Defaults *, QObject *)
{
	return nullptr;
}

void MockRemoteConnector::updateConnected(bool resync)
{
	QMetaObject::invokeMethod(this, resync ? "requestResync" : "reloadRemoteState", Qt::BlockingQueuedConnection);
}

void MockRemoteConnector::reloadRemoteState()
{
	QMutexLocker _(&mutex);

	emit remoteStateLoaded(RemoteDisconnected, {});
	if(connected) {
		emit remoteStateLoaded(RemoteLoadingState, {});
		QTimer::singleShot(reloadTimeout, this, [=](){
			QMutexLocker _(&mutex);
			emit remoteStateLoaded(RemoteReady, enabled ? pseudoState : StateHolder::ChangeHash());
		});
	}
}

void MockRemoteConnector::requestResync()
{
	QMutexLocker _(&mutex);
	if(connected) {
		emit performLocalReset(false);
		emit remoteStateLoaded(RemoteLoadingState, {});
		QTimer::singleShot(reloadTimeout, this, [=](){
			QMutexLocker _(&mutex);
			emit remoteStateLoaded(RemoteReady, enabled ? pseudoState : StateHolder::ChangeHash());
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

void MockRemoteConnector::resetUserData(const QVariant &)
{
	QMutexLocker _(&mutex);
}
