#ifndef QTDATASYNC_ICLOUDTRANSFORMER_H
#define QTDATASYNC_ICLOUDTRANSFORMER_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/datasetid.h"

#include <optional>

#include <QtCore/qobject.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qversionnumber.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qshareddata.h>

namespace QtDataSync {

namespace __private {
template <typename T>
struct SyncDataData;
}

template <typename T>
class SyncData
{
public:
	SyncData();
	SyncData(DatasetId key,
			 std::optional<T> data,
			 QDateTime modified,
			 std::optional<QVersionNumber> version,
			 QDateTime uploaded = {});
	SyncData(QString type,
			 QString key,
			 std::optional<T> data,
			 QDateTime modified,
			 std::optional<QVersionNumber> version,
			 QDateTime uploaded = {});
	SyncData(const SyncData &other) = default;
	SyncData(SyncData &&other) noexcept = default;
	SyncData &operator=(const SyncData &other) = default;
	SyncData &operator=(SyncData &&other) noexcept = default;
	~SyncData() = default;

	template <typename TOther>
	SyncData(DatasetId key, std::optional<T> data, const SyncData<TOther> &other);

	bool operator==(const SyncData &other) const;
	bool operator!=(const SyncData &other) const;

	DatasetId key() const;
	std::optional<T> data() const;
	QDateTime modified() const;
	std::optional<QVersionNumber> version() const;
	QDateTime uploaded() const;

	void setKey(DatasetId key);
	void setData(std::optional<T> data);
	void setModified(QDateTime modified);
	void setVersion(std::optional<QVersionNumber> version);
	void setUploaded(QDateTime uploaded);

private:
	QSharedDataPointer<__private::SyncDataData<T>> d;
};

using LocalData = SyncData<QVariantHash>;
using CloudData = SyncData<QJsonObject>;

class Q_DATASYNC_EXPORT ICloudTransformer : public QObject
{
	Q_OBJECT

public:
	virtual QString escapeType(const QString &name);
	virtual QString unescapeType(const QString &name);
	virtual DatasetId escapeKey(const DatasetId &key);
	virtual DatasetId unescapeKey(const DatasetId &key);

public Q_SLOTS:
	virtual void transformUpload(const QtDataSync::LocalData &data) = 0;
	virtual void transformDownload(const QtDataSync::CloudData &data) = 0;

Q_SIGNALS:
	void transformUploadDone(const QtDataSync::CloudData &data);
	void transformDownloadDone(const QtDataSync::LocalData &data);
	void transformError(const QtDataSync::DatasetId &key);

protected:
	explicit ICloudTransformer(QObject *parent = nullptr);
	ICloudTransformer(QObjectPrivate &dd, QObject *parent = nullptr);
};

class Q_DATASYNC_EXPORT ISynchronousCloudTransformer : public ICloudTransformer
{
public:
	void transformUpload(const LocalData &data) final;
	void transformDownload(const CloudData &data) final;

protected:
	explicit ISynchronousCloudTransformer(QObject *parent = nullptr);
	ISynchronousCloudTransformer(QObjectPrivate &dd, QObject *parent = nullptr);

	virtual QJsonObject transformUploadSync(const QVariantHash &data) const = 0;
	virtual QVariantHash transformDownloadSync(const QJsonObject &data) const = 0;
};

class PlainCloudTransformerPrivate;
class Q_DATASYNC_EXPORT PlainCloudTransformer : public ISynchronousCloudTransformer
{
	Q_OBJECT

public:
	explicit PlainCloudTransformer(QObject *parent = nullptr);

protected:
	QJsonObject transformUploadSync(const QVariantHash &data) const override;
	QVariantHash transformDownloadSync(const QJsonObject &data) const override;

private:
	Q_DECLARE_PRIVATE(PlainCloudTransformer)
};

// ------------- private implementation -------------

namespace __private {

template <typename T>
struct SyncDataData : public QSharedData {
	DatasetId key;
	std::optional<T> data = std::nullopt;
	QDateTime modified;
	std::optional<QVersionNumber> version = std::nullopt;
	QDateTime uploaded;
};

}

// ------------- generic implementation -------------

template <typename T>
SyncData<T>::SyncData() :
	d{new __private::SyncDataData<T>{}}
{}

template <typename T>
SyncData<T>::SyncData(DatasetId key, std::optional<T> data, QDateTime modified, std::optional<QVersionNumber> version, QDateTime uploaded) :
	d{new __private::SyncDataData<T>{{}, std::move(key), std::move(data), std::move(modified), std::move(version), std::move(uploaded)}}
{}

template <typename T>
SyncData<T>::SyncData(QString type, QString key, std::optional<T> data, QDateTime modified, std::optional<QVersionNumber> version, QDateTime uploaded) :
	SyncData{{std::move(type), std::move(key)}, std::move(data), std::move(modified), std::move(version), std::move(uploaded)}
{}

template <typename T>
template<typename TOther>
SyncData<T>::SyncData(DatasetId key, std::optional<T> data, const SyncData<TOther> &other) :
	SyncData{std::move(key), std::move(data), other.modified(), other.version(), other.uploaded()}
{}

template<typename T>
bool SyncData<T>::operator==(const SyncData &other) const
{
	return d == other.d || (
			   d->key == other.d->key &&
			   d->data == other.d->data &&
			   d->modified == other.d->modified &&
			   d->version == other.d->version &&
			   d->uploaded == other.d->uploaded);
}

template<typename T>
bool SyncData<T>::operator!=(const SyncData &other) const
{
	return d != other.d && (
			   d->key != other.d->key ||
			   d->data != other.d->data ||
			   d->modified != other.d->modified ||
			   d->version != other.d->version ||
			   d->uploaded != other.d->uploaded);
}

template <typename T>
DatasetId SyncData<T>::key() const
{
	return d->key;
}

template <typename T>
std::optional<T> SyncData<T>::data() const
{
	return d->data;
}

template <typename T>
QDateTime SyncData<T>::modified() const
{
	return d->modified;
}

template<typename T>
std::optional<QVersionNumber> SyncData<T>::version() const
{
	return d->version;
}

template<typename T>
QDateTime SyncData<T>::uploaded() const
{
	return d->uploaded;
}

template <typename T>
void SyncData<T>::setKey(DatasetId key)
{
	d->key = std::move(key);
}

template <typename T>
void SyncData<T>::setData(std::optional<T> data)
{
	d->data = std::move(data);
}

template <typename T>
void SyncData<T>::setModified(QDateTime modified)
{
	d->modified = std::move(modified);
}

template<typename T>
void SyncData<T>::setVersion(std::optional<QVersionNumber> version)
{
	d->version = std::move(version);
}

template<typename T>
void SyncData<T>::setUploaded(QDateTime uploaded)
{
	d->uploaded = std::move(uploaded);
}

}

Q_DECLARE_METATYPE(QtDataSync::LocalData)
Q_DECLARE_METATYPE(QtDataSync::CloudData)

#endif // QTDATASYNC_ICLOUDTRANSFORMER_H
