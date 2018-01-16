#ifndef MOCKCLIENT_H
#define MOCKCLIENT_H

#include <QObject>

#include "mockconnection.h"

class MockClient : public MockConnection
{
	Q_OBJECT

public:
	explicit MockClient(QObject *parent = nullptr);

	bool waitForConnected(quint16 port = 14242);

private:
	QSignalSpy _connectSpy;
};

#endif // MOCKCLIENT_H
