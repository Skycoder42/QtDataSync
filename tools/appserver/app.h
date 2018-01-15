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
		CodeOffset = 127,
		CleanupCode
	};
	Q_ENUM(ServiceCodes)

	explicit App(int &argc, char **argv);

	QSettings *configuration() const;
	QThreadPool *threadPool() const;
	QString absolutePath(const QString &path) const;

protected Q_SLOTS://defined like that to be ready for service inclusion
	bool start(const QString &serviceName);
	void stop();
	void pause();
	void resume();
	void processCommand(int code);

private Q_SLOTS:
	void completeStartup(bool ok);

private:
	QSettings *config;
	QThreadPool *mainPool;
	ClientConnector *connector;
	DatabaseController *database;
};

#undef qApp
#define qApp static_cast<App*>(QCoreApplication::instance())

#endif // APP_H
