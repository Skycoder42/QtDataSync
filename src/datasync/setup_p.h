#ifndef QTDATASYNC_SETUP_P_H
#define QTDATASYNC_SETUP_P_H

#include "qtdatasync_global.h"
#include "setup.h"
#include "defaults.h"

#include <QtCore/QMutex>
#include <QtCore/QThread>

#include <QtJsonSerializer/QJsonSerializer>

namespace QtDataSync {

class ExchangeEngine;//TODO include

class Q_DATASYNC_EXPORT SetupPrivate
{
	friend class Setup;

public:
	static Defaults *defaults(const QString &name = Setup::DefaultSetup);

	static void cleanupHandler();

private:
	struct SetupInfo {
		QThread *thread;
		Defaults *defaults;
		ExchangeEngine *engine;

		SetupInfo();
		SetupInfo(QThread *thread, Defaults *defaults, ExchangeEngine *engine);
	};

	static QMutex setupMutex;
	static QHash<QString, SetupInfo> engines;
	static unsigned long timeout;

	QString localDir;
	QScopedPointer<QJsonSerializer> serializer;
	QHash<QByteArray, QVariant> properties;
	std::function<void (QString, bool, QString)> fatalErrorHandler;

	SetupPrivate();
};

Q_DATASYNC_EXPORT void defaultFatalErrorHandler(QString error, bool recoverable, QString setupName);

}

#endif // QTDATASYNC_SETUP_P_H
