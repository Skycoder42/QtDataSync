#ifndef SQLLOCALSTORE_H
#define SQLLOCALSTORE_H

#include "localstore.h"

#include <QDir>
#include <QObject>
#include <QSqlDatabase>
#include <QStringList>

namespace QtDataSync {

class SqlLocalStore : public LocalStore
{
	Q_OBJECT

public:
	explicit SqlLocalStore(QObject *parent = nullptr);

	void initialize(const QString &localDir) override;
	void finalize(const QString &localDir) override;

public slots:
	void count(quint64 id, const QByteArray &typeName) override;
	void keys(quint64 id, const QByteArray &typeName) override;
	void loadAll(quint64 id, const QByteArray &typeName) override;
	void load(quint64 id, const ObjectKey &key, const QByteArray &keyProperty) override;
	void save(quint64 id, const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) override;
	void remove(quint64 id, const ObjectKey &key, const QByteArray &keyProperty) override;

private:
	QDir storageDir;
	QSqlDatabase database;

	QString tableName(const QByteArray &typeName) const;
	bool testTableExists(const QString &tableName) const;
};

}

#endif // SQLLOCALSTORE_H
