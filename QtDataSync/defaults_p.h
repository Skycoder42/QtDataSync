#ifndef DEFAULTS_P_H
#define DEFAULTS_P_H

#include "defaults.h"
#include <QSqlDatabase>

namespace QtDataSync {

class DefaultsPrivate
{
public:
	static const QString DatabaseName;

	DefaultsPrivate(const QString &setupName, const QDir &storageDir);

	QDir storageDir;
	quint64 dbRefCounter;
	QByteArray catName;
	QLoggingCategory logCat;
	QSettings *settings;
};

}

#endif // DEFAULTS_P_H
