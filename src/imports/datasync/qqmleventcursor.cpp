#include "qqmleventcursor.h"
#include <QtQml>
using namespace QtDataSync;

QQmlEventCursor::QQmlEventCursor(QObject *parent) :
	QObject{parent}
{}

EventCursor *QQmlEventCursor::first(QObject *parent)
{
	try {
		return EventCursor::first(parent);
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
		return nullptr;
	}
}

EventCursor *QQmlEventCursor::first(const QString &setupName, QObject *parent)
{
	try {
		return EventCursor::first(setupName, parent);
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
		return nullptr;
	}
}

EventCursor *QQmlEventCursor::last(QObject *parent)
{
	try {
		return EventCursor::last(parent);
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
		return nullptr;
	}
}

EventCursor *QQmlEventCursor::last(const QString &setupName, QObject *parent)
{
	try {
		return EventCursor::last(setupName, parent);
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
		return nullptr;
	}
}

EventCursor *QQmlEventCursor::create(quint64 index, QObject *parent)
{
	try {
		return EventCursor::create(index, parent);
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
		return nullptr;
	}
}

EventCursor *QQmlEventCursor::create(quint64 index, const QString &setupName, QObject *parent)
{
	try {
		return EventCursor::create(index, setupName, parent);
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
		return nullptr;
	}
}

EventCursor *QQmlEventCursor::load(const QByteArray &data, QObject *parent)
{
	try {
		return EventCursor::load(data, parent);
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
		return nullptr;
	}
}

EventCursor *QQmlEventCursor::load(const QByteArray &data, const QString &setupName, QObject *parent)
{
	try {
		return EventCursor::load(data, setupName, parent);
	} catch(Exception &e) {
		qmlWarning(this) << e.what();
		return nullptr;
	}
}
