#ifndef SQLLOCALSTORE_H
#define SQLLOCALSTORE_H

#include "localstore.h"

#include <QDir>
#include <QObject>
#include <QSqlDatabase>

namespace QtDataSync {

class SqlLocalStore : public LocalStore
{
	Q_OBJECT

public:
	explicit SqlLocalStore(QObject *parent = nullptr);

	void initialize() override;
	void finalize() override;

	void count(quint64 id, QByteArray typeName) override;
	void loadAll(quint64 id, QByteArray typeName) override;
	void load(quint64 id, QByteArray typeName, const QString &key, const QString &value) override;
	void save(quint64 id, QByteArray typeName, const QString &key, const QString &value, const QJsonObject &object) override;
	void remove(quint64 id, QByteArray typeName, const QString &key, const QString &value) override;
	void removeAll(quint64 id, QByteArray typeName) override;

private:
	QDir storageDir;
	QSqlDatabase database;

	QString tableName(QByteArray typeName) const;
	bool testTableExists(const QString &tableName) const;
};

}

#endif // SQLLOCALSTORE_H
