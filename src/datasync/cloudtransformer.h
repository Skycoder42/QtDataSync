#ifndef QTDATASYNC_ICLOUDTRANSFORMER_H
#define QTDATASYNC_ICLOUDTRANSFORMER_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/objectkey.h"

#include <optional>

#include <QtCore/qobject.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qshareddata.h>

#include <QtJsonSerializer/JsonSerializer>

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
	SyncData(ObjectKey key, std::optional<T> data, QDateTime modified, QDateTime uploaded = {});
	SyncData(QString type, QString key, std::optional<T> data, QDateTime modified, QDateTime uploaded = {});
	SyncData(const SyncData &other) = default;
	SyncData(SyncData &&other) noexcept = default;
	SyncData &operator=(const SyncData &other) = default;
	SyncData &operator=(SyncData &&other) noexcept = default;
	~SyncData() = default;

	template <typename TOther>
	SyncData(ObjectKey key, std::optional<T> data, const SyncData<TOther> &other);

	bool operator==(const SyncData &other) const;
	bool operator!=(const SyncData &other) const;

	ObjectKey key() const;
	std::optional<T> data() const;
	QDateTime modified() const;
	QDateTime uploaded() const;

	void setKey(ObjectKey key);
	void setData(std::optional<T> data);
	void setModified(QDateTime modified);
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
	virtual ObjectKey escapeKey(const ObjectKey &key);
	virtual ObjectKey unescapeKey(const ObjectKey &key);

public Q_SLOTS:
	virtual void transformUpload(const LocalData &data) = 0;
	virtual void transformDownload(const CloudData &data) = 0;

Q_SIGNALS:
	void transformUploadDone(const QtDataSync::CloudData &data);
	void transformDownloadDone(const QtDataSync::LocalData &data);
	void transformError(const QtDataSync::ObjectKey &key, const QString &message);

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

	virtual QJsonObject transformUploadSync(const QVariantHash &data) const = 0;  // throws QString
	virtual QVariantHash transformDownloadSync(const QJsonObject &data) const = 0;  // throws QString
};

class PlainCloudTransformerPrivate;
class Q_DATASYNC_EXPORT PlainCloudTransformer final : public ISynchronousCloudTransformer
{
	Q_OBJECT

	Q_PROPERTY(QtJsonSerializer::JsonSerializer* serializer READ serializer WRITE setSerializer NOTIFY serializerChanged)

public:
	explicit PlainCloudTransformer(QObject *parent = nullptr);
	explicit PlainCloudTransformer(QtJsonSerializer::JsonSerializer *serializer, QObject *parent = nullptr);

	QtJsonSerializer::JsonSerializer *serializer() const;

public Q_SLOTS:
	void setSerializer(QtJsonSerializer::JsonSerializer *serializer);

Q_SIGNALS:
	void serializerChanged(QtJsonSerializer::JsonSerializer *serializer, QPrivateSignal = {});

protected:
	QJsonObject transformUploadSync(const QVariantHash &data) const final;
	QVariantHash transformDownloadSync(const QJsonObject &data) const final;

private:
	Q_DECLARE_PRIVATE(PlainCloudTransformer)
};

// ------------- private implementation -------------

namespace __private {

template <typename T>
struct SyncDataData : public QSharedData {
	ObjectKey key;
	std::optional<T> data = std::nullopt;
	QDateTime modified;
	QDateTime uploaded;
};

}

// ------------- generic implementation -------------

template <typename T>
SyncData<T>::SyncData() :
	d{new __private::SyncDataData<T>{}}
{}

template <typename T>
SyncData<T>::SyncData(ObjectKey key, std::optional<T> data, QDateTime modified, QDateTime uploaded) :
	d{new __private::SyncDataData<T>{{}, std::move(key), std::move(data), std::move(modified), std::move(uploaded)}}
{}

template <typename T>
SyncData<T>::SyncData(QString type, QString key, std::optional<T> data, QDateTime modified, QDateTime uploaded) :
	SyncData{{std::move(type), std::move(key)}, std::move(data), std::move(modified), std::move(uploaded)}
{}
template <typename T>
template<typename TOther>
SyncData<T>::SyncData(ObjectKey key, std::optional<T> data, const SyncData<TOther> &other) :
	SyncData{std::move(key), std::move(data), other.modified(), other.uploaded()}
{}

template<typename T>
bool SyncData<T>::operator==(const SyncData &other) const
{
	return d == other.d || (
			   d->key == other.d->key &&
			   d->data == other.d->data &&
			   d->modified == other.d->modified &&
			   d->uploaded == other.d->uploaded);
}

template<typename T>
bool SyncData<T>::operator!=(const SyncData &other) const
{
	return d != other.d && (
			   d->key != other.d->key ||
			   d->data != other.d->data ||
			   d->modified != other.d->modified ||
			   d->uploaded != other.d->uploaded);
}

template <typename T>
ObjectKey SyncData<T>::key() const
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
QDateTime SyncData<T>::uploaded() const
{
	return d->uploaded;
}

template <typename T>
void SyncData<T>::setKey(ObjectKey key)
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
void SyncData<T>::setUploaded(QDateTime uploaded)
{
	d->uploaded = std::move(uploaded);
}

}

Q_DECLARE_METATYPE(QtDataSync::LocalData)
Q_DECLARE_METATYPE(QtDataSync::CloudData)

#endif // QTDATASYNC_ICLOUDTRANSFORMER_H
