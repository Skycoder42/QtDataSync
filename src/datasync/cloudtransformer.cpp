#include "cloudtransformer.h"
#include "cloudtransformer_p.h"
using namespace QtDataSync;

ICloudTransformer::ICloudTransformer(QObject *parent) :
	QObject{parent}
{}

ICloudTransformer::ICloudTransformer(QObjectPrivate &dd, QObject *parent) :
	QObject{dd, parent}
{}

CloudData ICloudTransformer::transform(ObjectKey key, const std::optional<QVariantHash> &data, QDateTime modified) const
{
	return CloudData {
		std::move(key),
		data ? transformUpload(*data) : std::optional<QJsonObject>{std::nullopt},
		std::move(modified)
	};
}

std::optional<QVariantHash> ICloudTransformer::transform(const CloudData &data)
{
	return data.data() ?
		transformDownload(*data.data()) :
		std::optional<QVariantHash>{std::nullopt};
}



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



PlainCloudTransformer::PlainCloudTransformer(QObject *parent) :
	PlainCloudTransformer{new QtJsonSerializer::JsonSerializer{}, parent}
{}

PlainCloudTransformer::PlainCloudTransformer(QtJsonSerializer::JsonSerializer *serializer, QObject *parent) :
	ICloudTransformer{*new PlainCloudTransformerPrivate{}, parent}
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

QJsonObject PlainCloudTransformer::transformUpload(const QVariantHash &data) const
{
	Q_D(const PlainCloudTransformer);
	return d->serializer->serialize(data);
}

QVariantHash PlainCloudTransformer::transformDownload(const QJsonObject &data) const
{
	Q_D(const PlainCloudTransformer);
	return d->serializer->deserialize<QVariantHash>(data);
}
