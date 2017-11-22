#ifndef LOCALSTORE_P_H
#define LOCALSTORE_P_H

#include "qtdatasync_global.h"
#include "defaults.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>

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

private:
};

}

#endif // LOCALSTORE_P_H
