#ifndef DATASYNCSERVICE_H
#define DATASYNCSERVICE_H

#include <QtService/Service>

#include <QtCore/QSettings>
#include <QtCore/QPointer>

#include "clientconnector.h"
#include "databasecontroller.h"

class DatasyncService : public QtService::Service
{
	Q_OBJECT

public:
	enum ServiceCodes {
		CodeOffset = 128,
		CleanupCode
	};
	Q_ENUM(ServiceCodes)

	explicit DatasyncService(int &argc, char **argv);

	const QSettings *configuration() const;
	QThreadPool *threadPool() const;
	QString absolutePath(const QString &path) const;

protected:
	CommandMode onStart() override;
	CommandMode onStop(int &exitCode) override;
	CommandMode onReload() override;
	CommandMode onPause() override;
	CommandMode onResume() override;

private Q_SLOTS:
	void completeStartup(bool ok);

private:
	const QSettings *_config;
	QThreadPool *_mainPool;
	ClientConnector *_connector;
	DatabaseController *_database;

	QString findConfig() const;

	void sigusr1();
	void command(int cmd);
	void setLogLevel();
	void setupThreadPool();
};

#undef qService
#define qService static_cast<DatasyncService*>(QtService::Service::instance())

#endif // DATASYNCSERVICE_H
