#include "cryptoqq.h"
#include "qiodevicesink.h"
#include "qiodevicesource.h"
using namespace CryptoQQ;

QSslKey CryptoQQ::toQtFormat(const CryptoPP::RSA::PrivateKey &pKey)
{
	QByteArray data;
	QByteArraySink sink(data);
	pKey.DEREncodePrivateKey(sink);
	return QSslKey(data, QSsl::Rsa, QSsl::Der);
}

CryptoPP::RSA::PrivateKey CryptoQQ::fromQtFormat(const QSslKey &key)
{
	CryptoPP::RSA::PrivateKey pKey;
	QByteArraySource source(key.toDer(), true);
	pKey.BERDecodePrivateKey(source, false, 0);
	return pKey;
}
