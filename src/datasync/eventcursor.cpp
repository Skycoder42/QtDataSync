#include "eventcursor.h"
#include "eventcursor_p.h"
#include "defaults_p.h"
using namespace QtDataSync;

#define QTDATASYNC_LOG d->logger

EventCursor::EventCursor(const QString &setupName, QObject *parent) :
	QObject{parent},
	d{new EventCursorPrivate{setupName, this}}
{}

EventCursor::~EventCursor() = default;

EventCursor *EventCursor::first(QObject *parent)
{
	return first(DefaultSetup, parent);
}

EventCursor *EventCursor::first(const QString &setupName, QObject *parent)
{
	auto cursor = new EventCursor{setupName, parent};
	QSqlQuery eventQuery{cursor->d->database};
	eventQuery.prepare(QStringLiteral("SELECT SeqId, Type, Key, Removed, Timestamp "
									  "FROM EventLog "
									  "ORDER BY SeqId ASC "
									  "LIMIT 1"));
	cursor->d->exec(eventQuery);
	if(eventQuery.first())
		cursor->d->readQuery(eventQuery);
	return cursor;
}

EventCursor *EventCursor::last(QObject *parent)
{
	return last(DefaultSetup, parent);
}

EventCursor *EventCursor::last(const QString &setupName, QObject *parent)
{
	auto cursor = new EventCursor{setupName, parent};
	QSqlQuery eventQuery{cursor->d->database};
	eventQuery.prepare(QStringLiteral("SELECT SeqId, Type, Key, Removed, Timestamp "
									  "FROM EventLog "
									  "ORDER BY SeqId DESC "
									  "LIMIT 1"));
	cursor->d->exec(eventQuery);
	if(eventQuery.first())
		cursor->d->readQuery(eventQuery);
	return cursor;
}

EventCursor *EventCursor::create(quint64 index, QObject *parent)
{
	return create(index, DefaultSetup, parent);
}

EventCursor *EventCursor::create(quint64 index, const QString &setupName, QObject *parent)
{
	auto cursor = new EventCursor{setupName, parent};
	QSqlQuery eventQuery{cursor->d->database};
	eventQuery.prepare(QStringLiteral("SELECT SeqId, Type, Key, Removed, Timestamp "
									  "FROM EventLog "
									  "WHERE SeqId = ? "
									  "LIMIT 1"));
	eventQuery.addBindValue(index);
	cursor->d->exec(eventQuery);
	if(eventQuery.first())
		cursor->d->readQuery(eventQuery);
	return cursor;
}

EventCursor *EventCursor::load(const QByteArray &data, QObject *parent)
{
	return load(data, DefaultSetup, parent);
}

EventCursor *EventCursor::load(const QByteArray &data, const QString &setupName, QObject *parent)
{
	Q_UNIMPLEMENTED();
	return nullptr;
}

QByteArray EventCursor::save() const
{
	Q_UNIMPLEMENTED();
	return {};
}

bool EventCursor::isValid() const
{
	return d->defaults.isValid() && d->index != 0;
}

quint64 EventCursor::index() const
{
	return d->index;
}

ObjectKey EventCursor::key() const
{
	return d->key;
}

bool EventCursor::wasRemoved() const
{
	return d->wasRemoved;
}

QDateTime EventCursor::timestamp() const
{
	return d->timestamp;
}

bool EventCursor::skipObsolete() const
{
	return d->skipObsolete;
}

void EventCursor::setSkipObsolete(bool skipObsolete)
{
	d->skipObsolete = skipObsolete;
}

bool EventCursor::hasNext() const
{
	Q_UNIMPLEMENTED();
	return false;
}

bool EventCursor::next()
{
	Q_UNIMPLEMENTED();
	return false;
}

void EventCursor::clearEventLog(quint64 offset)
{
	Q_UNIMPLEMENTED();
}

void EventCursor::setIndex(quint64 index)
{
	d->index = index;
}

void EventCursor::setKey(ObjectKey key)
{
	d->key = std::move(key);
}

void EventCursor::setWasRemoved(bool wasRemoved)
{
	d->wasRemoved = wasRemoved;
}

void EventCursor::setTimestamp(QDateTime timestamp)
{
	d->timestamp = std::move(timestamp);
}



EventCursorException::EventCursorException(const Defaults &defaults, quint64 index, QString context, const QString &message) :
	Exception{defaults, message},
	_index{index},
	_context{std::move(context)}
{}

quint64 EventCursorException::index() const
{
	return _index;
}

QString EventCursorException::context() const
{
	return _context;
}

