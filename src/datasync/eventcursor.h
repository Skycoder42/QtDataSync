#ifndef QTDATASYNC_EVENTCURSOR_H
#define QTDATASYNC_EVENTCURSOR_H

#include <functional>

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qbytearray.h>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/objectkey.h"
#include "QtDataSync/exception.h"

namespace QtDataSync {

class EventCursorPrivate;
//! A cursor style class to read from the global change event log
class Q_DATASYNC_EXPORT EventCursor : public QObject
{
	Q_OBJECT

	//! Holds whether the cursor is positioned on a valid record
	Q_PROPERTY(bool valid READ isValid CONSTANT)
	//! Holds the name of the setup this cursor operates on
	Q_PROPERTY(QString setupName READ setupName CONSTANT)

	//! Holds the current position of the cursor
	Q_PROPERTY(quint64 index READ index NOTIFY indexChanged USER true)
	//! Holds the key of the dataset this cursor is positioned on
	Q_PROPERTY(QtDataSync::ObjectKey key READ key NOTIFY keyChanged)
	//! Holds the information whether the dataset was changed or removed
	Q_PROPERTY(bool wasRemoved READ wasRemoved NOTIFY wasRemovedChanged)
	//! Provides the timestamp of when the change occured
	Q_PROPERTY(QDateTime timestamp READ timestamp NOTIFY timestampChanged)

	//! Specify if the cursor should automatically skip obsolete changes when advancing
	Q_PROPERTY(bool skipObsolete READ skipObsolete WRITE setSkipObsolete NOTIFY skipObsoleteChanged)

public:
	~EventCursor() override;

	//! Checks if the eventlog is currently beeing collected or not
	static bool isEventLogActive(const QString &setupName = DefaultSetup);

	//! Create a cursor positioned on the oldest change event
	static EventCursor *first(QObject *parent = nullptr);
	//! @copybrief EventCursor::first(QObject*)
	static EventCursor *first(const QString &setupName, QObject *parent = nullptr);
	//! Create a cursor positioned on the newest change event
	static EventCursor *last(QObject *parent = nullptr);
	//! @copybrief EventCursor::last(QObject*)
	static EventCursor *last(const QString &setupName, QObject *parent = nullptr);
	//! Create a cursor positioned on the given index, if it is a valid index
	static EventCursor *create(quint64 index, QObject *parent = nullptr);
	//! @copybrief EventCursor::create(quint64, QObject*)
	static EventCursor *create(quint64 index, const QString &setupName, QObject *parent = nullptr);
	//! Create a cursor positioned on the index provided by the previously stored data, if still valid
	static EventCursor *load(const QByteArray &data, QObject *parent = nullptr);
	//! @copybrief EventCursor::load(const QByteArray &, QObject*)
	static EventCursor *load(const QByteArray &data, const QString &setupName, QObject *parent = nullptr);
	//! Returns encoded position data of the cursor to be loaded again
	Q_INVOKABLE QByteArray save() const;

	//! @readAcFn{EventCursor::valid}
	bool isValid() const;
	//! @readAcFn{EventCursor::setupName}
	QString setupName() const;

	//! @readAcFn{EventCursor::index}
	quint64 index() const;
	//! @readAcFn{EventCursor::key}
	QtDataSync::ObjectKey key() const;
	//! @readAcFn{EventCursor::wasRemoved}
	bool wasRemoved() const;
	//! @readAcFn{EventCursor::timestamp}
	QDateTime timestamp() const;

	//! @readAcFn{EventCursor::skipObsolete}
	bool skipObsolete() const;

	//! Checks if there is a newer change event after the one this cursor points to
	Q_INVOKABLE bool hasNext() const;
	//! Advances the cursor to the next newer change event, if one exists
	Q_INVOKABLE bool next();

	//! Automatically scans though all change events
	Q_INVOKABLE void autoScanLog();
	//! Automatically scans though all change events, calling the given function on each event
	void autoScanLog(std::function<bool(const EventCursor *)> function, bool scanCurrent = true);
	//! @copybrief EventCursor::autoScanLog(std::function<bool(const EventCursor *)>, bool)
	void autoScanLog(QObject *scope, std::function<bool(const EventCursor *)> function, bool scanCurrent = true);
	//! @copydoc EventCursor::autoScanLog(QObject *, std::function<bool(const EventCursor *)>, bool)
	template <typename TClass>
	void autoScanLog(TClass *scope, bool(TClass::* function)(const EventCursor *), bool scanCurrent = true);

public Q_SLOTS:
	//! @writeAcFn{EventCursor::skipObsolete}
	void setSkipObsolete(bool skipObsolete);

	//! Clears all events from the log up to offset entries before the one this cursor points at
	void clearEventLog(quint64 offset = 0);

Q_SIGNALS:
	//! Is emitted whenever a new change event was added to the database
	void eventLogChanged(QPrivateSignal);

	//! @notifyAcFn{EventCursor::index}
	void indexChanged(quint64 index, QPrivateSignal);
	//! @notifyAcFn{EventCursor::key}
	void keyChanged(const QtDataSync::ObjectKey &key, QPrivateSignal);
	//! @notifyAcFn{EventCursor::wasRemoved}
	void wasRemovedChanged(bool wasRemoved, QPrivateSignal);
	//! @notifyAcFn{EventCursor::timestamp}
	void timestampChanged(const QDateTime &timestamp, QPrivateSignal);
	//! @notifyAcFn{EventCursor::skipObsolete}
	void skipObsoleteChanged(bool skipObsolete, QPrivateSignal);

private:
	friend class QtDataSync::EventCursorPrivate;
	QScopedPointer<EventCursorPrivate> d;

	explicit EventCursor(const QString &setupName, QObject *parent = nullptr);
};

//! Exception thrown from the event cursor if something goes wrong
class Q_DATASYNC_EXPORT EventCursorException : public Exception
{
public:
	//! @private
	EventCursorException(const Defaults &defaults,
						 quint64 index,
						 QString context,
						 const QString &message);

	//! The index of the change event on which something went wrong
	quint64 index() const;
	//! The context in which the error occured
	QString context() const;

	QByteArray className() const noexcept override;
	QString qWhat() const override;
	void raise() const override;
	QException *clone() const override;

protected:
	//! @private
	EventCursorException(const EventCursorException * const other);

	//! @private
	quint64 _index;
	//! @private
	QString _context;
};

// ------------- GENERIC IMPLEMENTATION -------------


template<typename TClass>
void EventCursor::autoScanLog(TClass *scope, bool (TClass::*function)(const EventCursor *), bool scanCurrent)
{
	static_assert(std::is_base_of<QObject, TClass>::value, "TClass must extend QObject");
	autoScanLog(scope, [scope, function](const EventCursor *cursor) -> bool {
		return (scope->*function)(cursor);
	}, scanCurrent);
}

}

#endif // QTDATASYNC_EVENTCURSOR_H
