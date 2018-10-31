#ifndef QTDATASYNC_ANDROIDSYNCCONTROL_P_H
#define QTDATASYNC_ANDROIDSYNCCONTROL_P_H

#include "androidsynccontrol.h"

#include <QtAndroidExtras/QAndroidJniObject>

namespace QtDataSync {

class AndroidSyncControlData : public QSharedData
{
public:
	AndroidSyncControlData();
	AndroidSyncControlData(const AndroidSyncControlData &other);

	QString serviceId;
	std::chrono::minutes delay;

	QAndroidJniObject createPendingIntent(bool allowCreate) const;
};

}

#endif // QTDATASYNC_ANDROIDSYNCCONTROL_P_H
