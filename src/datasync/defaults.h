#ifndef QTDATASYNC_DEFAULTS_H
#define QTDATASYNC_DEFAULTS_H

#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qdir.h>
#include <QtCore/qsettings.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qdebug.h>

#include <QtRemoteObjects/qremoteobjectnode.h>

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

	bool isValid() const;
	QSqlDatabase database() const;
	operator QSqlDatabase() const;
	QSqlDatabase *operator->() const;

private:
	QScopedPointer<DatabaseRefPrivate> d;
};

class DefaultsPrivate;
//! A helper class to get defaults per datasync instance (threadsafe)
class Q_DATASYNC_EXPORT Defaults
{
	Q_GADGET
	friend class DefaultsPrivate;

public:
	enum PropertyKey {
		CacheSize,
		PersistDeleted,
		ConflictPolicy,
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

	Defaults();
	Defaults(const QSharedPointer<DefaultsPrivate> &d);
	Defaults(const Defaults &other);
	~Defaults();

	//! Create a new logger instance
	Logger *createLogger(const QByteArray &subCategory, QObject *parent = nullptr) const;

	//! Returns the name of the current setup
	QString setupName() const;
	//! Returns the storage directory
	QDir storageDir() const;
	QUrl remoteAddress() const;
	QRemoteObjectNode *remoteNode() const;
	//! Returns a new instance of QSettings for this setup
	QSettings *createSettings(QObject *parent = nullptr, const QString &group = {}) const;
	//! Returns the serializer of the current setup
	const QJsonSerializer *serializer() const;
	const ConflictResolver *conflictResolver() const;
	QVariant property(PropertyKey key) const;

	static QVariant defaultParam(Setup::SignatureScheme scheme);
	static QVariant defaultParam(Setup::EncryptionScheme scheme);

	//! Aquire the standard sqlite database
	DatabaseRef aquireDatabase(QObject *object) const;

private:
	QSharedPointer<DefaultsPrivate> d;
};

}

#endif // QTDATASYNC_DEFAULTS_H
