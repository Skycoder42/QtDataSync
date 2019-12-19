#ifndef QTDATASYNC_ICLOUDTRANSFORMER_P_H
#define QTDATASYNC_ICLOUDTRANSFORMER_P_H

#include "icloudtransformer.h"

namespace QtDataSync {

struct CloudDataData : public QSharedData
{
	ObjectKey key;
	QJsonObject data;
	QDateTime modified;
};

}

#endif // QTDATASYNC_ICLOUDTRANSFORMER_P_H
