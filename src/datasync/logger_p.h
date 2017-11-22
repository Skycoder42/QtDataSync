#ifndef QTDATASYNC_LOGGER_P_H
#define QTDATASYNC_LOGGER_P_H

#include "qtdatasync_global.h"
#include "logger.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT LoggerPrivate
{
public:
	LoggerPrivate(const QString &setupName, const QByteArray &subCategory);

	QString setupName;
	QByteArray catName;
	QLoggingCategory logCat;
};

}

#endif // QTDATASYNC_LOGGER_P_H
