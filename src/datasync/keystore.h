#ifndef KEYSTORE_H
#define KEYSTORE_H

#include <QtCore/qobject.h>

#include <QtNetwork/qsslkey.h>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/exception.h"

namespace QtDataSync {

class KeyStore;
class Q_DATASYNC_EXPORT KeyStoreException : public Exception
{
public:
	KeyStoreException(const KeyStore * const keyStore, const QString &what);

	QString keyStoreName() const;

	QByteArray className() const noexcept override;
	QString qWhat() const override;
	void raise() const override;
	QException *clone() const override;

protected:
	KeyStoreException(const KeyStoreException * const other);

	const QString _keyStoreName;
};

class KeyStorePrivate;
class Q_DATASYNC_EXPORT KeyStore : public QObject
{
	Q_OBJECT
	friend class QtDataSync::KeyStoreException;

public:
	explicit KeyStore(const Defaults &defaults, QObject *parent = nullptr);
	~KeyStore();

	virtual QString providerName() const = 0;

	virtual bool isOpen() const = 0;
	virtual void openStore() = 0;
	virtual void closeStore() = 0;

	virtual bool contains(const QString &key) const = 0;
	virtual void save(const QString &key, const QByteArray &pKey) = 0;
	virtual QByteArray load(const QString &key) = 0;
	virtual void remove(const QString &key) = 0;

protected:
	Defaults defaults() const;

private:
	QScopedPointer<KeyStorePrivate> d;
};

class Q_DATASYNC_EXPORT KeyStorePlugin
{
public:
	virtual inline ~KeyStorePlugin() = default;

	virtual bool keystoreAvailable(const QString &provider) const = 0;
	virtual KeyStore *createInstance(const QString &provider, const Defaults &defaults, QObject *parent = nullptr) = 0;
};

}

#define QtDataSync_KeyStorePlugin_Iid "de.skycoder42.QtDataSync.KeyStorePlugin"
Q_DECLARE_INTERFACE(QtDataSync::KeyStorePlugin, QtDataSync_KeyStorePlugin_Iid)

#endif // KEYSTORE_H
