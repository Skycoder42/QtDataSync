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
	void loadAll(quint64 id, int metaTypeId) override;
	void load(quint64 id, int metaTypeId, const QString &key, const QString &value) override;
	void save(quint64 id, int metaTypeId, const QString &key, const QJsonObject &object) override;
	void remove(quint64 id, int metaTypeId, const QString &key, const QString &value) override;
	void removeAll(quint64 id, int metaTypeId) override;

private:
	static const QString DatabaseName;

	QDir storageDir;
	QSqlDatabase database;

	QString tableName(int metaTypeId) const;
};

}

#endif // SQLLOCALSTORE_H
