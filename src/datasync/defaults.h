#ifndef QTDATASYNC_DEFAULTS_H
#define QTDATASYNC_DEFAULTS_H

#include "QtDataSync/qtdatasync_global.h"

#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qdir.h>
#include <QtCore/qsettings.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qdebug.h>

class QSqlDatabase;
class QJsonSerializer;

namespace QtDataSync {

class Logger;

class DefaultsPrivate;
//! A helper class to get defaults per datasync instance
class Q_DATASYNC_EXPORT Defaults : public QObject
{
	Q_OBJECT

public:
	//! Constructor called from the setup
	QT_DEPRECATED Defaults(const QString &setupName,
						   const QDir &storageDir,
						   const QHash<QByteArray, QVariant> &properties,
						   QObject *parent = nullptr);
	//! Constructor called from the setup
	Defaults(const QString &setupName,
			 const QDir &storageDir,
			 const QHash<QByteArray, QVariant> &properties,
			 const QJsonSerializer *serializer,
			 QObject *parent = nullptr);
	~Defaults();

	//! A logging category for this setup
	QT_DEPRECATED const QLoggingCategory &loggingCategory() const;
	Logger *createLogger(const QByteArray &subCategory, QObject *parent = nullptr) const;

	//! Returns the name of the current setup
	QString setupName() const;
	//! Returns the storage directory
	QDir storageDir() const;
	//! Returns an instance of QSettings owned by defaults
	QSettings *settings() const;
	//! Returns a new instance of QSettings for this setup
	QSettings *createSettings(QObject *parent = nullptr) const;
	//! Returns the serializer of the current setup
	const QJsonSerializer *serializer() const;

	//! Aquire the standard sqlite database
	QSqlDatabase aquireDatabase();
	//! Release you reference to the standard sqlite database
	void releaseDatabase();

private:
	QScopedPointer<DefaultsPrivate> d;
};

}

#endif // QTDATASYNC_DEFAULTS_H
