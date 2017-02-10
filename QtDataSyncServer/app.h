#ifndef APP_H
#define APP_H

#include "clientconnector.h"

#include <QtBackgroundProcess>

class App : public QtBackgroundProcess::App
{
	Q_OBJECT
public:
	explicit App(int &argc, char **argv);

	// App interface
protected:
	void setupParser(QCommandLineParser &parser, bool useShortOptions) override;
	int startupApp(const QCommandLineParser &parser) override;
	bool requestAppShutdown(QtBackgroundProcess::Terminal *terminal, int &exitCode) override;

private:
	ClientConnector *connector;
};

#endif // APP_H
