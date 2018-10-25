#ifndef QTDATASYNC_EVENTCURSOR_P_H
#define QTDATASYNC_EVENTCURSOR_P_H

#include <QtSql/QSqlQuery>

#include "eventcursor.h"

#include "defaults.h"

namespace QtDataSync {

class EventCursorPrivate
{
	friend class QtDataSync::EventCursor;

public:
	EventCursorPrivate(const QString &setupName, EventCursor *q_ptr);

	static void initDatabase(Defaults defaults, DatabaseRef &database, Logger *logger);
	static void clearEventLog(Defaults defaults, DatabaseRef &database);

private:
	void exec(QSqlQuery &query, quint64 index = 0) const;
	void readQuery(const QSqlQuery &query);

	Defaults defaults;
	DatabaseRef database;
	Logger *logger;

	quint64 index = 0;
	QtDataSync::ObjectKey key;
	bool wasRemoved = false;
	QDateTime timestamp;

	bool skipObsolete = true;
};

}

#endif // QTDATASYNC_EVENTCURSOR_P_H
