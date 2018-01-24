#ifndef QTDATASYNC_CONTROLLER_P_H
#define QTDATASYNC_CONTROLLER_P_H

#include <chrono>

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QDeadlineTimer>

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

	virtual void initialize(const QVariantHash &params); //exceptions are allowed!
	virtual void finalize();

Q_SIGNALS:
	void controllerError(const QString &error);
	void operationTimeout();
	void specialOperationTimeout();
	void progressAdded(quint32 delta);
	void progressIncrement();

protected:
	Defaults defaults() const;
	Logger *logger() const;
	QSettings *settings() const;

	void beginOp(const std::chrono::minutes &interval = std::chrono::minutes(5),
				 bool startIfNotRunning = true);
	void beginSpecialOp(const std::chrono::minutes &interval);
	void endOp();

private Q_SLOTS:
	void onTimeout();

private:
	Defaults _defaults;
	Logger *_logger;
	QSettings *_settings;
	QTimer *_opTimer;

	QDeadlineTimer _specialOp;
};

}

#define QTDATASYNC_LOG_CONTROLLER logger()

#endif // QTDATASYNC_CONTROLLER_P_H
