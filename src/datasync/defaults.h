#ifndef QTDATASYNC_DEFAULTS_H
#define QTDATASYNC_DEFAULTS_H

#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qdir.h>
#include <QtCore/qsettings.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qdebug.h>
#include <QtCore/qreadwritelock.h>
#include <QtCore/qurl.h>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/exception.h"

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

private:
	QScopedPointer<DatabaseRefPrivate> d;
};

struct Q_DATASYNC_EXPORT RemoteConfig {
	QUrl url;
	QString accessKey;
	QHash<QByteArray, QByteArray> headers;

	RemoteConfig(const QUrl &url = {}, const QString &accessKey = {}, const QHash<QByteArray, QByteArray> &headers = {});
};

class DefaultsPrivate;
//! A helper class to get defaults per datasync instance (threadsafe)
class Q_DATASYNC_EXPORT Defaults
{
	Q_GADGET

public:
	enum PropertyKey {
		CacheSize,
		SslConfiguration,
		RemoteConfiguration,
		KeyStoreProvider,
		RsaKeySize
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

	//! Aquire the standard sqlite database
	DatabaseRef aquireDatabase(QObject *object) const;
	QReadWriteLock *databaseLock() const;

private:
	QSharedPointer<DefaultsPrivate> d;
};

class Q_DATASYNC_EXPORT SetupDoesNotExistException : public Exception
{
public:
	SetupDoesNotExistException(const QString &setupName);

	QByteArray className() const noexcept override;
	void raise() const override;
	QException *clone() const override;

protected:
	SetupDoesNotExistException(const SetupDoesNotExistException * const other);
};

}

Q_DECLARE_METATYPE(QtDataSync::RemoteConfig)

#endif // QTDATASYNC_DEFAULTS_H
