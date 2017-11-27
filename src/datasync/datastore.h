#ifndef DATASTORE_H
#define DATASTORE_H

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qvariant.h>

#include <functional>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/objectkey.h"
#include "QtDataSync/exception.h"

namespace QtDataSync {

class Defaults;

class DataStorePrivate;
class Q_DATASYNC_EXPORT DataStore : public QObject
{
	Q_OBJECT

public:
	explicit DataStore(QObject *parent = nullptr);
	explicit DataStore(const QString &setupName, QObject *parent = nullptr);
	~DataStore();

	//! @copybrief DataStore::count()
	qint64 count(int metaTypeId) const;
	//! @copybrief DataStore::keys()
	QStringList keys(int metaTypeId) const;
	//! @copybrief DataStore::loadAll()
	QVariantList loadAll(int metaTypeId) const;
	//! @copybrief DataStore::load(const QString &)
	QVariant load(int metaTypeId, const QString &key) const;
	//! @copybrief DataStore::load(const K &)
	inline QVariant load(int metaTypeId, const QVariant &key) const {
		return load(metaTypeId, key.toString());
	}
	//! @copybrief DataStore::save(const T &)
	void save(int metaTypeId, QVariant value);
	//! @copybrief DataStore::remove(const QString &)
	bool remove(int metaTypeId, const QString &key);
	//! @copybrief DataStore::remove(const K &)
	bool remove(int metaTypeId, const QVariant &key) {
		return remove(metaTypeId, key.toString());
	}
	//! @copybrief DataStore::search(const QString &)
	QVariantList search(int metaTypeId, const QString &query) const;
	//! @copybrief DataStore::iterate(const std::function<bool(T)> &, const std::function<void(const QException &)> &)
	void iterate(int metaTypeId,
				 const std::function<bool(QVariant)> &iterator,
				 const std::function<void(const QException &)> &onExcept = {}) const;

public Q_SLOTS:

Q_SIGNALS:

private:
	QScopedPointer<DataStorePrivate> d;
};



class Q_DATASYNC_EXPORT DataStoreException : public Exception
{
protected:
	DataStoreException(const Defaults &defaults, const QString &message);
	DataStoreException(const DataStoreException * const other);
};

class Q_DATASYNC_EXPORT LocalStoreException : public DataStoreException
{
public:
	LocalStoreException(const Defaults &defaults, const ObjectKey &key, const QString &context, const QString &message);

	ObjectKey key() const;
	QString context() const;

	QByteArray className() const noexcept override;
	QString qWhat() const override;
	void raise() const override;
	QException *clone() const override;

protected:
	LocalStoreException(const LocalStoreException * const other);

	const ObjectKey _key;
	const QString _context;
};

class Q_DATASYNC_EXPORT NoDataException : public DataStoreException
{
public:
	NoDataException(const Defaults &defaults, const ObjectKey &key);

	ObjectKey key() const;

	QByteArray className() const noexcept override;
	QString qWhat() const override;
	void raise() const override;
	QException *clone() const override;

protected:
	NoDataException(const NoDataException * const other);

	const ObjectKey _key;
};

class Q_DATASYNC_EXPORT InvalidDataException : public DataStoreException
{
public:
	InvalidDataException(const Defaults &defaults, const QByteArray &typeName, const QString &message);

	QByteArray typeName() const;

	QByteArray className() const noexcept override;
	QString qWhat() const override;
	void raise() const override;
	QException *clone() const override;

protected:
	InvalidDataException(const InvalidDataException * const other);

	const QByteArray _typeName;
};

}

#endif // DATASTORE_H
