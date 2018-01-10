#include "kwalletkeystore.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include <QtDataSync/defaults.h>

KWalletKeyStore::KWalletKeyStore(const QtDataSync::Defaults &defaults, QObject *parent) :
	KeyStore(defaults, parent),
	_wallet(nullptr)
{}

QString KWalletKeyStore::providerName() const
{
	return QStringLiteral("kwallet");
}

bool KWalletKeyStore::isOpen() const
{
	return _wallet;
}

void KWalletKeyStore::openStore()
{
	if(!_wallet) {
		_wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 0);
		if(_wallet) {
			if(!_wallet->isOpen()) {
				_wallet->deleteLater();
				_wallet = nullptr;
				throw QtDataSync::KeyStoreException(this, QStringLiteral("Failed to open KWallet instance"));
			} else {
				auto sDir = QCoreApplication::applicationName();
				if(!_wallet->hasFolder(sDir))
					_wallet->createFolder(sDir);

				if(!_wallet->setFolder(sDir)) {
					_wallet->deleteLater();
					_wallet = nullptr;
					throw QtDataSync::KeyStoreException(this, QStringLiteral("Failed to enter application folder in KWallet"));
				}
			}
		} else
			throw QtDataSync::KeyStoreException(this, QStringLiteral("Failed to acquire KWallet instance"));
	}
}

void KWalletKeyStore::closeStore()
{
	if(_wallet) {
		_wallet->sync();
		_wallet->deleteLater();
		_wallet = nullptr;
	}
}

bool KWalletKeyStore::contains(const QString &key) const
{
	return _wallet->hasEntry(key);
}

void KWalletKeyStore::save(const QString &key, const QByteArray &pkey)
{
	if(_wallet->writeEntry(key, pkey) != 0)
		throw QtDataSync::KeyStoreException(this, QStringLiteral("Failed to save key to KWallet"));
}

QByteArray KWalletKeyStore::load(const QString &key)
{
	QByteArray secret;
	auto res = _wallet->readEntry(key, secret);
	if(res == 0)
		return secret;
	else
		throw QtDataSync::KeyStoreException(this, QStringLiteral("Failed to load key from KWallet"));
}

void KWalletKeyStore::remove(const QString &key)
{
	if(_wallet->removeEntry(key) != 0)
		throw QtDataSync::KeyStoreException(this, QStringLiteral("Failed to remove key from KWallet"));
}
