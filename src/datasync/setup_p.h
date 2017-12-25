#ifndef QTDATASYNC_SETUP_P_H
#define QTDATASYNC_SETUP_P_H

#include <QtCore/QMutex>
#include <QtCore/QThread>

#include <QtJsonSerializer/QJsonSerializer>

#include "qtdatasync_global.h"
#include "setup.h"
#include "defaults.h"
#include "exchangeengine_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT SetupPrivate
{
	friend class Setup;

public:
	static void cleanupHandler();

	static ExchangeEngine *engine(const QString &setupName);

private:
	struct SetupInfo {
		QThread *thread;
		ExchangeEngine *engine;

		SetupInfo();
		SetupInfo(QThread *thread, ExchangeEngine *engine);
	};

	static const QString DefaultLocalDir;

	static QMutex setupMutex;
	static QHash<QString, SetupInfo> engines;
	static unsigned long timeout;

	QString localDir;
	QScopedPointer<QJsonSerializer> serializer;
	QHash<Defaults::PropertyKey, QVariant> properties;
	Setup::FatalErrorHandler fatalErrorHandler;

	SetupPrivate();
};

}

#endif // QTDATASYNC_SETUP_P_H
