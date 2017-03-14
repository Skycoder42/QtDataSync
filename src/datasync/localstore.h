#ifndef LOCALSTORE_H
#define LOCALSTORE_H

#include "QtDataSync/qdatasync_global.h"
#include "QtDataSync/defaults.h"

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qdir.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT LocalStore : public QObject
{
	Q_OBJECT

public:
	explicit LocalStore(QObject *parent = nullptr);

	virtual void initialize(Defaults *defaults);
	virtual void finalize();

public Q_SLOTS:
	virtual void count(quint64 id, const QByteArray &typeName) = 0;
	virtual void keys(quint64 id, const QByteArray &typeName) = 0;
	virtual void loadAll(quint64 id, const QByteArray &typeName) = 0;
	virtual void load(quint64 id, const ObjectKey &key, const QByteArray &keyProperty) = 0;
	virtual void save(quint64 id, const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) = 0;
	virtual void remove(quint64 id, const ObjectKey &key, const QByteArray &keyProperty) = 0;

	virtual void loadAllKeys() = 0;
	virtual void resetStore() = 0;

Q_SIGNALS:
	void requestCompleted(quint64 id, const QJsonValue &result);
	void requestFailed(quint64 id, const QString &errorString);
};

}

#endif // LOCALSTORE_H
