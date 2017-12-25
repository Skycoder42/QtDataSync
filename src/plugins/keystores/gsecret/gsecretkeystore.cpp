#include "gsecretkeystore.h"

#include <QtCore/QCoreApplication>

GSecretKeyStore::GSecretKeyStore(const QtDataSync::Defaults &defaults, QObject *parent) :
	KeyStore(defaults, parent),
	_libSecret(new LibSecretWrapper(QCoreApplication::applicationName().toUtf8()))
{}

QString GSecretKeyStore::providerName() const
{
	return QStringLiteral("gsecret");
}

void GSecretKeyStore::loadStore()
{
	try {
		_libSecret->setup();
	} catch(LibSecretException &e) {
		throw QtDataSync::KeyStoreException(this, QString::fromUtf8(e.what()));
	}
}

void GSecretKeyStore::closeStore()
{
	_libSecret->cleanup();
}

bool GSecretKeyStore::contains(const QString &key) const
{
	try {
		if(_libSecret->loadSecret(key.toUtf8()).isEmpty())
			return false;
		else
			return true;
	} catch (...) {
		return false;
	}
}

void GSecretKeyStore::storePrivateKey(const QString &key, const QByteArray &pKey)
{
	try {
		_libSecret->storeSecret(key.toUtf8(), pKey);
	} catch(LibSecretException &e) {
		throw QtDataSync::KeyStoreException(this, QString::fromUtf8(e.what()));
	}
}

QByteArray GSecretKeyStore::loadPrivateKey(const QString &key)
{
	try {
		return _libSecret->loadSecret(key.toUtf8());
	} catch(LibSecretException &e) {
		throw QtDataSync::KeyStoreException(this, QString::fromUtf8(e.what()));
	}
}

void GSecretKeyStore::remove(const QString &key)
{
	try {
		_libSecret->removeSecret(key.toUtf8());
	} catch(LibSecretException &e) {
		throw QtDataSync::KeyStoreException(this, QString::fromUtf8(e.what()));
	}
}
