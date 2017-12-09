#include "qsigndevice_p.h"
#include <qiodevicesource.h>
#include <qiodevicesink.h>
using namespace QtDataSync;
using namespace CryptoPP;

QSignDevice::QSignDevice(const RSA::PrivateKey &key, RandomNumberGenerator *rng, QObject *parent) :
	QPipeDevice(parent),
	_key(key),
	_rng(rng)
{}

QByteArray QSignDevice::process(QByteArray &&data)
{
	_buffer.append(data);
	return data;
}

void QSignDevice::end()
{
	//create signature
	RsaScheme::Signer signer(_key);

	QByteArray signature;
	QByteArraySource (_buffer, true,
		new SignerFilter(*_rng, signer,
			new QByteArraySink(signature)
	   ) // SignerFilter
	); // QByteArraySource

	sink()->write(signature);
}
