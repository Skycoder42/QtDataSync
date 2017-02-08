#include "sqllocalstore.h"

#include <QJsonArray>

using namespace QtDataSync;

SqlLocalStore::SqlLocalStore(QObject *parent) :
	LocalStore(parent)
{

}

void SqlLocalStore::initialize()
{

}

void SqlLocalStore::finalize()
{

}

void SqlLocalStore::loadAll(quint64 id, int metaTypeId)
{
	emit requestCompleted(id, QJsonArray());
}

void SqlLocalStore::load(quint64 id, int metaTypeId, const QString &key, const QString &value)
{
	emit requestCompleted(id, QJsonValue::Null);
}

void SqlLocalStore::save(quint64 id, int metaTypeId, const QString &key, const QJsonObject &object)
{
	emit requestCompleted(id, QJsonValue::Undefined);
}

void SqlLocalStore::remove(quint64 id, int metaTypeId, const QString &key, const QString &value)
{
	emit requestCompleted(id, QJsonValue::Undefined);
}

void SqlLocalStore::removeAll(quint64 id, int metaTypeId)
{
	emit requestCompleted(id, QJsonValue::Undefined);
}
