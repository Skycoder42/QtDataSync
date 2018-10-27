#ifndef QTDATASYNC_EVENTCURSOR_H
#define QTDATASYNC_EVENTCURSOR_H

#include <functional>

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>

#include "QtDataSync/qtdatasync_global.h"
#include "QtDataSync/objectkey.h"
#include "QtDataSync/exception.h"

namespace QtDataSync {

class EventCursorPrivate;
class Q_DATASYNC_EXPORT EventCursor : public QObject
{
	Q_OBJECT

	Q_PROPERTY(quint64 index READ index WRITE setIndex NOTIFY indexChanged USER true)
	Q_PROPERTY(QtDataSync::ObjectKey key READ key WRITE setKey NOTIFY keyChanged STORED false)
	Q_PROPERTY(bool wasRemoved READ wasRemoved WRITE setWasRemoved NOTIFY wasRemovedChanged STORED false)
	Q_PROPERTY(QDateTime timestamp READ timestamp WRITE setTimestamp NOTIFY timestampChanged STORED false)

	Q_PROPERTY(bool skipObsolete READ skipObsolete WRITE setSkipObsolete NOTIFY skipObsoleteChanged)

public:
	~EventCursor() override;

	static bool isEventLogActive(const QString &setupName = DefaultSetup);

	static EventCursor *first(QObject *parent = nullptr);
	static EventCursor *first(const QString &setupName, QObject *parent = nullptr);
	static EventCursor *last(QObject *parent = nullptr);
	static EventCursor *last(const QString &setupName, QObject *parent = nullptr);
	static EventCursor *create(quint64 index, QObject *parent = nullptr);
	static EventCursor *create(quint64 index, const QString &setupName, QObject *parent = nullptr);
	static EventCursor *load(const QByteArray &data, QObject *parent = nullptr);
	static EventCursor *load(const QByteArray &data, const QString &setupName, QObject *parent = nullptr);
	QByteArray save() const;

	bool isValid() const;

	quint64 index() const;
	QtDataSync::ObjectKey key() const;
	bool wasRemoved() const;
	QDateTime timestamp() const;

	bool skipObsolete() const;

	bool hasNext() const;
	bool next();

	void autoScanLog();
	template <typename TFunc>
	void autoScanLog(TFunc &&function, bool scanCurrent = true);
	template <typename TFunc>
	void autoScanLog(QObject *scope, TFunc &&function, bool scanCurrent = true);
	template <typename TClass>
	void autoScanLog(TClass *scope, bool(TClass::* function)(const EventCursor *), bool scanCurrent = true);

public Q_SLOTS:
	void setSkipObsolete(bool skipObsolete);
	void clearEventLog(quint64 offset = 0);

Q_SIGNALS:
	void eventLogChanged(QPrivateSignal);

	void indexChanged(quint64 index, QPrivateSignal);
	void keyChanged(const QtDataSync::ObjectKey &key, QPrivateSignal);
	void wasRemovedChanged(bool wasRemoved, QPrivateSignal);
	void timestampChanged(const QDateTime &timestamp, QPrivateSignal);
	void skipObsoleteChanged(bool skipObsolete, QPrivateSignal);

private:
	friend class QtDataSync::EventCursorPrivate;
	QScopedPointer<EventCursorPrivate> d;

	explicit EventCursor(const QString &setupName, QObject *parent = nullptr);

	void setIndex(quint64 index);
	void setKey(QtDataSync::ObjectKey key);
	void setWasRemoved(bool wasRemoved);
	void setTimestamp(QDateTime timestamp);

	void scanLogImpl(QObject *scope, std::function<bool(const EventCursor *)> function, bool scanCurrent);
};

class Q_DATASYNC_EXPORT EventCursorException : public Exception
{
public:
	EventCursorException(const Defaults &defaults,
						 quint64 index,
						 QString context,
						 const QString &message);

	quint64 index() const;
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

template<typename TFunc>
void EventCursor::autoScanLog(TFunc &&function, bool scanCurrent)
{
	scanLogImpl(this, std::forward<TFunc>(function), scanCurrent);
}

template<typename TFunc>
void EventCursor::autoScanLog(QObject *scope, TFunc &&function, bool scanCurrent)
{
	scanLogImpl(scope, std::forward<TFunc>(function), scanCurrent);
}

template<typename TClass>
void EventCursor::autoScanLog(TClass *scope, bool (TClass::*function)(const EventCursor *), bool scanCurrent)
{
	static_assert (std::is_base_of<QObject, TClass>::value, "TClass must extend QObject");
	scanLogImpl(scope, [scope, function](const EventCursor *cursor) -> bool {
		return (scope->*function)(cursor);
	}, scanCurrent);
}

}

#endif // QTDATASYNC_EVENTCURSOR_H
