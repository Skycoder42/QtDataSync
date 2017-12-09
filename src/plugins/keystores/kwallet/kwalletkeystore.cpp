#include "kwalletkeystore.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

KWalletKeyStore::KWalletKeyStore(QObject *parent) :
	KeyStore(parent),
	_wallet(nullptr)
{}

bool KWalletKeyStore::loadStore()
{
	if(!_wallet) {
		_wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 0);
		if(_wallet) {
			if(!_wallet->isOpen()) {
				qWarning() << "KWallet not open";
				_wallet->deleteLater();
				return false;
			} else {
				auto sDir = QCoreApplication::applicationName();
				if(!_wallet->hasFolder(sDir))
					_wallet->createFolder(sDir);

				if(!_wallet->setFolder(sDir)) {
					qWarning() << "Failed to enter application folder in KWallet";
					_wallet->deleteLater();
					return false;
				}
			}
		}
	}

	return _wallet;
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
	if(_wallet)
		return _wallet->hasEntry(key);
	else
		return false;
}

bool KWalletKeyStore::storePrivateKey(const QString &key, const QSslKey &pkey)
{
	if(!_wallet)
		return false;
	else {
		auto res = _wallet->writeEntry(key, pkey.toDer());
		return (res == 0);
	}
}

QSslKey KWalletKeyStore::loadPrivateKey(const QString &key)
{
	if(!_wallet)
		return {};
	else {
		if(!_wallet->hasEntry(key))
			return {};
		else {
			QByteArray secret;
			auto res = _wallet->readEntry(key, secret);
			if(res == 0)
				return QSslKey(secret, QSsl::Rsa, QSsl::Der);
			else
				return {};
		}
	}
}

bool KWalletKeyStore::remove(const QString &key)
{
	if(!_wallet)
		return false;
	else {
		auto res = _wallet->removeEntry(key);
		return (res == 0);
	}
}
