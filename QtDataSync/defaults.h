#ifndef DEFAULTS_H
#define DEFAULTS_H

#include "qtdatasync_global.h"
#include <QDir>
#include <QSettings>
#include <QSqlDatabase>

namespace QtDataSync {

class QTDATASYNCSHARED_EXPORT Defaults
{
public:
	static QSettings *settings(const QDir &storageDir, QObject *parent = nullptr);
	static QSqlDatabase aquireDatabase(const QDir &storageDir);
	static void releaseDatabase(const QDir &storageDir);

private:
	static const QString DatabaseName;
	static QHash<QString, quint64> refCounter;
};

}

#endif // DEFAULTS_H
