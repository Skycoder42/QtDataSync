#ifndef KEYSTORE_H
#define KEYSTORE_H

#include <QtCore/qobject.h>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT KeyStore : public QObject
{
	Q_OBJECT

public:
	explicit KeyStore(QObject *parent = nullptr);

	static QStringList listProviders();
	static QString defaultProvider();

	virtual bool canCheckContains() const = 0;
	virtual bool contains(const QByteArray &key) const;

public Q_SLOTS:
	virtual void storeSecret(const QByteArray &key, const QByteArray &secret) = 0;
	virtual void loadSecret(const QByteArray &key) = 0;

Q_SIGNALS:
	void secretStored();
	void secretLoaded(const QByteArray &secret);
	void secretNotExistant();
	void storeError(const QString &error);
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
