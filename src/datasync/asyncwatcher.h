#ifndef QTDATASYNC_ASYNCWATCHER_H
#define QTDATASYNC_ASYNCWATCHER_H

#include "QtDataSync/qtdatasync_global.h"

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>

#include <QtSql/qsqldatabase.h>

class QRemoteObjectNode;

namespace QtDataSync {

class Engine;

class AsyncWatcherPrivate;
class Q_DATASYNC_EXPORT AsyncWatcher : public QObject
{
	Q_OBJECT

public:
	explicit AsyncWatcher(Engine *engine, QObject *parent = nullptr);
	explicit AsyncWatcher(const QUrl &remoteUrl, QObject *parent = nullptr);
	explicit AsyncWatcher(QRemoteObjectNode *remoteNode, QObject *parent = nullptr);

	void addDatabase(const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection));
	void addDatabase(const QString &databaseConnection,
					 const QString &originalConnection);
	void addDatabase(QSqlDatabase database);
	void addDatabase(QSqlDatabase database,
					 const QString &originalConnection);

	void removeDatabase(const QString &databaseConnection = QLatin1String(QSqlDatabase::defaultConnection));
	void removeDatabase(QSqlDatabase database);

Q_SIGNALS:
	void initialized(QPrivateSignal = {});

private:
	Q_DECLARE_PRIVATE(AsyncWatcher)

	Q_PRIVATE_SLOT(d_func(), void _q_nodeError(QRemoteObjectNode::ErrorCode))
	Q_PRIVATE_SLOT(d_func(), void _q_nodeInitialized())
	Q_PRIVATE_SLOT(d_func(), void _q_tableAdded(const TableEvent &))
	Q_PRIVATE_SLOT(d_func(), void _q_tableRemoved(const TableEvent &))
	Q_PRIVATE_SLOT(d_func(), void _q_tableEvent(const QString &))
};

}

#endif // QTDATASYNC_ASYNCWATCHER_H
