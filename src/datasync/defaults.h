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
class EmitterAdapter;

class DatabaseRefPrivate;
//! A wrapper around QSqlDatabase to manage the connections
class Q_DATASYNC_EXPORT DatabaseRef
{
	Q_DISABLE_COPY(DatabaseRef)

public:
	//! Default constructor. Constructs an invalid database reference
	DatabaseRef();
	~DatabaseRef();
	//! @private
	DatabaseRef(DatabaseRefPrivate *d);
	//! Move constructor
	DatabaseRef(DatabaseRef &&other);
	//! Move assignment operator
	DatabaseRef &operator=(DatabaseRef &&other);

	//! Returns true if this reference points to a valid database
	bool isValid() const;
	//! Returns the database that is managed by this reference
	QSqlDatabase database() const;
	//! Implicit cast operator to the managed database
	operator QSqlDatabase() const;
	//! Arrow operator to access the database
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
	//! The keys of special properties set on the defaults
	enum PropertyKey {
		CacheSize, //!< @copybrief Setup::cacheSize
		PersistDeleted, //!< @copybrief Setup::persistDeletedVersion
		ConflictPolicy, //!< @copybrief Setup::syncPolicy
		SslConfiguration, //!< @copybrief Setup::sslConfiguration
		RemoteConfiguration, //!< @copybrief Setup::remoteConfiguration
		KeyStoreProvider, //!< @copybrief Setup::keyStoreProvider
		SignScheme, //!< @copybrief Setup::signatureScheme
		SignKeyParam, //!< @copybrief Setup::signatureKeyParam
		CryptScheme, //!< @copybrief Setup::encryptionScheme
		CryptKeyParam, //!< @copybrief Setup::encryptionKeyParam
		SymScheme, //!< @copybrief Setup::cipherScheme
		SymKeyParam //!< @copybrief Setup::cipherKeySize
	};
	Q_ENUM(PropertyKey)

	//! Default constructor. Constructs an invalid defaults object
	Defaults();
	//! @private
	Defaults(const QSharedPointer<DefaultsPrivate> &d);
	//! Copy constructor
	Defaults(const Defaults &other);
	~Defaults();

	//! Create a new logger instance
	Logger *createLogger(const QByteArray &subCategory, QObject *parent = nullptr) const;

	//! Returns the name of the current setup
	QString setupName() const;
	//! Returns the storage directory
	QDir storageDir() const;
	//! Returns the url to use for the remote object connection
	QUrl remoteAddress() const;
	//! Returns the remote object node for the current thread to connect to the engine
	QRemoteObjectNode *remoteNode() const;
	//! Returns a new instance of QSettings for this setup
	QSettings *createSettings(QObject *parent = nullptr, const QString &group = {}) const;
	//! Returns the serializer of the current setup
	const QJsonSerializer *serializer() const;
	//! Returns the conflict resolver of the current setup
	const ConflictResolver *conflictResolver() const;
	//! Returns the extra property defined by the given key
	QVariant property(PropertyKey key) const;

	//! Returns the standard key parameter for the given signature scheme
	static QVariant defaultParam(Setup::SignatureScheme scheme);
	//! Returns the standard key parameter for the given encryption scheme
	static QVariant defaultParam(Setup::EncryptionScheme scheme);

	//! Aquire a reference to the standard sqlite database
	DatabaseRef aquireDatabase(QObject *object) const;

	//! @private
	EmitterAdapter *createEmitter(QObject *parent = nullptr) const;
	//! @private
	QVariant cacheHandle() const;

private:
	QSharedPointer<DefaultsPrivate> d;
};

}

#endif // QTDATASYNC_DEFAULTS_H
