#ifndef QTDATASYNC_KEYSTORE_H
#define QTDATASYNC_KEYSTORE_H

#include <QtCore/qobject.h>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/exception.h"

namespace QtDataSync {

class KeyStore;
//! An exception to be thrown from within a keystore if an error occurs
class Q_DATASYNC_EXPORT KeyStoreException : public Exception
{
public:
	//! Constructor that takes the store it was thrown from and an error message
	KeyStoreException(const KeyStore * const keyStore, const QString &what);

	//! The name of the type of store the exception was thrown from
	QString storeProviderName() const;

	QByteArray className() const noexcept override;
	QString qWhat() const override;
	void raise() const override;
	QException *clone() const override;

protected:
	//! @private
	KeyStoreException(const KeyStoreException * const other);

	//! @private
	const QString _keyStoreName;
};

class KeyStorePrivate;
//! An interface for a generic keystore to securely store secret cryptographic keys
class Q_DATASYNC_EXPORT KeyStore : public QObject
{
	Q_OBJECT
	friend class QtDataSync::KeyStoreException;

public:
	//! Default constructor for a specific setup
	explicit KeyStore(const Defaults &defaults, QObject *parent = nullptr);
	~KeyStore();

	//! Returns the provider type the keystore is of
	virtual QString providerName() const = 0;

	//! Checks if the store is currently open
	virtual bool isOpen() const = 0;
	//! Opens the store so the library can access it
	virtual void openStore() = 0;
	//! Closes the connection to the store
	virtual void closeStore() = 0;

	//! Checks if a key already exists in the store
	virtual bool contains(const QString &key) const = 0;
	//! Saves a secret key with the given identifier in the store
	virtual void save(const QString &key, const QByteArray &pKey) = 0;
	//! Loads a previously saved key for the given identifier from the store
	virtual QByteArray load(const QString &key) = 0;
	//! Removes the key for the given identifier from the store permanently
	virtual void remove(const QString &key) = 0;

protected:
	//! Returns the defaults of the datasync instance the store is operated from
	Defaults defaults() const;

private:
	QScopedPointer<KeyStorePrivate> d;
};

//! The keystore plugin to be implemented to provide custom keystores
class Q_DATASYNC_EXPORT KeyStorePlugin
{
public:
	virtual inline ~KeyStorePlugin() = default;

	//! Check if the keystore type identified by the provider is currently accessible
	virtual bool keystoreAvailable(const QString &provider) const = 0;
	//! Create an instance of a keystore for the given provider and parent
	virtual KeyStore *createInstance(const QString &provider, const Defaults &defaults, QObject *parent = nullptr) = 0;
};

}

//! The IID to be used to create a keystore plugin
#define QtDataSync_KeyStorePlugin_Iid "de.skycoder42.QtDataSync.KeyStorePlugin"
Q_DECLARE_INTERFACE(QtDataSync::KeyStorePlugin, QtDataSync_KeyStorePlugin_Iid)

//! @file keystore.h The Keystore header
#endif // QTDATASYNC_KEYSTORE_H
