#ifndef QTDATASYNC_LOGGER_H
#define QTDATASYNC_LOGGER_H

#include <QtCore/qglobal.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {

class LoggerPrivate;
//! A Helper class for simple and structured logging
class Q_DATASYNC_EXPORT Logger : public QObject
{
	Q_OBJECT

public:
	//! Constructor, internally used by Defaults::createLogger
	explicit Logger(QByteArray subCategory, const QString &setupName, QObject *parent = nullptr);
	//! Destructor
	~Logger();

	//! Returns the logging category this logger logs to
	const QLoggingCategory &loggingCategory() const;

	//! report a fatal error to the datasync engine
	void reportFatalError(const QString &error, bool resyncRecoverable, const char *file, int line, const char *function);

private:
	QScopedPointer<LoggerPrivate> d;
};

}

//! A define to log like with qDebug, but using a logger. QTDATASYNC_LOG must be defined as a pointer to a logger
#define logDebug() qCDebug(QTDATASYNC_LOG->loggingCategory())
//! A define to log like with qInfo, but using a logger. QTDATASYNC_LOG must be defined as a pointer to a logger
#define logInfo() qCInfo(QTDATASYNC_LOG->loggingCategory())
//! A define to log like with qWarning, but using a logger. QTDATASYNC_LOG must be defined as a pointer to a logger
#define logWarning() qCWarning(QTDATASYNC_LOG->loggingCategory())
//! A define to log like with qCritical, but using a logger. QTDATASYNC_LOG must be defined as a pointer to a logger
#define logCritical() qCCritical(QTDATASYNC_LOG->loggingCategory())
//! A convenient define to log a fatal error, using QtDataSync::Logger::reportFatalError. QTDATASYNC_LOG must be defined as a pointer to a logger
#define logFatal(resyncRecoverable, error) QTDATASYNC_LOG->reportFatalError(error, resyncRecoverable, QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC)

//! @file logger.h The Logger header
#endif // QTDATASYNC_LOGGER_H
