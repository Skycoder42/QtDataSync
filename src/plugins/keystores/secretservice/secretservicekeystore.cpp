#include "secretservicekeystore.h"

#include <QtCore/QCoreApplication>

SecretServiceKeyStore::SecretServiceKeyStore(const QtDataSync::Defaults &defaults, const QString &providerName, QObject *parent) :
	KeyStore(defaults, parent),
	_providerName(providerName),
	_libSecret(new LibSecretWrapper(QCoreApplication::applicationName().toUtf8()))
{}

QString SecretServiceKeyStore::providerName() const
{
	return _providerName;
}

bool SecretServiceKeyStore::isOpen() const
{
	return _libSecret->isOpen();
}

void SecretServiceKeyStore::openStore()
{
	try {
		_libSecret->setup();
	} catch(LibSecretException &e) {
		throw QtDataSync::KeyStoreException(this, QString::fromUtf8(e.what()));
	}
}

void SecretServiceKeyStore::closeStore()
{
	_libSecret->cleanup();
}

bool SecretServiceKeyStore::contains(const QString &key) const
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

void SecretServiceKeyStore::save(const QString &key, const QByteArray &pKey)
{
	try {
		_libSecret->storeSecret(key.toUtf8(), pKey);
	} catch(LibSecretException &e) {
		throw QtDataSync::KeyStoreException(this, QString::fromUtf8(e.what()));
	}
}

QByteArray SecretServiceKeyStore::load(const QString &key)
{
	try {
		return _libSecret->loadSecret(key.toUtf8());
	} catch(LibSecretException &e) {
		throw QtDataSync::KeyStoreException(this, QString::fromUtf8(e.what()));
	}
}

void SecretServiceKeyStore::remove(const QString &key)
{
	try {
		_libSecret->removeSecret(key.toUtf8());
	} catch(LibSecretException &e) {
		throw QtDataSync::KeyStoreException(this, QString::fromUtf8(e.what()));
	}
}
