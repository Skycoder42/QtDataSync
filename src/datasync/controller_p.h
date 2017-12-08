#ifndef CONTROLLER_P_H
#define CONTROLLER_P_H

#include <QtCore/QObject>

#include "qtdatasync_global.h"
#include "defaults.h"
#include "logger.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT Controller : public QObject
{
	Q_OBJECT

public:
	explicit Controller(const QByteArray &name,
						Defaults defaults,
						QObject *parent = nullptr);

	//exceptions are allowed!
	virtual void initialize();
	virtual void finalize();

protected:
	Defaults defaults() const;
	Logger *logger() const;
	QSettings *settings() const;

private:
	Defaults _defaults;
	Logger *_logger;
	QSettings *_settings;
};

}

#define QTDATASYNC_LOG_CONTROLLER logger()

#endif // CONTROLLER_P_H
