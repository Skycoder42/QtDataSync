#include "cloudtransformer.h"
#include "cloudtransformer_p.h"
#include <QtCore/QUrl>
using namespace QtDataSync;

QString ICloudTransformer::escapeType(const QString &name)
{
	return QString::fromUtf8(QUrl::toPercentEncoding(name));
}

QString ICloudTransformer::unescapeType(const QString &name)
{
	return QUrl::fromPercentEncoding(name.toUtf8());
}

ObjectKey ICloudTransformer::escapeKey(const ObjectKey &key)
{
	return {
		escapeType(key.typeName),
		QLatin1Char('_') + QString::fromUtf8(QUrl::toPercentEncoding(key.id)) // prepend _ to not have arrays
	};
}

ObjectKey ICloudTransformer::unescapeKey(const ObjectKey &key)
{
	Q_ASSERT_X(key.id.startsWith(QLatin1Char('_')), Q_FUNC_INFO, "Invalid object key. id should start with an _");
	return {
		unescapeType(key.typeName),
		QUrl::fromPercentEncoding(key.id.mid(1).toUtf8())  // remove _ at beginning
	};
}

ICloudTransformer::ICloudTransformer(QObject *parent) :
	  QObject{parent}
{}

ICloudTransformer::ICloudTransformer(QObjectPrivate &dd, QObject *parent) :
	  QObject{dd, parent}
{}



void ISynchronousCloudTransformer::transformUpload(const LocalData &data)
{
	try {
		if (data.data())
			Q_EMIT transformUploadDone({escapeKey(data.key()), transformUploadSync(*data.data()), data});
		else
			Q_EMIT transformUploadDone({escapeKey(data.key()), std::nullopt, data});
	} catch (QString &error) {
		Q_EMIT transformError(data.key(), error);
	}
}

void ISynchronousCloudTransformer::transformDownload(const CloudData &data)
{
	try {
		if (data.data())
			Q_EMIT transformDownloadDone({unescapeKey(data.key()), transformDownloadSync(*data.data()), data});
		else
			Q_EMIT transformDownloadDone({unescapeKey(data.key()), std::nullopt, data});
	} catch (QString &error) {
		Q_EMIT transformError(unescapeKey(data.key()), error);
	}
}

ISynchronousCloudTransformer::ISynchronousCloudTransformer(QObject *parent) :
	  ICloudTransformer{parent}
{}

ISynchronousCloudTransformer::ISynchronousCloudTransformer(QObjectPrivate &dd, QObject *parent) :
	  ICloudTransformer{dd, parent}
{}



PlainCloudTransformer::PlainCloudTransformer(QObject *parent) :
	PlainCloudTransformer{new QtJsonSerializer::JsonSerializer{}, parent}
{}

PlainCloudTransformer::PlainCloudTransformer(QtJsonSerializer::JsonSerializer *serializer, QObject *parent) :
	ISynchronousCloudTransformer{*new PlainCloudTransformerPrivate{}, parent}
{
	Q_D(PlainCloudTransformer);
	d->serializer = serializer;
	d->serializer->setParent(this);
}

QtJsonSerializer::JsonSerializer *PlainCloudTransformer::serializer() const
{
	Q_D(const PlainCloudTransformer);
	return d->serializer;
}

void PlainCloudTransformer::setSerializer(QtJsonSerializer::JsonSerializer *serializer)
{
	Q_D(PlainCloudTransformer);
	if (d->serializer == serializer)
		return;

	if (d->serializer->parent() == this)
		d->serializer->deleteLater();
	d->serializer = serializer;
	d->serializer->setParent(this);
	Q_EMIT serializerChanged(d->serializer);
}

QJsonObject PlainCloudTransformer::transformUploadSync(const QVariantHash &data) const
{
	Q_D(const PlainCloudTransformer);
	return d->serializer->serialize(data);
}

QVariantHash PlainCloudTransformer::transformDownloadSync(const QJsonObject &data) const
{
	Q_D(const PlainCloudTransformer);
	return d->serializer->deserialize<QVariantHash>(data);
}
