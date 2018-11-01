#ifndef QQMLEVENTCURSOR_H
#define QQMLEVENTCURSOR_H

#include <QtDataSync/EventCursor>

namespace QtDataSync {

class QQmlEventCursor : public QObject
{
	Q_OBJECT

public:
	explicit QQmlEventCursor(QObject *parent = nullptr);

	Q_INVOKABLE QtDataSync::EventCursor *first(QObject *parent = nullptr);
	Q_INVOKABLE QtDataSync::EventCursor *first(const QString &setupName, QObject *parent = nullptr);
	Q_INVOKABLE QtDataSync::EventCursor *last(QObject *parent = nullptr);
	Q_INVOKABLE QtDataSync::EventCursor *last(const QString &setupName, QObject *parent = nullptr);
	Q_INVOKABLE QtDataSync::EventCursor *create(quint64 index, QObject *parent = nullptr);
	Q_INVOKABLE QtDataSync::EventCursor *create(quint64 index, const QString &setupName, QObject *parent = nullptr);
	Q_INVOKABLE QtDataSync::EventCursor *load(const QByteArray &data, QObject *parent = nullptr);
	Q_INVOKABLE QtDataSync::EventCursor *load(const QByteArray &data, const QString &setupName, QObject *parent = nullptr);
};

}

#endif // QQMLEVENTCURSOR_H
