#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QProcess>

#ifdef Q_OS_UNIX
#include <sys/types.h>
#include <signal.h>
#elif Q_OS_WIN
#include <qt_windows.h>
#endif


class TestAppServer : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testStart();
	void testStop();

private:
	QProcess *server;
};

void TestAppServer::initTestCase()
{
	QByteArray confPath { SETUP_FILE };
	QVERIFY(QFile::exists(QString::fromUtf8(confPath)));
	qputenv("QDSAPP_CONFIG_FILE", confPath);

#ifdef Q_OS_UNIX
	QString binPath { QStringLiteral(BUILD_BIN_DIR "qdsappd") };
#elif Q_OS_WIN
	QString binPath { QStringLiteral(BUILD_BIN_DIR "qdsappsvc") };
#else
	QString binPath { QStringLiteral(BUILD_BIN_DIR "qdsapp") };
#endif
	QVERIFY(QFile::exists(binPath));

	server = new QProcess(this);
	server->setProgram(binPath);
	server->setProcessChannelMode(QProcess::ForwardedErrorChannel);
}

void TestAppServer::cleanupTestCase()
{
	if(server->isOpen()) {
		server->kill();
		QVERIFY(server->waitForFinished(5000));
	}
	server->deleteLater();
}

void TestAppServer::testStart()
{
	server->start();
	QVERIFY(server->waitForStarted(5000));
	QVERIFY(!server->waitForFinished(5000));
}

void TestAppServer::testStop()
{
	//send a signal to stop
#ifdef Q_OS_UNIX
	server->terminate(); //same as kill(SIGTERM)
#elif Q_OS_WIN
	GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, server->processId());
#endif
	QVERIFY(server->waitForFinished(5000));
	QCOMPARE(server->exitStatus(), QProcess::NormalExit);
	QCOMPARE(server->exitCode(), 0);
	server->close();
}

QTEST_MAIN(TestAppServer)

#include "tst_appserver.moc"
