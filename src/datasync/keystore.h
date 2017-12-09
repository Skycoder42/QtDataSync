#ifndef KEYSTORE_H
#define KEYSTORE_H

#include <QtCore/qobject.h>

#include <QtNetwork/qsslkey.h>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT KeyStore : public QObject
{
	Q_OBJECT

public:
	explicit KeyStore(QObject *parent = nullptr);

	static QStringList listProviders();
	static QString defaultProvider();

	virtual bool loadStore() = 0;
	virtual void closeStore() = 0;

	virtual bool contains(const QString &key) const = 0;
	virtual bool storePrivateKey(const QString &key, const QSslKey &pKey) = 0;
	virtual QSslKey loadPrivateKey(const QString &key) = 0;
	virtual bool remove(const QString &key) = 0;
};

class Q_DATASYNC_EXPORT KeyStorePlugin
{
public:
	virtual inline ~KeyStorePlugin() = default;

	virtual KeyStore *createInstance(const QString &provider, QObject *parent = nullptr) = 0;
};

}

#define QtDataSync_KeyStorePlugin_Iid "de.skycoder42.QtDataSync.KeyStorePlugin"
Q_DECLARE_INTERFACE(QtDataSync::KeyStorePlugin, QtDataSync_KeyStorePlugin_Iid)

#endif // KEYSTORE_H
