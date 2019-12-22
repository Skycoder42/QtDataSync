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

struct CloudDataData;
class Q_DATASYNC_EXPORT CloudData
{
	Q_GADGET

	Q_PROPERTY(QtDataSync::ObjectKey key READ key WRITE setKey)
	Q_PROPERTY(std::optional<QJsonObject> data READ data WRITE setData)
	Q_PROPERTY(QDateTime modified READ modified WRITE setModified)

public:
	CloudData();
	CloudData(ObjectKey key, std::optional<QJsonObject> data, QDateTime modified);
	CloudData(QString type, QString key, std::optional<QJsonObject> data, QDateTime modified);
	CloudData(const CloudData &other);
	CloudData(CloudData &&other) noexcept;
	CloudData &operator=(const CloudData &other);
	CloudData &operator=(CloudData &&other) noexcept;
	~CloudData();

	ObjectKey key() const;
	std::optional<QJsonObject> data() const;
	QDateTime modified() const;

	void setKey(ObjectKey key);
	void setData(std::optional<QJsonObject> data);
	void setModified(QDateTime modified);

private:
	QSharedDataPointer<CloudDataData> d;
};

class Q_DATASYNC_EXPORT ICloudTransformer : public QObject
{
	Q_OBJECT

public Q_SLOTS:
	virtual void transformUpload(ObjectKey key, const std::optional<QVariantHash> &data, QDateTime modified) = 0;
	virtual void transformDownload(const CloudData &data) = 0;

Q_SIGNALS:
	void transformUploadDone(const CloudData &data);
	void transformDownloadDone(ObjectKey key, const std::optional<QVariantHash> &data, QDateTime modified);

protected:
	explicit ICloudTransformer(QObject *parent = nullptr);
	ICloudTransformer(QObjectPrivate &dd, QObject *parent = nullptr);
};

class Q_DATASYNC_EXPORT ISynchronousCloudTransformer : public ICloudTransformer
{
public:
	void transformUpload(ObjectKey key, const std::optional<QVariantHash> &data, QDateTime modified) final;
	void transformDownload(const CloudData &data) final;

protected:
	explicit ISynchronousCloudTransformer(QObject *parent = nullptr);
	ISynchronousCloudTransformer(QObjectPrivate &dd, QObject *parent = nullptr);

	virtual QJsonObject transformUploadSync(const QVariantHash &data) const = 0;
	virtual QVariantHash transformDownloadSync(const QJsonObject &data) const = 0;
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

}

#endif // QTDATASYNC_ICLOUDTRANSFORMER_H
