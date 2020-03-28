#include "symmetriccryptocloudtransformer.h"
using namespace QtDataSync;
using namespace QtDataSync::Crypto;

void SymmetricCryptoCloudTransformerBase::transformUpload(const LocalData &data)
{
	try {
		CryptoPP::SecByteBlock key;  // TODO init
		CryptoPP::SecByteBlock iv;  // TODO init
		_rng.GenerateBlock(iv, iv.size());
		QByteArray plain;  // TODO get plain
		const auto cipher = encrypt(key, iv, plain);
		Q_EMIT transformUploadDone(CloudData {
			escapeKey(data.key()),
			QJsonObject {
				{QStringLiteral("salt"), QString{}},  //  TODO store IV
				{QStringLiteral("data"), base64Encode(cipher)},
			},
			data
		});
	} catch (CryptoPP::Exception &e) {
		// TODO log exception
		Q_EMIT transformError(data.key());
	}
}

void SymmetricCryptoCloudTransformerBase::transformDownload(const CloudData &data)
{

}

QString SymmetricCryptoCloudTransformerBase::base64Encode(const QByteArray &data) const
{
	return QString::fromUtf8(data.toBase64());
}
