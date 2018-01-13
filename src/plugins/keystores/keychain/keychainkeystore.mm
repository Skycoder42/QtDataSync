#include "keychainkeystore.h"
#include <QtCore/QCoreApplication>

#include <QtDataSync/defaults.h>

#include <Foundation/Foundation.h>
#include <Security/Security.h>

#define QTDATASYNC_LOG _logger

KeychainKeyStore::KeychainKeyStore(const QtDataSync::Defaults &defaults, QObject *parent) :
	KeyStore(defaults, parent),
	_open(false),
	_logger(defaults.createLogger("keychain", this))
{}

QString KeychainKeyStore::providerName() const
{
	return QStringLiteral("keychain");
}

bool KeychainKeyStore::isOpen() const
{
	return _open;
}

void KeychainKeyStore::openStore()
{
	_open = true;
}

void KeychainKeyStore::closeStore()
{
	_open = false;
}

bool KeychainKeyStore::contains(const QString &key) const
{
	NSDictionary *const query = @{
			(__bridge id) kSecClass: (__bridge id) kSecClassGenericPassword,
			(__bridge id) kSecAttrService: serviceName().toNSString(),
			(__bridge id) kSecAttrAccount: key.toNSString()
	};

	auto ret = SecItemCopyMatching((__bridge CFDictionaryRef) query, nil);
	if(ret == errSecSuccess)
		return true;
	else if(ret == errSecItemNotFound)
		return false;
	else
		throw QtDataSync::KeyStoreException(this, errorMsg(ret));
}

void KeychainKeyStore::save(const QString &key, const QByteArray &pKey)
{
	NSDictionary *const query = @{
			(__bridge id) kSecClass: (__bridge id) kSecClassGenericPassword,
			(__bridge id) kSecAttrService: serviceName().toNSString(),
			(__bridge id) kSecAttrAccount: key.toNSString()
	};

	auto ret = SecItemCopyMatching((__bridge CFDictionaryRef) query, nil);
	if(ret == errSecSuccess) {
		NSDictionary *const update = @{
				(__bridge id) kSecValueData: (__bridge NSData *) pKey.toCFData(),
		};

		ret = SecItemUpdate((__bridge CFDictionaryRef) query, (__bridge CFDictionaryRef) update);
	} else if(ret == errSecItemNotFound) {
		NSDictionary *const insert = @{
				(__bridge id) kSecClass: (__bridge id) kSecClassGenericPassword,
				(__bridge id) kSecAttrService: serviceName().toNSString(),
				(__bridge id) kSecAttrAccount: key.toNSString(),
				(__bridge id) kSecValueData: pKey.toNSData(),
		};

		ret = SecItemAdd((__bridge CFDictionaryRef) insert, nil);
	}

	if(ret != errSecSuccess)
		throw QtDataSync::KeyStoreException(this, errorMsg(ret));
}

QByteArray KeychainKeyStore::load(const QString &key)
{
	NSDictionary *const query = @{
		(__bridge id) kSecClass: (__bridge id) kSecClassGenericPassword,
			(__bridge id) kSecAttrService: serviceName().toNSString(),
			(__bridge id) kSecAttrAccount: key.toNSString(),
			(__bridge id) kSecReturnData: @YES,
	};

	CFTypeRef dataRef = nil;
	auto ret = SecItemCopyMatching((__bridge CFDictionaryRef) query, &dataRef);
	if(ret == errSecSuccess && dataRef) {
		auto pKey = QByteArray::fromCFData((CFDataRef)dataRef);
		CFRelease(dataRef);
		return pKey;
	} else
		throw QtDataSync::KeyStoreException(this, errorMsg(ret));
}

void KeychainKeyStore::remove(const QString &key)
{
	const NSDictionary *const query = @{
			(__bridge id) kSecClass: (__bridge id) kSecClassGenericPassword,
			(__bridge id) kSecAttrService: serviceName().toNSString(),
			(__bridge id) kSecAttrAccount: key.toNSString(),
	};

	auto ret = SecItemDelete((__bridge CFDictionaryRef) query);
	if(ret != errSecSuccess)
		throw QtDataSync::KeyStoreException(this, errorMsg(ret));
}

QString KeychainKeyStore::serviceName()
{
	return QCoreApplication::applicationName();
}

QString KeychainKeyStore::errorMsg(OSStatus status)
{
#ifdef Q_OS_MACOS
	auto stringRef = SecCopyErrorMessageString(status, nullptr);
	auto res = QString::fromUtf8(CFStringGetCStringPtr(stringRef, kCFStringEncodingUTF8));
	CFRelease(stringRef);
	return res;
#else
	return QStringLiteral("Keychain error code: %1").arg(status);
#endif
}
