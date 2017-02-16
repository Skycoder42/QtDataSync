#ifndef DEFAULTS_H
#define DEFAULTS_H

#include "qtdatasync_global.h"
#include <QObject>
#include <QDir>
#include <QSettings>
#include <QLoggingCategory>
class QSqlDatabase;

namespace QtDataSync {

class DefaultsPrivate;
class QTDATASYNCSHARED_EXPORT Defaults : public QObject
{
	Q_OBJECT

public:
	Defaults(const QString &setupName, const QDir &storageDir, QObject *parent = nullptr);
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
