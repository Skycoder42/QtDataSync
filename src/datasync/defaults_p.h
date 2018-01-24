#ifndef QTDATASYNC_DEFAULTS_P_H
#define QTDATASYNC_DEFAULTS_P_H

#include <QtCore/QMutex>
#include <QtCore/QThreadStorage>

#include <QtSql/QSqlDatabase>

#include <QtJsonSerializer/QJsonSerializer>

#include "qtdatasync_global.h"
#include "objectkey.h"
#include "defaults.h"
#include "logger.h"
#include "conflictresolver.h"
#include "emitteradapter_p.h"

class ChangeEmitterReplica;

namespace QtDataSync {

class ChangeEmitter;

//no exports needed
class DatabaseRefPrivate : public QObject
{
public:
	DatabaseRefPrivate(QSharedPointer<DefaultsPrivate> defaultsPrivate, QObject *object);
	~DatabaseRefPrivate();

	QSqlDatabase &db();
	bool eventFilter(QObject *watched, QEvent *event) override;

private:
	QSharedPointer<DefaultsPrivate> _defaultsPrivate;
	QObject *_object;
	QSqlDatabase _database;
};

//export needed for tests
class Q_DATASYNC_EXPORT DefaultsPrivate : public QObject
{
	friend class Defaults;
	Q_OBJECT

public:
	static const QString DatabaseName;

	static void createDefaults(const QString &setupName,
							   bool isPassive,
							   const QDir &storageDir,
							   const QUrl &roAddress,
							   const QHash<Defaults::PropertyKey, QVariant> &properties,
							   QJsonSerializer *serializer,
							   ConflictResolver *resolver);
	static void removeDefaults(const QString &setupName);
	static void clearDefaults();
	static QSharedPointer<DefaultsPrivate> obtainDefaults(const QString &setupName);

	DefaultsPrivate(const QString &setupName,
					const QDir &storageDir,
					const QUrl &roAddress,
					const QHash<Defaults::PropertyKey, QVariant> &properties,
					QJsonSerializer *serializer,
					ConflictResolver *resolver);
	~DefaultsPrivate();

	QSqlDatabase acquireDatabase();
	void releaseDatabase();

	QRemoteObjectNode *acquireNode();

public Q_SLOTS:
	void roThreadDone();

Q_SIGNALS:
	void passiveCreated();
	void passiveReady();

private Q_SLOTS:
	void makePassive();

private:
	static QMutex setupDefaultsMutex;
	static QHash<QString, QSharedPointer<DefaultsPrivate>> setupDefaults;
	static QThreadStorage<QHash<QString, quint64>> dbRefHash;

	QString setupName;
	QDir storageDir;
	Logger *logger;
	QUrl roAddress;
	QJsonSerializer *serializer;
	ConflictResolver *resolver;
	QHash<Defaults::PropertyKey, QVariant> properties;

	QMutex roMutex;
	QHash<QThread*, QRemoteObjectNode*> roNodes;

	QSharedPointer<EmitterAdapter::CacheInfo> cacheInfo;

	ChangeEmitterReplica *passiveEmitter;
};

}

#endif // QTDATASYNC_DEFAULTS_P_H
