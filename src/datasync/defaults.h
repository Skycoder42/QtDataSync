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
//! A helper class to get defaults per datasync instance
class Q_DATASYNC_EXPORT Defaults : public QObject
{
	Q_OBJECT

public:
	//! Constructor called from the setup
	Defaults(const QString &setupName,
			 const QDir &storageDir,
			 const QHash<QByteArray, QVariant> &properties,
			 QObject *parent = nullptr);
	~Defaults();

	//! A logging category for this setup
	const QLoggingCategory &loggingCategory() const;

	//! Returns the storage directory
	QDir storageDir() const;
	//! Returns an instance of QSettings owned by defaults
	QSettings *settings() const;
	//! Returns a new instance of QSettings for this setup
	QSettings *createSettings(QObject *parent = nullptr) const;

	//! Aquire the standard sqlite database
	QSqlDatabase aquireDatabase();
	//! Release you reference to the standard sqlite database
	void releaseDatabase();

private:
	QScopedPointer<DefaultsPrivate> d;
};

}

#endif // DEFAULTS_H
