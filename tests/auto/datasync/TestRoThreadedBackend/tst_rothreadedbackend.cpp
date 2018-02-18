#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QtRemoteObjects>

#include <QtDataSync/private/exchangebuffer_p.h>

#include "rep_testclass_source.h"
#include "rep_testclass_replica.h"

using namespace QtDataSync;

class TestClass : public TestClassSimpleSource
{
	Q_OBJECT

public:
	TestClass(QObject *parent = nullptr);

public Q_SLOTS:
	void serverDo(int id) override;
};

class TestRoThreadedBackend : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();

	void testExchangeDevice();
	void testRemoteObjects();
};

void TestRoThreadedBackend::initTestCase()
{
#ifdef Q_OS_LINUX
	if(!qgetenv("LD_PRELOAD").contains("Qt5DataSync"))
		qWarning() << "No LD_PRELOAD set - this may fail on systems with multiple version of the modules";
#endif
}

void TestRoThreadedBackend::testExchangeDevice()
{
	ExchangeBuffer p1;
	ExchangeBuffer p2;

	QByteArray testMessage = "some text";

	QSignalSpy c1Spy(&p1, &ExchangeBuffer::partnerConnected);
	QSignalSpy d1Spy(&p1, &ExchangeBuffer::partnerDisconnected);
	QSignalSpy r1Spy(&p1, &ExchangeBuffer::readyRead);
	QSignalSpy c2Spy(&p2, &ExchangeBuffer::partnerConnected);
	QSignalSpy d2Spy(&p2, &ExchangeBuffer::partnerDisconnected);
	QSignalSpy r2Spy(&p2, &ExchangeBuffer::readyRead);

	//connect p1 -> p2
	QVERIFY(p1.connectTo(&p2));
	QCOMPARE(c1Spy.size(), 1);
	QVERIFY(c2Spy.wait());
	QCOMPARE(c2Spy.size(), 1);

	//write p1
	p1.write(testMessage);
	QVERIFY(r2Spy.wait());
	QCOMPARE(r1Spy.size(), 0);
	QCOMPARE(r2Spy.size(), 1);
	QCOMPARE(p1.bytesAvailable(), 0);
	QCOMPARE(p2.bytesAvailable(), testMessage.size());
	QCOMPARE(p2.read(testMessage.size()), testMessage);
	QCOMPARE(p2.bytesAvailable(), 0);

	//write p2
	p2.write(testMessage);
	QVERIFY(r1Spy.wait());
	QCOMPARE(r1Spy.size(), 1);
	QCOMPARE(r2Spy.size(), 1);
	QCOMPARE(p1.bytesAvailable(), testMessage.size());
	QCOMPARE(p2.bytesAvailable(), 0);
	QCOMPARE(p1.read(testMessage.size()), testMessage);
	QCOMPARE(p1.bytesAvailable(), 0);

	//disconnect p2 -> p1
	p2.close();
	QCOMPARE(d2Spy.size(), 1);
	QVERIFY(d1Spy.wait());
	QCOMPARE(d1Spy.size(), 1);
}

void TestRoThreadedBackend::testRemoteObjects()
{
	QUrl url(QStringLiteral("threaded:///some/path"));

	QRemoteObjectHost host(url);
	host.enableRemoting(new TestClass(&host));

	QRemoteObjectNode node;
	QVERIFY(node.connectToNode(url));

	auto test = node.acquire<TestClassReplica>();
	QVERIFY(test);
	if(!test->currState()) {
		QSignalSpy stateSpy(test, &TestClassReplica::currStateChanged);
		QVERIFY(stateSpy.wait());
		QCOMPARE(stateSpy.size(), 1);
		QVERIFY(stateSpy.takeFirst()[0].toBool());
	}

	QSignalSpy doneSpy(test, &TestClassReplica::serverDone);
	test->serverDo(42);
	QVERIFY(doneSpy.wait());
	QCOMPARE(doneSpy.size(), 1);
	QCOMPARE(doneSpy.takeFirst()[0].toInt(), 42);

	test->serverDo(43);
	QVERIFY(doneSpy.wait());
	QCOMPARE(doneSpy.size(), 1);
	QCOMPARE(doneSpy.takeFirst()[0].toInt(), 43);
}



TestClass::TestClass(QObject *parent) :
	TestClassSimpleSource(parent)
{
	pushCurrState(true);
}

void TestClass::serverDo(int id)
{
	emit serverDone(id);
}

QTEST_MAIN(TestRoThreadedBackend)

#include "tst_rothreadedbackend.moc"
