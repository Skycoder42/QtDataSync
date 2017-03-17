#include "mocklocalstore.h"

#include <QJsonArray>

MockLocalStore::MockLocalStore(QObject *parent) :
	LocalStore(parent),
	mutex(QMutex::Recursive),
	enabled(false),
	pseudoStore(),
	failCount(0)
{}

QList<QtDataSync::ObjectKey> MockLocalStore::loadAllKeys()
{
	QMutexLocker _(&mutex);
	if(!enabled)
		return {};
	else
		return pseudoStore.keys();
}

void MockLocalStore::resetStore()
{
	QMutexLocker _(&mutex);
	if(!enabled)
		return;
	else
		pseudoStore.clear();
}

void MockLocalStore::count(quint64 id, const QByteArray &typeName)
{
	QMutexLocker _(&mutex);
	if(!enabled) {
		emit requestCompleted(id, 0);
		return;
	} else if(failCount > 0)
		emit requestFailed(id, QString::number(failCount--));
	else {
		auto cnt = 0;
		foreach(auto key, pseudoStore.keys()) {
			if(key.first == typeName)
				cnt++;
		}

		emit requestCompleted(id, cnt);
	}
}

void MockLocalStore::keys(quint64 id, const QByteArray &typeName)
{
	QMutexLocker _(&mutex);
	if(!enabled) {
		emit requestCompleted(id, QJsonArray());
		return;
	} else if(failCount > 0)
		emit requestFailed(id, QString::number(failCount--));
	else {
		QStringList keys;
		foreach(auto key, pseudoStore.keys()) {
			if(key.first == typeName)
				keys.append(key.second);
		}

		emit requestCompleted(id, QJsonArray::fromStringList(keys));
	}
}

void MockLocalStore::loadAll(quint64 id, const QByteArray &typeName)
{
	QMutexLocker _(&mutex);
	if(!enabled) {
		emit requestCompleted(id, QJsonArray());
		return;
	} else if(failCount > 0)
		emit requestFailed(id, QString::number(failCount--));
	else {
		QJsonArray data;
		foreach(auto key, pseudoStore.keys()) {
			if(key.first == typeName)
				data.append(pseudoStore[key]);
		}

		emit requestCompleted(id, data);
	}
}

void MockLocalStore::load(quint64 id, const QtDataSync::ObjectKey &key, const QByteArray &)
{
	QMutexLocker _(&mutex);
	if(!enabled) {
		emit requestCompleted(id, QJsonValue::Null);
		return;
	} else if(failCount > 0)
		emit requestFailed(id, QString::number(failCount--));
	else {
		if(pseudoStore.contains(key))
			emit requestCompleted(id, pseudoStore.value(key));
		else
			emit requestCompleted(id, QJsonValue::Null);
	}
}

void MockLocalStore::save(quint64 id, const QtDataSync::ObjectKey &key, const QJsonObject &object, const QByteArray &)
{
	QMutexLocker _(&mutex);
	if(!enabled) {
		emit requestCompleted(id, QJsonValue::Undefined);
		return;
	} else if(failCount > 0)
		emit requestFailed(id, QString::number(failCount--));
	else {
		pseudoStore.insert(key, object);
		emit requestCompleted(id, QJsonValue::Undefined);
	}
}

void MockLocalStore::remove(quint64 id, const QtDataSync::ObjectKey &key, const QByteArray &)
{
	QMutexLocker _(&mutex);
	if(!enabled) {
		emit requestCompleted(id, QJsonValue::Undefined);
		return;
	} else if(failCount > 0)
		emit requestFailed(id, QString::number(failCount--));
	else
		emit requestCompleted(id, pseudoStore.remove(key) > 0);
}

void MockLocalStore::search(quint64 id, const QByteArray &typeName, const QString &searchQuery)
{
	QMutexLocker _(&mutex);
	if(!enabled) {
		emit requestCompleted(id, QJsonArray());
		return;
	} else if(failCount > 0)
		emit requestFailed(id, QString::number(failCount--));
	else {
		QJsonArray data;
		foreach(auto key, pseudoStore.keys()) {
			if(key.first == typeName &&
			   key.second.contains(searchQuery))
				data.append(pseudoStore[key]);
		}

		emit requestCompleted(id, data);
	}
}
