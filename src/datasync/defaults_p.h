#ifndef QTDATASYNC_DEFAULTS_P_H
#define QTDATASYNC_DEFAULTS_P_H

#include "qtdatasync_global.h"
#include "defaults.h"
#include "logger.h"

#include <QtCore/QMutex>
#include <QtCore/QThreadStorage>
#include <QtSql/QSqlDatabase>
#include <QtJsonSerializer/QJsonSerializer>

namespace QtDataSync {

class Q_DATASYNC_EXPORT DatabaseRefPrivate : public QObject
{
public:
	DatabaseRefPrivate(QSharedPointer<DefaultsPrivate> defaultsPrivate, QObject *object);
	~DatabaseRefPrivate();

	QSqlDatabase &db();
	bool eventFilter(QObject *watched, QEvent *event) override;

private:
	QSharedPointer<DefaultsPrivate> defaultsPrivate;
	QObject *object;
	QSqlDatabase database;
};

class Q_DATASYNC_EXPORT DefaultsPrivate : public QObject
{
	friend class Defaults;

public:
	static const QString DatabaseName;

	static void createDefaults(const QString &setupName,
							   const QDir &storageDir,
							   const QHash<Defaults::PropertyKey, QVariant> &properties,
							   QJsonSerializer *serializer);
	static void removeDefaults(const QString &setupName);
	static void clearDefaults();
	static QSharedPointer<DefaultsPrivate> obtainDefaults(const QString &setupName);

	DefaultsPrivate(const QString &setupName,
					const QDir &storageDir,
					const QHash<Defaults::PropertyKey, QVariant> &properties,
					QJsonSerializer *serializer);

	QSqlDatabase acquireDatabase(QThread *thread);
	void releaseDatabase(QThread *thread);

private:
	static QMutex setupDefaultsMutex;
	static QHash<QString, QSharedPointer<DefaultsPrivate>> setupDefaults;

	QString setupName;
	QDir storageDir;
	Logger *logger;
	QJsonSerializer *serializer;
	QHash<Defaults::PropertyKey, QVariant> properties;

	QThreadStorage<quint64> dbRefCounter;
};

}

#endif // QTDATASYNC_DEFAULTS_P_H
