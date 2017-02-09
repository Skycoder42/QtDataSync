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

public slots:
	void count(quint64 id, const QByteArray &typeName) override;
	void loadAll(quint64 id, const QByteArray &typeName) override;
	void load(quint64 id, const ObjectKey &key, const QByteArray &keyProperty) override;
	void save(quint64 id, const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) override;
	void remove(quint64 id, const ObjectKey &key, const QByteArray &keyProperty) override;
	void removeAll(quint64 id, const QByteArray &typeName) override;

private:
	QDir storageDir;
	QSqlDatabase database;

	QString tableName(const QByteArray &typeName) const;
	bool testTableExists(const QString &tableName) const;
};

}

#endif // SQLLOCALSTORE_H
