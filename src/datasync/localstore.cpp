#include "localstore_p.h"

#include "setup.h"

using namespace QtDataSync;

LocalStore::LocalStore(QObject *parent) :
	LocalStore(Setup::DefaultSetup, parent)
{}

LocalStore::LocalStore(const QString &setupName, QObject *parent) :
	QObject(parent)
{}

quint64 LocalStore::count(const QByteArray &typeName)
{

}

QStringList LocalStore::keys(const QByteArray &typeName)
{

}

QList<QJsonObject> LocalStore::loadAll(const QByteArray &typeName)
{

}

QJsonObject LocalStore::load(const ObjectKey &id)
{

}

void LocalStore::save(const ObjectKey &id, const QJsonObject &data)
{

}

void LocalStore::remove(const ObjectKey &id)
{

}

QList<QJsonObject> LocalStore::find(const QByteArray &typeName, const QString &query)
{

}
