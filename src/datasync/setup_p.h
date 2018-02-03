#ifndef QTDATASYNC_SETUP_P_H
#define QTDATASYNC_SETUP_P_H

#include <QtCore/QMutex>
#include <QtCore/QThread>

#include <QtJsonSerializer/QJsonSerializer>

#include "qtdatasync_global.h"
#include "setup.h"
#include "defaults.h"
#include "exchangeengine_p.h"
#include "conflictresolver.h"

namespace QtDataSync {

//must be exported for tests
class Q_DATASYNC_EXPORT SetupPrivate
{
	friend class Setup;

public:
	static void cleanupHandler();
	static unsigned long currentTimeout();

	static ExchangeEngine *engine(const QString &setupName);

	QDir createStorageDir(const QString &setupName);
	void createDefaults(const QString &setupName, const QDir &storageDir, bool passive);

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
	QUrl roAddress;
	QScopedPointer<QJsonSerializer> serializer;
	QScopedPointer<ConflictResolver> resolver;
	QHash<Defaults::PropertyKey, QVariant> properties;
	Setup::FatalErrorHandler fatalErrorHandler;

	SetupPrivate();
};

}

#endif // QTDATASYNC_SETUP_P_H
