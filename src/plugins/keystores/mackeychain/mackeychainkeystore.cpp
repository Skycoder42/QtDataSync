#include "mackeychainkeystore.h"
#include <QtCore/QCoreApplication>

#include <QtDataSync/defaults.h>

#define QTDATASYNC_LOG _logger

MacKeychainKeyStore::MacKeychainKeyStore(const QtDataSync::Defaults &defaults, QObject *parent) :
	KeyStore(defaults, parent),
	_open(false),
	_logger(defaults.createLogger("mackeychain", this))
{}

QString MacKeychainKeyStore::providerName() const
{
	return QStringLiteral("mackeychain");
}

bool MacKeychainKeyStore::isOpen() const
{
	return _open;
}

void MacKeychainKeyStore::openStore()
{
	_open = true;
}

void MacKeychainKeyStore::closeStore()
{
	_open = false;
}

bool MacKeychainKeyStore::contains(const QString &key) const
{
	auto item = loadItem(key);
	if(item) {
		CFRelease(item);
		return true;
	} else
		return false;
}

void MacKeychainKeyStore::save(const QString &key, const QByteArray &pKey)
{
	auto service = serviceName();
	auto account = key.toUtf8();
	auto ret = SecKeychainAddGenericPassword(nullptr,
											 service.size(),
											 service.constData(),
											 account.size(),
											 account.constData(),
											 pKey.size(),
											 pKey.constData(),
											 nullptr);
	if(ret == noErr)
		return;
	else if(ret == errSecDuplicateItem) {
		remove(key);
		save(key, pKey);
	} else
		throw QtDataSync::KeyStoreException(this, errorMsg(ret));
}

QByteArray MacKeychainKeyStore::load(const QString &key)
{
	auto service = serviceName();
	auto account = key.toUtf8();
	UInt32 pwLen = 0;
	char *pwData = nullptr;
	auto ret = SecKeychainFindGenericPassword(nullptr,
											  service.size(),
											  service.constData(),
											  account.size(),
											  account.constData(),
											  &pwLen,
											  reinterpret_cast<void**>(&pwData),
											  nullptr);
	if(ret == noErr) {
		QByteArray pKey(pwData, pwLen);
		ret = SecKeychainItemFreeContent(nullptr, pwData);
		if(ret != noErr) {
			logWarning().noquote() << "Failed to free password entry with error:"
								   << errorMsg(ret);
		}
		return pKey;
	} else
		throw QtDataSync::KeyStoreException(this, errorMsg(ret));
}

void MacKeychainKeyStore::remove(const QString &key)
{
	auto item = loadItem(key);
	if(item) {
		auto ret = SecKeychainItemDelete(item);
		CFRelease(item);
		if(ret != noErr)
			throw QtDataSync::KeyStoreException(this, errorMsg(ret));
	}
}

QByteArray MacKeychainKeyStore::serviceName()
{
	return QCoreApplication::applicationName().toUtf8();
}

QString MacKeychainKeyStore::errorMsg(OSStatus status)
{
	auto stringRef = SecCopyErrorMessageString(status, nullptr);
	auto res = QString::fromUtf8(CFStringGetCStringPtr(stringRef, kCFStringEncodingUTF8));
	CFRelease(stringRef);
	return res;
}

SecKeychainItemRef MacKeychainKeyStore::loadItem(const QString &key) const
{
	auto service = serviceName();
	auto account = key.toUtf8();
	SecKeychainItemRef item;
	auto ret = SecKeychainFindGenericPassword(nullptr,
											  service.size(),
											  service.constData(),
											  account.size(),
											  account.constData(),
											  0,
											  nullptr,
											  &item);
	if(ret == noErr)
		return item;
	else if(ret == errSecItemNotFound)
		return nullptr;
	else
		throw QtDataSync::KeyStoreException(this, errorMsg(ret));
}
