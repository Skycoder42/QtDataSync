#ifndef QTDATASYNC_CRYPTO_SYMMETRICCRYPTOCLOUDTRANSFORMER_H
#define QTDATASYNC_CRYPTO_SYMMETRICCRYPTOCLOUDTRANSFORMER_H

#include "QtDataSyncCrypto/qtdatasynccrypto_global.h"
#include "QtDataSyncCrypto/securebytearray.h"
#include "QtDataSyncCrypto/qiodevicesource.h"
#include "QtDataSyncCrypto/qiodevicesink.h"

#include <QtDataSync/cloudtransformer.h>

#include <cryptopp/filters.h>
#include <cryptopp/gcm.h>
#include <cryptopp/eax.h>
#include <cryptopp/aes.h>

namespace QtDataSync::Crypto {

class SymmetricCryptoCloudTransformerBasePrivate;
class Q_DATASYNC_CRYPTO_EXPORT SymmetricCryptoCloudTransformerBase : public ISynchronousCloudTransformer
{
	Q_OBJECT

public:
	explicit SymmetricCryptoCloudTransformerBase(QObject *parent = nullptr);

protected:
	QJsonObject transformUploadSync(const QVariantHash &data) const final;
	QVariantHash transformDownloadSync(const QJsonObject &data) const final;

	virtual size_t ivSize() const = 0;
	virtual QByteArray encrypt(const SecureByteArray &key,
							   const SecureByteArray &iv,
							   const QByteArray &plain) const = 0;
	virtual QByteArray decrypt(const SecureByteArray &key,
							   const SecureByteArray &iv,
							   const QByteArray &cipher) const = 0;

private:
	Q_DECLARE_PRIVATE(SymmetricCryptoCloudTransformerBase)
};

template <template<class> class TScheme, class TCipher>
class SymmetricCryptoCloudTransformer : public SymmetricCryptoCloudTransformerBase
{
public:
	using Cipher = TScheme<TCipher>;

protected:
	size_t ivSize() const override {
		typename Cipher::Encryption e;
		return e.IVSize();
	}

	QByteArray encrypt(const SecureByteArray &key, const SecureByteArray &iv, const QByteArray &plain) const override {
		typename Cipher::Encryption encryptor;
		encryptor.SetKeyWithIV(key.data(), key.size(),
							   iv.data(), iv.size());
		QByteArray cipher;
		QByteArraySource {
			plain, true, new CryptoPP::AuthenticatedDecryptionFilter {
				encryptor, new QByteArraySink {
					cipher
				}
			}
		};
		return cipher;
	}

	QByteArray decrypt(const SecureByteArray &key, const SecureByteArray &iv, const QByteArray &cipher) const override {
		typename Cipher::Decryption decryptor;
		decryptor.SetKeyWithIV(key.data(), key.size(),
							   iv.data(), iv.size());
		QByteArray plain;
		QByteArraySource {
			cipher, true, new CryptoPP::AuthenticatedDecryptionFilter {
				decryptor, new QByteArraySink {
					plain
				}
			}
		};
		return plain;
	}
};

}

#endif // QTDATASYNC_CRYPTO_SYMMETRICCRYPTOCLOUDTRANSFORMER_H
