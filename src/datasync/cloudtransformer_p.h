#ifndef QTDATASYNC_ICLOUDTRANSFORMER_P_H
#define QTDATASYNC_ICLOUDTRANSFORMER_P_H

#include "cloudtransformer.h"

#include <QtCore/private/qobject_p.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT PlainCloudTransformerPrivate : public QObjectPrivate
{
	Q_DECLARE_PUBLIC(PlainCloudTransformer)

public:
	static const QString MarkerByteArray;
	static const QString MarkerUrl;
	static const QString MarkerDateTime;
	static const QString MarkerUuid;
	static const QString MarkerVariant;

	QJsonValue serialize(const QVariant &data) const;
	QVariant deserialize(const QJsonValue &json) const;

	QJsonValue writeVariant(const QVariant &data, const QString &marker = MarkerByteArray);
};

}

#endif // QTDATASYNC_ICLOUDTRANSFORMER_P_H
