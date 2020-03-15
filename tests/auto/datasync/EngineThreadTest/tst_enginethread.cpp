#include <QtTest>
#include <QtDataSync/EngineThread>
#include <QtDataSync/Setup>
#include "anonauth.h"
#include "testlib.h"
using namespace QtDataSync;
using namespace std::chrono_literals;

#define VERIFY_SPY(sigSpy, errorSpy) QVERIFY2(sigSpy.wait(), qUtf8Printable(QStringLiteral("Error on table: ") + errorSpy.value(0).value(0).toString()))

class EngineThreadTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void createEngineThread();
	void deleteAccount();
	void stopEngineThread();

private:
	using EngineState = Engine::EngineState;
	EngineThread *_engineThread = nullptr;
};

void EngineThreadTest::initTestCase()
{
	qRegisterMetaType<Engine::EngineState>("Engine::EngineState");
}

void EngineThreadTest::cleanupTestCase()
{
	try {
		if (_engineThread) {
			if (_engineThread->isRunning()) {
				_engineThread->quit();
				_engineThread->wait(5000);
			}
			_engineThread->deleteLater();
		}
	} catch(std::exception &e) {
		QFAIL(e.what());
	}

}

void EngineThreadTest::createEngineThread()
{
	try {
		auto called = false;
		auto initFn = [&](Engine *engine, QThread *thread) {
			QVERIFY(engine);
			QCOMPARE(thread, _engineThread);
			engine->start();
			called = true;
		};

		_engineThread = Setup<AnonAuth>()
							.setFirebaseProjectId(QStringLiteral(FIREBASE_PROJECT_ID))
							.setFirebaseApiKey(QStringLiteral(FIREBASE_API_KEY))
							.setSettings(TestLib::createSettings(nullptr))
							.createThreadedEngine(initFn, this);
		QVERIFY(_engineThread);
		QCOMPARE(called, false);
		QVERIFY(!_engineThread->engine());

		const auto watcher = _engineThread->createWatcher(this);
		QVERIFY(watcher.isStarted());
		QVERIFY(watcher.isRunning());
		QVERIFY(!watcher.isFinished());


		QSignalSpy engineSpy{_engineThread, &EngineThread::engineCreated};
		QVERIFY(engineSpy.isValid());

		_engineThread->start();
		QTRY_COMPARE(called, true);
		QTRY_VERIFY(_engineThread->engine());
		QTRY_VERIFY(watcher.isFinished());
		QVERIFY(watcher.result());
		QCOMPARE(engineSpy.size(), 1);
		QCOMPARE(engineSpy[0][0].value<Engine*>(), _engineThread->engine());

		const auto engine = _engineThread->engine();
		QTRY_COMPARE(engine->state(), EngineState::TableSync);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void EngineThreadTest::deleteAccount()
{
	const auto engine = _engineThread->engine();
	QVERIFY(engine);
	QCOMPARE(engine->state(), EngineState::TableSync);
	QSignalSpy stateSpy{engine, &Engine::stateChanged};
	QVERIFY(stateSpy.isValid());

	const auto auth = static_cast<AnonAuth*>(engine->authenticator());
	QVERIFY(auth);
	auth->block = true;
	QMetaObject::invokeMethod(engine,
							  "deleteAccount",
							  Qt::QueuedConnection);
	QVERIFY(stateSpy.wait());
	QTRY_COMPARE(stateSpy.last()[0].value<Engine::EngineState>(), EngineState::SigningIn);
}

void EngineThreadTest::stopEngineThread()
{
	QVERIFY(_engineThread->engine());
	QVERIFY(_engineThread->isRunning());

	_engineThread->quit();
	QVERIFY(_engineThread->wait(5000));

	QVERIFY(!_engineThread->engine());
}

QTEST_MAIN(EngineThreadTest)

#include "tst_enginethread.moc"
