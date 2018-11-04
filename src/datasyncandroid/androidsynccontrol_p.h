#ifndef QTDATASYNC_ANDROIDSYNCCONTROL_P_H
#define QTDATASYNC_ANDROIDSYNCCONTROL_P_H

#include "androidsynccontrol.h"

#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroidExtras/QAndroidIntent>

namespace QtDataSync {

class AndroidSyncControlPrivate
{
public:
	QString serviceId {QStringLiteral("de.skycoder42.qtservice.AndroidService")};
	std::chrono::minutes interval{60};

	QAndroidIntent createIntent() const;
	QAndroidJniObject createPendingIntent(bool allowCreate) const;
};

}

#endif // QTDATASYNC_ANDROIDSYNCCONTROL_P_H
