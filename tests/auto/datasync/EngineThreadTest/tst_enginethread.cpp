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

private:
	EngineThread *_engineThread = nullptr;
};

void EngineThreadTest::initTestCase()
{
}

void EngineThreadTest::cleanupTestCase()
{
	try {
		if (_engineThread) {
			if (_engineThread->isRunning()) {
				const auto engine = _engineThread->engine();
				QSignalSpy stateSpy{engine, &Engine::stateChanged};
				QVERIFY(stateSpy.isValid());
				QMetaObject::invokeMethod(engine,
										  "deleteAccount",
										  Qt::QueuedConnection);
				for (auto i = 0; i < 50; ++i) {
					if (stateSpy.wait(100)) {
						if (stateSpy.last()[0].value<Engine::EngineState>() == Engine::EngineState::Inactive) {
							qDebug() << "deleted account";
							return;
						}
					}
				}
				qDebug() << "failed to delete account";

				_engineThread->quit();
				QVERIFY(_engineThread->wait(5000));
			} else
				qDebug() << "engine thread inactive - not deleting account";

			_engineThread->deleteLater();
		} else
			qDebug() << "engine thread inactive - not deleting account";

	} catch(std::exception &e) {
		QFAIL(e.what());
	}

}

void EngineThreadTest::createEngineThread()
{
	auto called = false;
	auto initFn = [&](Engine *engine, QThread *thread) {
		QVERIFY(engine);
		QCOMPARE(thread, _engineThread);
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

	_engineThread->start();
	QTRY_COMPARE(called, true);
	QVERIFY(_engineThread->engine());
}

QTEST_MAIN(EngineThreadTest)

#include "tst_enginethread.moc"
