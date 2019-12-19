#ifndef QTDATASYNC_ICLOUDTRANSFORMER_H
#define QTDATASYNC_ICLOUDTRANSFORMER_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/objectkey.h"

#include <QtCore/qobject.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qshareddata.h>

namespace QtDataSync {

struct CloudDataData;
class Q_DATASYNC_EXPORT CloudData
{
	Q_GADGET

	Q_PROPERTY(QtDataSync::ObjectKey key READ key WRITE setKey)
	Q_PROPERTY(QJsonObject data READ data WRITE setData)
	Q_PROPERTY(QDateTime modified READ modified WRITE setModified)

public:
	CloudData();
	CloudData(ObjectKey key, QJsonObject data, QDateTime modified);
	CloudData(QByteArray type, QString key, QJsonObject data, QDateTime modified);
	CloudData(const CloudData &other);
	CloudData(CloudData &&other) noexcept;
	CloudData &operator=(const CloudData &other);
	CloudData &operator=(CloudData &&other) noexcept;
	~CloudData();

	ObjectKey key() const;
	QJsonObject data() const;
	QDateTime modified() const;

	void setKey(ObjectKey key);
	void setData(QJsonObject data);
	void setModified(QDateTime modified);

private:
	QSharedDataPointer<CloudDataData> d;
};

class Q_DATASYNC_EXPORT ICloudTransformer
{
	Q_DISABLE_COPY(ICloudTransformer)

public:
	ICloudTransformer();
	virtual ~ICloudTransformer();
};

}

#endif // QTDATASYNC_ICLOUDTRANSFORMER_H
