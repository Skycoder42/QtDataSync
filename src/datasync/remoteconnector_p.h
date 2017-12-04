#ifndef REMOTECONNECTOR_P_H
#define REMOTECONNECTOR_P_H

#include <QtCore/QObject>

#include "defaults.h"
#include "qtdatasync_global.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT RemoteConnector : public QObject
{
	Q_OBJECT

public:
	explicit RemoteConnector(const Defaults &defaults, QObject *parent = nullptr);

private:
	Defaults _defaults;
	Logger *_logger;
	QSettings *_settings;
};

}

#endif // REMOTECONNECTOR_P_H
