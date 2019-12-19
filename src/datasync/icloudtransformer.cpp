#include "icloudtransformer.h"
#include "icloudtransformer_p.h"
using namespace QtDataSync;

ICloudTransformer::ICloudTransformer() = default;

ICloudTransformer::~ICloudTransformer() = default;



CloudData::CloudData() :
	d{new CloudDataData{}}
{}

CloudData::CloudData(ObjectKey key, QJsonObject data, QDateTime modified) :
	d{new CloudDataData{{}, std::move(key), std::move(data), std::move(modified)}}
{}

CloudData::CloudData(QByteArray type, QString key, QJsonObject data, QDateTime modified) :
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

QJsonObject CloudData::data() const
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

void CloudData::setData(QJsonObject data)
{
	d->data = std::move(data);
}

void CloudData::setModified(QDateTime modified)
{
	d->modified = std::move(modified);
}
