#include "symmetriccryptocloudtransformer.h"
#include "symmetriccryptocloudtransformer_p.h"
#include <QtCore/QDataStream>
using namespace QtDataSync;
using namespace QtDataSync::Crypto;

SymmetricCryptoCloudTransformerBase::SymmetricCryptoCloudTransformerBase(QObject *parent) :
	ISynchronousCloudTransformer{*new SymmetricCryptoCloudTransformerBasePrivate{}, parent}
{}

QJsonObject SymmetricCryptoCloudTransformerBase::transformUploadSync(const QVariantHash &data) const
{
	Q_D(const SymmetricCryptoCloudTransformerBase);
	QByteArray plain;
	QDataStream stream{&plain, QIODevice::WriteOnly};
	stream << data;

	SecureByteArray key;  // TODO get key
	SecureByteArray iv{ivSize(), Qt::Uninitialized};
	d->rng.GenerateBlock(iv, iv.size());
	return QJsonObject {
		{QStringLiteral("version"), stream.version()},
		{QStringLiteral("salt"), d->base64Encode(iv)},  //  TODO store IV
		{QStringLiteral("data"), d->base64Encode(encrypt(key, iv, plain))},
	};
}

QVariantHash SymmetricCryptoCloudTransformerBase::transformDownloadSync(const QJsonObject &data) const
{
	Q_D(const SymmetricCryptoCloudTransformerBase);

	SecureByteArray key;  // TODO get key
	const SecureByteArray iv = d->base64Decode(data[QStringLiteral("salt")].toString());
	const auto plain = decrypt(key, iv, d->base64Decode(data[QStringLiteral("data")].toString()));

	QVariantHash result;
	QDataStream stream{plain};
	stream.setVersion(data[QStringLiteral("version")].toInt());
	stream.startTransaction();
	stream >> result;
	if (stream.commitTransaction())
		return result;
	else
		throw nullptr; // TODO real exception
}

QString SymmetricCryptoCloudTransformerBasePrivate::base64Encode(const QByteArray &data) const
{
	return QString::fromUtf8(data.toBase64(QByteArray::Base64Encoding | QByteArray::KeepTrailingEquals));
}

QByteArray SymmetricCryptoCloudTransformerBasePrivate::base64Decode(const QString &data) const
{
	return QByteArray::fromBase64(data.toUtf8(), QByteArray::Base64Encoding);
}
