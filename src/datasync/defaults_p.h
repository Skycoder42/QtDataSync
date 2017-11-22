#ifndef QTDATASYNC_DEFAULTS_P_H
#define QTDATASYNC_DEFAULTS_P_H

#include "qtdatasync_global.h"
#include "defaults.h"
#include "logger.h"

#include <QtSql/QSqlDatabase>
#include <QtJsonSerializer/QJsonSerializer>

namespace QtDataSync {

class Q_DATASYNC_EXPORT DefaultsPrivate : public QObject
{
public:
	static const QString DatabaseName;

	DefaultsPrivate(const QString &setupName,
					const QDir &storageDir,
					const QJsonSerializer *serializer);
	~DefaultsPrivate();

	QString setupName;
	QDir storageDir;
	quint64 dbRefCounter;
	Logger *logger;
	QSettings *settings;
	QPointer<const QJsonSerializer> serializer;
};

}

#endif // QTDATASYNC_DEFAULTS_P_H
