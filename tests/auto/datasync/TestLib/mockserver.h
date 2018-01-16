#ifndef MOCKSERVER_H
#define MOCKSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QtTest>

#include "mockconnection.h"

class MockServer : public QObject
{
	Q_OBJECT

public:
	explicit MockServer(QObject *parent = nullptr);

	void init();
	QUrl url() const;

	void clear();
	bool waitForConnected(MockConnection **connection, int timeout = 5000);

private:
	QWebSocketServer *_server;

	QSignalSpy _connectedSpy;
};


#endif // MOCKSERVER_H
