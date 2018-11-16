#ifndef QQMLEVENTCURSOR_H
#define QQMLEVENTCURSOR_H

#include <QtDataSync/EventCursor>

#ifdef DOXYGEN_RUN
namespace de::skycoder42::QtDataSync {

/*! @brief The QML bindings for the static methods of of QtDataSync::EventCursor
 *
 * @extends QtQml.QtObject
 * @since 4.2
 *
 * @sa QtDataSync::EventCursor
 */
class EventLog
#else
namespace QtDataSync {

class QQmlEventCursor : public QObject
#endif
{
	Q_OBJECT

public:
	//! @private
	explicit QQmlEventCursor(QObject *parent = nullptr);

	//! @copydoc QtDataSync::EventCursor::first(QObject*)
	Q_INVOKABLE QtDataSync::EventCursor *first(QObject *parent = nullptr);
	//! @copydoc QtDataSync::EventCursor::first(const QString &, QObject*)
	Q_INVOKABLE QtDataSync::EventCursor *first(const QString &setupName, QObject *parent = nullptr);
	//! @copydoc QtDataSync::EventCursor::last(QObject*)
	Q_INVOKABLE QtDataSync::EventCursor *last(QObject *parent = nullptr);
	//! @copydoc QtDataSync::EventCursor::last(const QString &, QObject*)
	Q_INVOKABLE QtDataSync::EventCursor *last(const QString &setupName, QObject *parent = nullptr);
	//! @copydoc QtDataSync::EventCursor::create(quint64, QObject*)
	Q_INVOKABLE QtDataSync::EventCursor *create(quint64 index, QObject *parent = nullptr);
	//! @copydoc QtDataSync::EventCursor::create(quint64, const QString &, QObject*)
	Q_INVOKABLE QtDataSync::EventCursor *create(quint64 index, const QString &setupName, QObject *parent = nullptr);
	//! @copydoc QtDataSync::EventCursor::load(const QByteArray &, QObject*)
	Q_INVOKABLE QtDataSync::EventCursor *load(const QByteArray &data, QObject *parent = nullptr);
	//! @copydoc QtDataSync::EventCursor::load(const QByteArray &, const QString &, QObject*)
	Q_INVOKABLE QtDataSync::EventCursor *load(const QByteArray &data, const QString &setupName, QObject *parent = nullptr);
};

}

#endif // QQMLEVENTCURSOR_H