EventCursorException::EventCursorException(const EventCursorException * const other) :
	Exception{other},
	_index{other->_index},
	_context{other->_context}
{}

QByteArray EventCursorException::className() const noexcept
{
	return QTDATASYNC_EXCEPTION_NAME(EventCursorException);
}

QString EventCursorException::qWhat() const
{
	auto msg = Exception::qWhat();
	if(_index != 0)
		msg += QStringLiteral("\n\tIndex: %L1").arg(_index);
	msg += QStringLiteral("\n\tContext: %1").arg(_context);
	return msg;
}

void EventCursorException::raise() const
{
	throw *this;
}

QException *EventCursorException::clone() const
{
	return new EventCursorException{this};
}

// ------------- PRIVATE IMPLEMENTATION -------------

#undef QTDATASYNC_LOG
#define QTDATASYNC_LOG logger

EventCursorPrivate::EventCursorPrivate(const QString &setupName, EventCursor *q_ptr) :
	defaults{DefaultsPrivate::obtainDefaults(setupName)},
	database{defaults.aquireDatabase(q_ptr)},
	logger{defaults.createLogger("eventlogger", q_ptr)}
{}

void EventCursorPrivate::initDatabase(Defaults defaults, DatabaseRef &database, Logger *logger)
{
	if(!database->tables().contains(QStringLiteral("EventLog"))) {
		QSqlQuery createQuery{database};
		createQuery.prepare(QStringLiteral("CREATE TABLE IF NOT EXISTS EventLog ( "
										   "	SeqId		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
										   "	Type		TEXT NOT NULL, "
										   "	Id			TEXT NOT NULL, "
										   "	Version		INTEGER NOT NULL, "
										   "	Removed		INTEGER NOT NULL, "
										   "	Timestamp	INTEGER NOT NULL "
										   ");"));
		if(!createQuery.exec()) {
			throw EventCursorException {
				defaults,
				0,
				createQuery.executedQuery().simplified(),
				createQuery.lastError().text()
			};
		}
		logDebug() << "Created EventLog table";
	}

	for(const auto &type : {std::make_pair(QStringLiteral("INSERT"), false),
							std::make_pair(QStringLiteral("UPDATE"), true)}) {
		QSqlQuery hasTriggersQuery{database};
		hasTriggersQuery.prepare(QStringLiteral("SELECT 1 FROM sqlite_master "
												 "WHERE type = ? AND name = ?"));
		hasTriggersQuery.addBindValue(QStringLiteral("trigger"));
		hasTriggersQuery.addBindValue(QStringLiteral("eventlog_%1").arg(type.first));
		if(!hasTriggersQuery.exec()) {
			throw EventCursorException {
				defaults,
				0,
				hasTriggersQuery.executedQuery().simplified(),
				hasTriggersQuery.lastError().text()
			};
		}

		if(!hasTriggersQuery.first()) {
			QSqlQuery createTriggerQuery{database};
			auto query = QStringLiteral("CREATE TRIGGER IF NOT EXISTS eventlog_%1 "
										"AFTER %1 "
										"ON DataIndex "
										"%2"
										"BEGIN "
										"	INSERT INTO EventLog "
										"	(Type, Id, Version, Removed, Timestamp) "
										"	VALUES(NEW.Type, NEW.Id, NEW.Version, NEW.File IS NULL, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'));"
										"END;").arg(type.first);
			if(type.second)
				query = query.arg(QStringLiteral("WHEN NEW.Version != OLD.Version "));
			else
				query = query.arg(QString{});
			createTriggerQuery.prepare(query);
			if(!createTriggerQuery.exec()) {
				throw EventCursorException {
					defaults,
					0,
					createTriggerQuery.executedQuery().simplified(),
					createTriggerQuery.lastError().text()
				};
			}
			logDebug() << "Created eventlog trigger for" << type.first << "operation";
		}
	}
}

void EventCursorPrivate::clearEventLog(Defaults defaults, DatabaseRef &database)
{
	QSqlQuery eventQuery(database);
	eventQuery.prepare(QStringLiteral("DELETE FROM EventLog"));
	if(!eventQuery.exec()) {
		throw EventCursorException {
			defaults,
			0,
			eventQuery.executedQuery().simplified(),
			eventQuery.lastError().text()
		};
	}
}

void EventCursorPrivate::exec(QSqlQuery &query, quint64 index) const
{

}

void EventCursorPrivate::readQuery(const QSqlQuery &query)
{
	index = query.value(0).toULongLong();
	key.typeName = query.value(1).toByteArray();
	key.id = query.value(2).toString();
	wasRemoved = query.value(3).toBool();
	timestamp = query.value(4).toDateTime().toLocalTime();
}
