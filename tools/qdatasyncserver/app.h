#ifndef APP_H
#define APP_H

#include <QtCore/QSettings>
#include <QtCore/QPointer>

#include <QtBackgroundProcess/App>
#include <QtBackgroundProcess/Terminal>

#include "clientconnector.h"
#include "databasecontroller.h"

class App : public QtBackgroundProcess::App
{
	Q_OBJECT

public:
	explicit App(int &argc, char **argv);

	QSettings *configuration() const;
	QThreadPool *threadPool() const;
	QString absolutePath(const QString &path) const;

	// App interface
protected:
	void setupParser(QCommandLineParser &parser, bool useShortOptions) override;
	int startupApp(const QCommandLineParser &parser) override;
	bool requestAppShutdown(QtBackgroundProcess::Terminal *terminal, int &) override;

private slots:
	void terminalConnected(QtBackgroundProcess::Terminal *terminal);
	void quitting();

	void dbDone(bool ok);

private:
	QSettings *config;
	QThreadPool *mainPool;
	ClientConnector *connector;
	DatabaseController *database;

	QPointer<QtBackgroundProcess::Terminal> currentTerminal;
	QString lastError;
	bool dbRdy;

	void completeStart();

	void startCleanup();
	void cleanupOperationDone(int rowsAffected, const QString &error);
};

#undef qApp
#define qApp static_cast<App*>(QCoreApplication::instance())

#endif // APP_H
