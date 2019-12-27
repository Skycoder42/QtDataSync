#ifndef QTDATASYNC_ICLOUDTRANSFORMER_P_H
#define QTDATASYNC_ICLOUDTRANSFORMER_P_H

#include "cloudtransformer.h"

#include <QtCore/private/qobject_p.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT PlainCloudTransformerPrivate : public QObjectPrivate
{
	Q_DECLARE_PUBLIC(PlainCloudTransformer)
public:
	QtJsonSerializer::JsonSerializer *serializer = nullptr;
};

}

#endif // QTDATASYNC_ICLOUDTRANSFORMER_P_H
