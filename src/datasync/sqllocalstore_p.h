#ifndef SQLLOCALSTORE_P_H
#define SQLLOCALSTORE_P_H

#include "localstore.h"

#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QtCore/QStringList>

#include <QtSql/QSqlDatabase>

namespace QtDataSync {

class Q_DATASYNC_EXPORT SqlLocalStore : public LocalStore
{
	Q_OBJECT

public:
	explicit SqlLocalStore(QObject *parent = nullptr);

	void initialize(Defaults *defaults) override;
	void finalize() override;

	QList<ObjectKey> loadAllKeys() override;
	void resetStore() override;

public Q_SLOTS:
	void count(quint64 id, const QByteArray &typeName) override;
	void keys(quint64 id, const QByteArray &typeName) override;
	void loadAll(quint64 id, const QByteArray &typeName) override;
	void load(quint64 id, const ObjectKey &key, const QByteArray &keyProperty) override;
	void save(quint64 id, const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) override;
	void remove(quint64 id, const ObjectKey &key, const QByteArray &keyProperty) override;

private:
	Defaults *defaults;
	QSqlDatabase database;

	QDir typeDirectory(quint64 id, const QByteArray &typeName);
	bool testTableExists(const QString &typeDirectory) const;
};

}

#endif // SQLLOCALSTORE_P_H
