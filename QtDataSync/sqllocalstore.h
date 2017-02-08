#ifndef SQLLOCALSTORE_H
#define SQLLOCALSTORE_H

#include "localstore.h"

#include <QObject>

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
};

}

#endif // SQLLOCALSTORE_H
