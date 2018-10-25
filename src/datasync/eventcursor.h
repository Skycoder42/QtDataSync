#ifndef QTDATASYNC_EVENTCURSOR_H
#define QTDATASYNC_EVENTCURSOR_H

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

	Q_PROPERTY(quint64 index READ index WRITE setIndex USER true)
	Q_PROPERTY(QtDataSync::ObjectKey key READ key WRITE setKey)
	Q_PROPERTY(bool wasRemoved READ wasRemoved WRITE setWasRemoved)
	Q_PROPERTY(QDateTime timestamp READ timestamp WRITE setTimestamp)

	Q_PROPERTY(bool skipObsolete READ skipObsolete WRITE setSkipObsolete)

public:
	~EventCursor() override;

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
	void setSkipObsolete(bool skipObsolete);

	bool hasNext() const;
	bool next();

	void clearEventLog(quint64 offset = 0);

private:
	friend class QtDataSync::EventCursorPrivate;
	QScopedPointer<EventCursorPrivate> d;

	explicit EventCursor(const QString &setupName, QObject *parent = nullptr);

	void setIndex(quint64 index);
	void setKey(QtDataSync::ObjectKey key);
	void setWasRemoved(bool wasRemoved);
	void setTimestamp(QDateTime timestamp);
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

}

#endif // QTDATASYNC_EVENTCURSOR_H
