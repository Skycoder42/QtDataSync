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
	//! @private
	explicit Logger(QByteArray subCategory, const QString &setupName, QObject *parent = nullptr);
	~Logger();

	//! Returns the logging category this logger logs to
	const QLoggingCategory &loggingCategory() const;

	//! report a fatal error to the datasync engine
	Q_NORETURN void reportFatalError(const QString &error, const char *file, int line, const char *function);
	//! @copydoc Logger::reportFatalError(const QString &, const char *, int, const char *)
	Q_NORETURN void reportFatalError(const char *error, const char *file, int line, const char *function);

private:
	QScopedPointer<LoggerPrivate> d;
};

}

//! @private Internal macro
#define QT_DATASYNC_LOG_BASE QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC)
//! @private Internal macro
#define QT_DATASYNC_LOG_CAT QTDATASYNC_LOG->loggingCategory()
//! A define to log like with qDebug, but using a logger. QTDATASYNC_LOG must be defined as a pointer to a logger
#define logDebug() QT_DATASYNC_LOG_BASE.debug(QT_DATASYNC_LOG_CAT)
//! A define to log like with qInfo, but using a logger. QTDATASYNC_LOG must be defined as a pointer to a logger
#define logInfo() QT_DATASYNC_LOG_BASE.info(QT_DATASYNC_LOG_CAT)
//! A define to log like with qWarning, but using a logger. QTDATASYNC_LOG must be defined as a pointer to a logger
#define logWarning() QT_DATASYNC_LOG_BASE.warning(QT_DATASYNC_LOG_CAT)
//! A define to log like with qCritical, but using a logger. QTDATASYNC_LOG must be defined as a pointer to a logger
#define logCritical() QT_DATASYNC_LOG_BASE.critical(QT_DATASYNC_LOG_CAT)
//! A convenient define to log a fatal error, using QtDataSync::Logger::reportFatalError. QTDATASYNC_LOG must be defined as a pointer to a logger
#define logFatal(error) QTDATASYNC_LOG->reportFatalError(error, QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC)

//! @file logger.h The Logger header
#endif // QTDATASYNC_LOGGER_H
