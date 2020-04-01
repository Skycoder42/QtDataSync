#ifndef QTDATASYNC_CRYPTO_BYTEARRAY_H
#define QTDATASYNC_CRYPTO_BYTEARRAY_H

#include "QtDataSyncCrypto/qtdatasynccrypto_global.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qshareddata.h>

#include <cryptopp/secblock.h>

namespace QtDataSync::Crypto {

class SecureByteArrayData;
class Q_DATASYNC_CRYPTO_EXPORT SecureByteArray
{
public:
	SecureByteArray();
	SecureByteArray(const SecureByteArray &other);
	SecureByteArray(SecureByteArray &&other) noexcept;
	SecureByteArray &operator=(const SecureByteArray &other);
	SecureByteArray &operator=(SecureByteArray &&other) noexcept;
	~SecureByteArray();

	SecureByteArray(size_t size);
	SecureByteArray(const CryptoPP::byte *data, size_t size);
	SecureByteArray(const char *data, int size);

	bool isValid() const;
	explicit operator bool() const;
	bool operator!() const;

	size_t size() const;
	const CryptoPP::byte *constData() const;
	const CryptoPP::byte *data() const;
	CryptoPP::byte *data();

	QByteArray toRaw() const;
	static SecureByteArray fromRaw(const QByteArray &data);

private:
	QSharedDataPointer<SecureByteArrayData> d;
};

}

#endif // QTDATASYNC_CRYPTO_BYTEARRAY_H
