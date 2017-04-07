#ifndef QTDATASYNC_LOCALSTORE_H
#define QTDATASYNC_LOCALSTORE_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/defaults.h"

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qdir.h>

namespace QtDataSync {

//! The class responsible for storing data locally
class Q_DATASYNC_EXPORT LocalStore : public QObject
{
	Q_OBJECT

public:
	//! Constructor
	explicit LocalStore(QObject *parent = nullptr);

	//! Called from the engine to initialize the store
	virtual void initialize(Defaults *defaults);
	//! Called from the engine to finalize the store
	virtual void finalize();

	//! Load all keys for all types that exist in the local store
	virtual QList<ObjectKey> loadAllKeys() = 0;
	//! Reset the whole store by deleting all data
	virtual void resetStore() = 0;

public Q_SLOTS:
	//! Count the number of datasets of the given type
	virtual void count(quint64 id, const QByteArray &typeName) = 0;
	//! Load all keys of datasets of the given type
	virtual void keys(quint64 id, const QByteArray &typeName) = 0;
	//! Load all datasets of the given type
	virtual void loadAll(quint64 id, const QByteArray &typeName) = 0;
	//! Load the datasets of the given type and key, and the specified key property
	virtual void load(quint64 id, const ObjectKey &key, const QByteArray &keyProperty) = 0;
	//! Save the datasets of the given type and key, and the specified key property
	virtual void save(quint64 id, const ObjectKey &key, const QJsonObject &object, const QByteArray &keyProperty) = 0;
	//! Remove the datasets of the given type and key, and the specified key property
	virtual void remove(quint64 id, const ObjectKey &key, const QByteArray &keyProperty) = 0;
	//! Load all datasets of the given type that match the search query
	virtual void search(quint64 id, const QByteArray &typeName, const QString &searchQuery) = 0;

Q_SIGNALS:
	//! Is emitted when a request was completed successfully
	void requestCompleted(quint64 id, const QJsonValue &result);
	//! Is emitted when a request failed
	void requestFailed(quint64 id, const QString &errorString);
};

}

#endif // QTDATASYNC_LOCALSTORE_H
