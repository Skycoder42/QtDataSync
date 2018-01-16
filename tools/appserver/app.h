#ifndef APP_H
#define APP_H

#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>
#include <QtCore/QPointer>

#include "clientconnector.h"
#include "databasecontroller.h"

class App : public QCoreApplication
{
	Q_OBJECT

public:
	enum ServiceCodes {
		CodeOffset = 128,
		CleanupCode
	};
	Q_ENUM(ServiceCodes)

	explicit App(int &argc, char **argv);

	const QSettings *configuration() const;
	QThreadPool *threadPool() const;
	QString absolutePath(const QString &path) const;

public Q_SLOTS://defined like that to be ready for service inclusion
	bool start(const QString &serviceName);
	void stop();
	void pause();
	void resume();
	void reload();
	void processCommand(int code);

private Q_SLOTS:
	void completeStartup(bool ok);
	void onSignal(int signal);

private:
	const QSettings *_config;
	QThreadPool *_mainPool;
	ClientConnector *_connector;
	DatabaseController *_database;

	QString findConfig() const;
};

#undef qApp
#define qApp static_cast<App*>(QCoreApplication::instance())

#endif // APP_H
