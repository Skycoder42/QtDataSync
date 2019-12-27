#include "cloudtransformer.h"
#include "cloudtransformer_p.h"
using namespace QtDataSync;

ICloudTransformer::ICloudTransformer(QObject *parent) :
	  QObject{parent}
{}

ICloudTransformer::ICloudTransformer(QObjectPrivate &dd, QObject *parent) :
	  QObject{dd, parent}
{}



void ISynchronousCloudTransformer::transformUpload(const LocalData &data)
{
	if (data.data())
		Q_EMIT transformUploadDone({data, transformUploadSync(*data.data())});
	else
		Q_EMIT transformUploadDone({data, std::nullopt});
}

void ISynchronousCloudTransformer::transformDownload(const CloudData &data)
{
	if (data.data())
		Q_EMIT transformDownloadDone({data, transformDownloadSync(*data.data())});
	else
		Q_EMIT transformDownloadDone({data, std::nullopt});
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
