#include "cloudtransformer.h"
#include "cloudtransformer_p.h"
using namespace QtDataSync;

CloudData::CloudData() :
	d{new CloudDataData{}}
{}

CloudData::CloudData(ObjectKey key, std::optional<QJsonObject> data, QDateTime modified) :
	d{new CloudDataData{{}, std::move(key), std::move(data), std::move(modified)}}
{}

CloudData::CloudData(QString type, QString key, std::optional<QJsonObject> data, QDateTime modified) :
	CloudData{{std::move(type), std::move(key)}, std::move(data), std::move(modified)}
{}

CloudData::CloudData(const CloudData &other) = default;

CloudData::CloudData(CloudData &&other) noexcept = default;

CloudData &CloudData::operator=(const CloudData &other) = default;

CloudData &CloudData::operator=(CloudData &&other) noexcept = default;

CloudData::~CloudData() = default;

ObjectKey CloudData::key() const
{
	return d->key;
}

std::optional<QJsonObject> CloudData::data() const
{
	return d->data;
}

QDateTime CloudData::modified() const
{
	return d->modified;
}

void CloudData::setKey(ObjectKey key)
{
	d->key = std::move(key);
}

void CloudData::setData(std::optional<QJsonObject> data)
{
	d->data = std::move(data);
}

void CloudData::setModified(QDateTime modified)
{
	d->modified = std::move(modified);
}



ICloudTransformer::ICloudTransformer(QObject *parent) :
	  QObject{parent}
{}

ICloudTransformer::ICloudTransformer(QObjectPrivate &dd, QObject *parent) :
	  QObject{dd, parent}
{}



void ISynchronousCloudTransformer::transformUpload(ObjectKey key, const std::optional<QVariantHash> &data, QDateTime modified)
{
	if (data)
		Q_EMIT transformUploadDone({std::move(key), transformUploadSync(*data), std::move(modified)});
	else
		Q_EMIT transformUploadDone({std::move(key), std::nullopt, std::move(modified)});
}

void ISynchronousCloudTransformer::transformDownload(const CloudData &data)
{
	if (data.data())
		Q_EMIT transformDownloadDone(data.key(), transformDownloadSync(*data.data()), data.modified());
	else
		Q_EMIT transformDownloadDone(data.key(), std::nullopt, data.modified());
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
