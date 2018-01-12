#include "wincredkeystore.h"

#include <QCoreApplication>
#include <qt_windows.h>
#include <wincred.h>

WinCredKeyStore::WinCredKeyStore(const QtDataSync::Defaults &defaults, QObject *parent) :
	KeyStore(defaults, parent),
	_open(false)
{}

QString WinCredKeyStore::providerName() const
{
	return QStringLiteral("wincred");
}

bool WinCredKeyStore::isOpen() const
{
	return _open;
}

void WinCredKeyStore::openStore()
{
	_open = true;
}

void WinCredKeyStore::closeStore()
{
	_open = false;
}

bool WinCredKeyStore::contains(const QString &key) const
{
	auto res = tryLoad(key);
	if(res.isEmpty()) {
		auto error = GetLastError();
		if(error == ERROR_NOT_FOUND)
			return false;
		else
			throw QtDataSync::KeyStoreException(this, formatWinError(error));
	} else
		return true;
}

void WinCredKeyStore::save(const QString &key, const QByteArray &pKey)
{
	CREDENTIALW cred;
	auto fullName = fullKey(key);

	ZeroMemory(&cred, sizeof(CREDENTIALW));
	cred.Comment = const_cast<wchar_t*>(L"QtDataSync private key");
	cred.Type = CRED_TYPE_GENERIC;
	cred.TargetName = reinterpret_cast<LPWSTR>(const_cast<ushort*>(fullName.utf16()));
	cred.CredentialBlobSize = pKey.size();
	cred.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char*>(pKey.data()));
	cred.Persist = CRED_PERSIST_LOCAL_MACHINE;

	if(!CredWriteW(&cred, 0))
		throw QtDataSync::KeyStoreException(this, formatWinError(GetLastError()));
}

QByteArray WinCredKeyStore::load(const QString &key)
{
	auto res = tryLoad(key);
	if(res.isEmpty())
		throw QtDataSync::KeyStoreException(this, formatWinError(GetLastError()));
	else
		return res;
}

void WinCredKeyStore::remove(const QString &key)
{
	auto fullName = fullKey(key);
	if(!CredDeleteW(reinterpret_cast<LPWSTR>(const_cast<ushort*>(fullName.utf16())), CRED_TYPE_GENERIC, 0))
		throw QtDataSync::KeyStoreException(this, formatWinError(GetLastError()));
}

QString WinCredKeyStore::fullKey(const QString &key)
{
	return QStringLiteral("%1_QtDataSync_%2")
			.arg(QCoreApplication::applicationName())
			.arg(key);
}

QString WinCredKeyStore::formatWinError(ulong error)
{
	wchar_t *buffer = nullptr;
	auto num = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
							  NULL,
							  error,
							  0,
							  reinterpret_cast<LPWSTR>(&buffer),
							  0,
							  NULL);
	if(buffer) {
		auto res = QString::fromWCharArray(buffer, num);
		LocalFree(buffer);
		return res;
	} else
		return {};
}

QByteArray WinCredKeyStore::tryLoad(const QString &key) const
{
	PCREDENTIALW pCred = NULL;
	auto fullName = fullKey(key);

	if(CredReadW(reinterpret_cast<LPWSTR>(const_cast<ushort*>(fullName.utf16())), CRED_TYPE_GENERIC, 0, &pCred)) {
		QByteArray res(reinterpret_cast<const char*>(pCred->CredentialBlob), pCred->CredentialBlobSize);
		CredFree(pCred);
		return res;
	} else
		return QByteArray();
}
