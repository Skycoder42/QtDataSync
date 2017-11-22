#ifndef QTDATASYNC_DEFAULTS_P_H
#define QTDATASYNC_DEFAULTS_P_H

#include "qtdatasync_global.h"
#include "defaults.h"
#include "logger.h"

#include <QtCore/QMutex>
#include <QtSql/QSqlDatabase>
#include <QtJsonSerializer/QJsonSerializer>

namespace QtDataSync {

class Q_DATASYNC_EXPORT DatabaseRefPrivate
{
public:
	DatabaseRefPrivate(QSharedPointer<DefaultsPrivate> defaultsPrivate, QSqlDatabase &memberDb);
	~DatabaseRefPrivate();

private:
	QSharedPointer<DefaultsPrivate> defaultsPrivate;
	QSqlDatabase &memberDb;
};

class Q_DATASYNC_EXPORT DefaultsPrivate : public QObject
{
	friend class Defaults;

public:
	static const QString DatabaseName;

	static void createDefaults(const QString &setupName,
							   const QDir &storageDir,
							   const QHash<QByteArray, QVariant> &properties,
							   QJsonSerializer *serializer);
	static void removeDefaults(const QString &setupName);
	static void clearDefaults();
	static QSharedPointer<DefaultsPrivate> obtainDefaults(const QString &setupName);

	DefaultsPrivate(const QString &setupName,
					const QDir &storageDir,
					const QHash<QByteArray, QVariant> &properties,
					QJsonSerializer *serializer);
	~DefaultsPrivate();

	QSqlDatabase acquireDatabase();
	void releaseDatabase();

private:
	static QMutex setupDefaultsMutex;
	static QHash<QString, QSharedPointer<DefaultsPrivate>> setupDefaults;

	QString setupName;
	QDir storageDir;
	Logger *logger;
	QJsonSerializer *serializer;

	quint64 dbRefCounter;
};

}

#endif // QTDATASYNC_DEFAULTS_P_H
