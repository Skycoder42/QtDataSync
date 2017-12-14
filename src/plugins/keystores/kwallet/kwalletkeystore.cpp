#include "kwalletkeystore.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include <QtDataSync/defaults.h>

KWalletKeyStore::KWalletKeyStore(const QtDataSync::Defaults &defaults, QObject *parent) :
	KeyStore(defaults, parent),
	_wallet(nullptr)
{}

void KWalletKeyStore::loadStore()
{
	if(!_wallet) {
		_wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 0);
		if(_wallet) {
			if(!_wallet->isOpen()) {
				_wallet->deleteLater();
				throw QtDataSync::KeyStoreException(defaults(),
													QStringLiteral("kwallet"),
													QStringLiteral("Failed to open KWallet instance"));
			} else {
				auto sDir = QCoreApplication::applicationName();
				if(!_wallet->hasFolder(sDir))
					_wallet->createFolder(sDir);

				if(!_wallet->setFolder(sDir)) {
					_wallet->deleteLater();
					throw QtDataSync::KeyStoreException(defaults(),
														QStringLiteral("kwallet"),
														QStringLiteral("Failed to enter application folder in KWallet"));
				}
			}
		} else {
			throw QtDataSync::KeyStoreException(defaults(),
												QStringLiteral("kwallet"),
												QStringLiteral("Failed to acquire KWallet instance"));
		}
	}
}

void KWalletKeyStore::closeStore()
{
	if(_wallet) {
		_wallet->sync();
		_wallet->deleteLater();
	}
}

bool KWalletKeyStore::contains(const QString &key) const
{
	return _wallet->hasEntry(key);
}

void KWalletKeyStore::storePrivateKey(const QString &key, const QSslKey &pkey)
{
	if(_wallet->writeEntry(key, pkey.toDer()) != 0) {
		throw QtDataSync::KeyStoreException(defaults(),
											QStringLiteral("kwallet"),
											QStringLiteral("Failed to save key to KWallet"));
	}
}

QSslKey KWalletKeyStore::loadPrivateKey(const QString &key)
{
	QByteArray secret;
	auto res = _wallet->readEntry(key, secret);
	if(res == 0)
		return QSslKey(secret, QSsl::Rsa, QSsl::Der);//TODO dont assume RSA
	else {
		throw QtDataSync::KeyStoreException(defaults(),
											QStringLiteral("kwallet"),
											QStringLiteral("Failed to load key from KWallet"));
	}
}

void KWalletKeyStore::remove(const QString &key)
{
	if(_wallet->removeEntry(key) != 0) {
		throw QtDataSync::KeyStoreException(defaults(),
											QStringLiteral("kwallet"),
											QStringLiteral("Failed to remove key from KWallet"));
	}
}
