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

	static QMutex setupMutex;
	static QHash<QString, SetupInfo> engines;
	static unsigned long timeout;

	QString localDir;
	QScopedPointer<QJsonSerializer> serializer;//TODO use normal pointer instead
	QHash<Defaults::PropertyKey, QVariant> properties;
	Setup::FatalErrorHandler fatalErrorHandler;

	SetupPrivate();
};

Q_DATASYNC_EXPORT void defaultFatalErrorHandler(QString error, QString setup, const QMessageLogContext &context);

}

#endif // QTDATASYNC_SETUP_P_H
