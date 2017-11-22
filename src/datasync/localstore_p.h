#ifndef LOCALSTORE_P_H
#define LOCALSTORE_P_H

#include "qtdatasync_global.h"
#include "defaults.h"
#include "logger.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QCache>

#include <QtSql/QSqlDatabase>

#include <QReadWriteLock>

namespace QtDataSync {

class Q_DATASYNC_EXPORT LocalStore : public QObject
{
	Q_OBJECT

public:
	explicit LocalStore(QObject *parent = nullptr);
	explicit LocalStore(const QString &setupName, QObject *parent = nullptr);

	quint64 count(const QByteArray &typeName);
	QStringList keys(const QByteArray &typeName);

	QList<QJsonObject> loadAll(const QByteArray &typeName);
	QJsonObject load(const ObjectKey &id);
	void save(const ObjectKey &id, const QJsonObject &data);
	void remove(const ObjectKey &id);
	QList<QJsonObject> find(const QByteArray &typeName, const QString &query);

Q_SIGNALS:
	void dataChanged(const ObjectKey &key);

private:
	static QReadWriteLock globalLock;

	Defaults defaults;
	Logger *logger;

	QSqlDatabase database;
	DatabaseRef dbRef;

	QHash<QByteArray, QString> tableNameCache;
	QCache<ObjectKey, QJsonObject> dataCache;//TODO clear on changed

	QString table(const QByteArray &typeName);
	QDir typeDirectory(const QByteArray &typeName);
	QJsonObject readJson(const QByteArray &typeName, const QString &fileName);
};

}

#endif // LOCALSTORE_P_H
