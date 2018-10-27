#ifndef QTDATASYNC_EVENTCURSOR_P_H
#define QTDATASYNC_EVENTCURSOR_P_H

#include <QtSql/QSqlQuery>

#include "eventcursor.h"

#include "defaults.h"
#include "emitteradapter_p.h"

namespace QtDataSync {

class EventCursorPrivate
{
	friend class QtDataSync::EventCursor;

public:
	EventCursorPrivate(const QString &setupName, EventCursor *q_ptr);

	static bool isLogActive(const Defaults &defaults, DatabaseRef &database);
	static void initDatabase(const Defaults &defaults, DatabaseRef &database, Logger *logger, bool createTriggers);
	static void clearEventLog(const Defaults &defaults, DatabaseRef &database);

private:
	void exec(QSqlQuery &query, quint64 qIndex = 0) const;
	void readQuery(const QSqlQuery &query);
	void prepareNextQuery(QSqlQuery &query, bool withData) const;

	Defaults defaults;
	DatabaseRef database;
	EmitterAdapter *emitter;
	Logger *logger;

	quint64 index = 0;
	QtDataSync::ObjectKey key;
	bool wasRemoved = false;
	QDateTime timestamp;

	bool skipObsolete = true;
};

}

#endif // QTDATASYNC_EVENTCURSOR_P_H
