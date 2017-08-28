#ifndef QTDATASYNC_LOGGER_H
#define QTDATASYNC_LOGGER_H

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/setup.h"

#include <QtCore/qglobal.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

namespace QtDataSync {

class LoggerPrivate;
class Q_DATASYNC_EXPORT Logger : public QObject
{
	Q_OBJECT

public:
	explicit Logger(QByteArray subCategory, const QString &setupName = Setup::DefaultSetup, QObject *parent = nullptr);
	~Logger();

	const QLoggingCategory &loggingCategory() const;

	void reportFatalError(const QString &error, bool resyncRecoverable, const char *file, int line, const char *function);

private:
	QScopedPointer<LoggerPrivate> d;
};

}

#define logDebug() qCDebug(QTDATASYNC_LOG->loggingCategory())
#define logInfo() qCInfo(QTDATASYNC_LOG->loggingCategory())
#define logWarning() qCWarning(QTDATASYNC_LOG->loggingCategory())
#define qlogCritical() qCCritical(QTDATASYNC_LOG->loggingCategory())
#define logFatal(resyncRecoverable, error) QTDATASYNC_LOG->reportFatalError(error, resyncRecoverable, QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC)

#endif // QTDATASYNC_LOGGER_H
