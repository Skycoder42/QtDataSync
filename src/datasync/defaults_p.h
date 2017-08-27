#ifndef QTDATASYNC_DEFAULTS_P_H
#define QTDATASYNC_DEFAULTS_P_H

#include "qtdatasync_global.h"
#include "defaults.h"

#include <QtSql/QSqlDatabase>

namespace QtDataSync {

class Q_DATASYNC_EXPORT DefaultsPrivate
{
public:
	static const QString DatabaseName;

	DefaultsPrivate(const QString &setupName,
					const QDir &storageDir,
					const QJsonSerializer *serializer);

	QString setupName;
	QDir storageDir;
	quint64 dbRefCounter;
	QByteArray catName;
	QLoggingCategory logCat;
	QSettings *settings;
	QPointer<const QJsonSerializer> serializer;
};

}

#endif // QTDATASYNC_DEFAULTS_P_H
