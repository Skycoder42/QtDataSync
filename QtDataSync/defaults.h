#ifndef DEFAULTS_H
#define DEFAULTS_H

#include "qtdatasync_global.h"
#include <QDir>
#include <QSettings>
#include <QSqlDatabase>
#include <QLoggingCategory>

namespace QtDataSync {

class QTDATASYNCSHARED_EXPORT Defaults
{
public:
	static void registerSetup(const QDir &storageDir, const QString &name);
	static void unregisterSetup(const QDir &storageDir);

	static QSettings *settings(const QDir &storageDir, QObject *parent = nullptr);
	static QSqlDatabase aquireDatabase(const QDir &storageDir);
	static void releaseDatabase(const QDir &storageDir);

	static const QLoggingCategory &loggingCategory(const QDir &storageDir);

private:
	static const QString DatabaseName;
	static QHash<QString, quint64> refCounter;
	static QHash<QString, QPair<QByteArray, QSharedPointer<QLoggingCategory>>> sNames;
};

}

#endif // DEFAULTS_H
