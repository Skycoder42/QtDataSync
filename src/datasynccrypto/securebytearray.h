#ifndef QTDATASYNC_CRYPTO_BYTEARRAY_H
#define QTDATASYNC_CRYPTO_BYTEARRAY_H

#include "QtDataSyncCrypto/qtdatasynccrypto_global.h"

#include <QtCore/qbytearray.h>

#include <cryptopp/secblock.h>

namespace QtDataSync::Crypto {

class Q_DATASYNC_CRYPTO_EXPORT SecureByteArray : public QByteArray
{
public:
	SecureByteArray() = default;
	SecureByteArray(const SecureByteArray &other) = default;
	SecureByteArray(SecureByteArray &&other) noexcept = default;
	SecureByteArray &operator=(const SecureByteArray &other) = default;
	SecureByteArray &operator=(SecureByteArray &&other) noexcept = default;

	SecureByteArray(const QByteArray &other);
	SecureByteArray(QByteArray &&other) noexcept;
	SecureByteArray &operator=(const QByteArray &other);
	SecureByteArray &operator=(QByteArray &&other) noexcept;

	SecureByteArray(const char *data, int size = -1);
	SecureByteArray(int size, char c);
	SecureByteArray(int size, Qt::Initialization);

	SecureByteArray(const CryptoPP::byte *data, size_t size);
	SecureByteArray(size_t size, CryptoPP::byte b);
	SecureByteArray(size_t size, Qt::Initialization);

	static SecureByteArray fromRawByteData(const CryptoPP::byte *data, size_t size);

	CryptoPP::byte *byteData();
	const CryptoPP::byte *byteData() const;
	const CryptoPP::byte *constByteData() const;
	size_t byteSize() const;

	operator CryptoPP::byte*();
	operator const CryptoPP::byte*() const;
};

}

#endif // QTDATASYNC_CRYPTO_BYTEARRAY_H
