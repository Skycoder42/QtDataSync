#ifndef APP_H
#define APP_H

#include "clientconnector.h"
#include "databasecontroller.h"

#include <QSettings>
#include <QtBackgroundProcess>

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
	bool requestAppShutdown(QtBackgroundProcess::Terminal *terminal, int &exitCode) override;

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
	void completeCleanup();
};

#undef qApp
#define qApp static_cast<App*>(QCoreApplication::instance())

#endif // APP_H
