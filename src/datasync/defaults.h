#ifndef DEFAULTS_H
#define DEFAULTS_H

#include "QtDataSync/qdatasync_global.h"

#include <QtCore/qobject.h>
#include <QtCore/qdir.h>
#include <QtCore/qsettings.h>
#include <QtCore/qloggingcategory.h>

class QSqlDatabase;

namespace QtDataSync {

class DefaultsPrivate;
class Q_DATASYNC_EXPORT Defaults : public QObject
{
	Q_OBJECT

public:
	Defaults(const QString &setupName,
			 const QDir &storageDir,
			 const QHash<QByteArray, QVariant> &properties,
			 QObject *parent = nullptr);
	~Defaults();

	const QLoggingCategory &loggingCategory() const;

	QDir storageDir() const;
	QSettings *settings() const;
	QSettings *createSettings(QObject *parent = nullptr) const;

	QSqlDatabase aquireDatabase();
	void releaseDatabase();

private:
	QScopedPointer<DefaultsPrivate> d;
};

}

#endif // DEFAULTS_H
