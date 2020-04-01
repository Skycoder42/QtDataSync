#include "symmetriccryptocloudtransformer.h"
#include "symmetriccryptocloudtransformer_p.h"
#include <QtCore/QDataStream>
using namespace QtDataSync;
using namespace QtDataSync::Crypto;

SymmetricCryptoCloudTransformerBase::SymmetricCryptoCloudTransformerBase(QObject *parent) :
	ICloudTransformer{*new SymmetricCryptoCloudTransformerBasePrivate{}, parent}
{}

void SymmetricCryptoCloudTransformerBase::transformUpload(const LocalData &data)
{
	Q_D(const SymmetricCryptoCloudTransformerBase);
	if (!data.data()) {
		Q_EMIT transformUploadDone(CloudData {
			escapeKey(data.key()),
			std::nullopt,
			data
		});
		return;
	}

	d->keyProvider.loadKey([this, data](const SecureByteArray &key) {
		try {
			Q_EMIT transformUploadDone(CloudData {
				escapeKey(data.key()),
				transformUploadImpl(key, data.data().value()),
				data
			});
		} catch (std::exception &e) {
			// TODO log error
			Q_EMIT transformError(data.key());
		}
	}, [this, key = data.key()]() {
		Q_EMIT transformError(key);
	});
}

void SymmetricCryptoCloudTransformerBase::transformDownload(const CloudData &data)
{
	Q_D(const SymmetricCryptoCloudTransformerBase);
	if (!data.data()) {
		Q_EMIT transformDownloadDone(LocalData {
			unescapeKey(data.key()),
			std::nullopt,
			data
		});
		return;
	}

	d->keyProvider.loadKey([this, data](const SecureByteArray &key) {
		try {
			Q_EMIT transformDownloadDone(LocalData {
				unescapeKey(data.key()),
				transformDownloadImpl(key, data.data().value()),
				data
			});
		} catch (std::exception &e) {
			// TODO log error
			Q_EMIT transformError(unescapeKey(data.key()));
		}
	}, [this, key = unescapeKey(data.key())]() {
		Q_EMIT transformError(key);
	});
}

QJsonObject SymmetricCryptoCloudTransformerBase::transformUploadImpl(const SecureByteArray &key, const QVariantHash &data) const
{
	Q_D(const SymmetricCryptoCloudTransformerBase);
	QByteArray plain;
	QDataStream stream{&plain, QIODevice::WriteOnly};
	stream << data;

	SecureByteArray iv{ivSize()};
	d->rng.GenerateBlock(iv.data(), iv.size());
	return QJsonObject {
		{QStringLiteral("version"), stream.version()},
		{QStringLiteral("salt"), d->base64Encode(iv.toRaw())},  //  TODO store IV
		{QStringLiteral("data"), d->base64Encode(encrypt(key, iv, plain))},
	};
}

QVariantHash SymmetricCryptoCloudTransformerBase::transformDownloadImpl(const SecureByteArray &key, const QJsonObject &data) const
{
	Q_D(const SymmetricCryptoCloudTransformerBase);

	const SecureByteArray iv = SecureByteArray::fromRaw(d->base64Decode(data[QStringLiteral("salt")]));
	const auto plain = decrypt(key, iv, d->base64Decode(data[QStringLiteral("data")]));

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

QByteArray SymmetricCryptoCloudTransformerBasePrivate::base64Decode(const QJsonValue &data) const
{
	return base64Decode(data.toString());
}
