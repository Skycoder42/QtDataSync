#ifndef QTDATASYNC_CRYPTO_SYMMETRICCRYPTOCLOUDTRANSFORMER_H
#define QTDATASYNC_CRYPTO_SYMMETRICCRYPTOCLOUDTRANSFORMER_H

#include "QtDataSyncCrypto/qtdatasynccrypto_global.h"
#include "QtDataSyncCrypto/qiodevicesource.h"
#include "QtDataSyncCrypto/qiodevicesink.h"

#include <QtDataSync/cloudtransformer.h>

#include <cryptopp/osrng.h>
#include <cryptopp/filters.h>
#include <cryptopp/gcm.h>
#include <cryptopp/aes.h>

namespace QtDataSync::Crypto {

class Q_DATASYNC_CRYPTO_EXPORT SymmetricCryptoCloudTransformerBase : public ICloudTransformer
{
	Q_OBJECT

public:
	explicit SymmetricCryptoCloudTransformerBase(QObject *parent = nullptr);

public Q_SLOTS:
	void transformUpload(const LocalData &data) final;
	void transformDownload(const CloudData &data) final;

protected:
	virtual QByteArray encrypt(const CryptoPP::SecByteBlock &key,
							   const CryptoPP::SecByteBlock &iv,
							   const QByteArray &plain) = 0;
	virtual QByteArray decrypt(const CryptoPP::SecByteBlock &key,
							   const CryptoPP::SecByteBlock &iv,
							   const QByteArray &cipher) = 0;

private:
	CryptoPP::AutoSeededRandomPool _rng;  // TODO move to private

	QString base64Encode(const QByteArray &data) const;
};

template <template<class> class TScheme, class TCipher>
class SymmetricCryptoCloudTransformer : public SymmetricCryptoCloudTransformerBase
{
public:
	using Cipher = TScheme<TCipher>;

protected:
	QByteArray encrypt(const CryptoPP::SecByteBlock &key, const CryptoPP::SecByteBlock &iv, const QByteArray &plain) override {
		typename Cipher::Encryption encryptor;
		encryptor.SetKeyWithIV(key, key.size(),
							   iv, iv.size());
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

	QByteArray decrypt(const CryptoPP::SecByteBlock &key, const CryptoPP::SecByteBlock &iv, const QByteArray &cipher) override {
		typename Cipher::Decryption decryptor;
		decryptor.SetKeyWithIV(key, key.size(),
							   iv, iv.size());
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
