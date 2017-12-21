#ifndef QTDATASYNC_DEFAULTS_H
#define QTDATASYNC_DEFAULTS_H

#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qdir.h>
#include <QtCore/qsettings.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qdebug.h>
#include <QtCore/qreadwritelock.h>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/exception.h"
#include "QtDataSync/setup.h"

class QSqlDatabase;
class QJsonSerializer;

namespace QtDataSync {

class Logger;
class Defaults;

class DatabaseRefPrivate;
class Q_DATASYNC_EXPORT DatabaseRef
{
	Q_DISABLE_COPY(DatabaseRef)

public:
	DatabaseRef();
	~DatabaseRef();
	DatabaseRef(DatabaseRefPrivate *d);
	DatabaseRef(DatabaseRef &&other);
	DatabaseRef &operator=(DatabaseRef &&other);

	QSqlDatabase database() const;
	operator QSqlDatabase() const;
	QSqlDatabase *operator->() const;

	void createGlobalScheme(Defaults defaults);

private:
	QScopedPointer<DatabaseRefPrivate> d;
};

class DefaultsPrivate;
//! A helper class to get defaults per datasync instance (threadsafe)
class Q_DATASYNC_EXPORT Defaults
{
	Q_GADGET

public:
	enum PropertyKey {
		CacheSize,
		PersistDeleted,
		SslConfiguration,
		RemoteConfiguration,
		KeyStoreProvider,
		SignScheme,
		SignKeyParam,
		CryptScheme,
		CryptKeyParam,
		SymScheme,
		SymKeyParam
	};
	Q_ENUM(PropertyKey)

	//! Copy constructor
	Defaults(const QString &setupName); //TODO check try/catch where used
	Defaults(const Defaults &other);
	~Defaults();

	//! Create a new logger instance
	Logger *createLogger(const QByteArray &subCategory, QObject *parent = nullptr) const;

	//! Returns the name of the current setup
	QString setupName() const;
	//! Returns the storage directory
	QDir storageDir() const;
	//! Returns a new instance of QSettings for this setup
	QSettings *createSettings(QObject *parent = nullptr, const QString &group = {}) const;
	//! Returns the serializer of the current setup
	const QJsonSerializer *serializer() const;
	QVariant property(PropertyKey key) const;

	static QVariant defaultParam(Setup::SignatureScheme scheme);
	static QVariant defaultParam(Setup::EncryptionScheme scheme);

	//! Aquire the standard sqlite database
	DatabaseRef aquireDatabase(QObject *object) const;
	QReadWriteLock *databaseLock() const;

private:
	QSharedPointer<DefaultsPrivate> d;
};

}

#endif // QTDATASYNC_DEFAULTS_H
