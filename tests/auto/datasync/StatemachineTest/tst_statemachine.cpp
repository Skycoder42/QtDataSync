#include <QtTest>
#include <QtDataSync>

#include "enginestatemachine.h"

#define TEST_STATES(...) \
do {\
	QVERIFY(idleSpy.wait()); \
	QStringList states {__VA_ARGS__}; \
	if (!QTest::qCompare(statemachine->activeStateNames(false), states, "statemachine->activeStateNames(false)", #__VA_ARGS__, __FILE__, __LINE__))\
		return;\
} while (false)

class StatemachineTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();

	void testNormalRun();
	void testDeleteTable();
	void testLiveSync();
	void testLogout();
	void testDeleteAcc();
	void testError();

private:
	EngineStateMachine *statemachine;
};

void StatemachineTest::initTestCase()
{
	statemachine = new EngineStateMachine{this};
	QVERIFY(statemachine->init());
}

void StatemachineTest::testNormalRun()
{
	QSignalSpy idleSpy{statemachine, &EngineStateMachine::reachedStableState};

	statemachine->stop();
	statemachine->start();
	TEST_STATES(QStringLiteral("Inactive"));

	statemachine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));

	statemachine->submitEvent(QStringLiteral("signedIn"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("DelTables"));

	statemachine->submitEvent(QStringLiteral("delDone"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcWaiting"));

	statemachine->submitEvent(QStringLiteral("dlContinue"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcWaiting"));

	statemachine->submitEvent(QStringLiteral("dataReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"));

	statemachine->submitEvent(QStringLiteral("procContinue"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"));

	statemachine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"),
				QStringLiteral("DlDone"));

	statemachine->submitEvent(QStringLiteral("triggerSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"),
				QStringLiteral("DlRunning"));

	statemachine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"),
				QStringLiteral("DlDone"));

	statemachine->submitEvent(QStringLiteral("procReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("DlReady"));

	statemachine->submitEvent(QStringLiteral("triggerUpload"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Uploading"));

	statemachine->submitEvent(QStringLiteral("ulContinue"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Uploading"));

	statemachine->submitEvent(QStringLiteral("triggerSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcWaiting"));

	statemachine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("DlReady"));

	statemachine->submitEvent(QStringLiteral("triggerUpload"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Uploading"));

	statemachine->submitEvent(QStringLiteral("syncReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronized"));

	statemachine->submitEvent(QStringLiteral("triggerUpload"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Uploading"));

	statemachine->submitEvent(QStringLiteral("syncReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronized"));

	statemachine->submitEvent(QStringLiteral("triggerSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcWaiting"));

	statemachine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("DlReady"));

	statemachine->submitEvent(QStringLiteral("triggerUpload"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Uploading"));

	statemachine->submitEvent(QStringLiteral("syncReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronized"));

	statemachine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Inactive"));
}

void StatemachineTest::testDeleteTable()
{
	QSignalSpy idleSpy{statemachine, &EngineStateMachine::reachedStableState};

	statemachine->stop();
	statemachine->start();
	TEST_STATES(QStringLiteral("Inactive"));

	statemachine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));

	statemachine->submitEvent(QStringLiteral("signedIn"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("DelTables"));

	statemachine->submitEvent(QStringLiteral("delContinue"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("DelTables"));

	statemachine->submitEvent(QStringLiteral("delDone"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcWaiting"));

	statemachine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("DlReady"));

	statemachine->submitEvent(QStringLiteral("triggerUpload"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Uploading"));

	statemachine->submitEvent(QStringLiteral("syncReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronized"));

	statemachine->submitEvent(QStringLiteral("delTable"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("DelTables"));

	statemachine->submitEvent(QStringLiteral("delContinue"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("DelTables"));

	statemachine->submitEvent(QStringLiteral("delDone"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcWaiting"));

	statemachine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Inactive"));
}

void StatemachineTest::testLiveSync()
{
	QSignalSpy idleSpy{statemachine, &EngineStateMachine::reachedStableState};

	statemachine->stop();
	statemachine->start();
	TEST_STATES(QStringLiteral("Inactive"));

	statemachine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));

	statemachine->submitEvent(QStringLiteral("signedIn"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("DelTables"));

	statemachine->submitEvent(QStringLiteral("delContinue"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("DelTables"));

	statemachine->submitEvent(QStringLiteral("delDone"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcWaiting"));

	statemachine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("DlReady"));

	statemachine->submitEvent(QStringLiteral("triggerLiveSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("LiveSync"),
				QStringLiteral("UlFiber"),
				QStringLiteral("UlRunning"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsRunning"));

	statemachine->submitEvent(QStringLiteral("procContinue"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("LiveSync"),
				QStringLiteral("UlFiber"),
				QStringLiteral("UlRunning"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsRunning"));

	statemachine->submitEvent(QStringLiteral("procReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("LiveSync"),
				QStringLiteral("UlFiber"),
				QStringLiteral("UlRunning"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsWaiting"));

	statemachine->submitEvent(QStringLiteral("dataReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("LiveSync"),
				QStringLiteral("UlFiber"),
				QStringLiteral("UlRunning"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsRunning"));

	statemachine->submitEvent(QStringLiteral("ulContinue"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("LiveSync"),
				QStringLiteral("UlFiber"),
				QStringLiteral("UlRunning"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsRunning"));

	statemachine->submitEvent(QStringLiteral("syncReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("LiveSync"),
				QStringLiteral("UlFiber"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsRunning"),
				QStringLiteral("UlWaiting"));

	statemachine->submitEvent(QStringLiteral("procReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("LiveSync"),
				QStringLiteral("UlFiber"),
				QStringLiteral("LsFiber"),
				QStringLiteral("UlWaiting"),
				QStringLiteral("LsWaiting"));

	statemachine->submitEvent(QStringLiteral("triggerUpload"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("LiveSync"),
				QStringLiteral("UlFiber"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsWaiting"),
				QStringLiteral("UlRunning"));

	statemachine->submitEvent(QStringLiteral("forceSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcWaiting"));

	statemachine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("DlReady"));

	statemachine->submitEvent(QStringLiteral("triggerLiveSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("LiveSync"),
				QStringLiteral("UlFiber"),
				QStringLiteral("UlRunning"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsRunning"));

	statemachine->submitEvent(QStringLiteral("delTable"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("DelTables"));

	statemachine->submitEvent(QStringLiteral("delDone"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcWaiting"));

	statemachine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("DlReady"));

	statemachine->submitEvent(QStringLiteral("triggerLiveSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("LiveSync"),
				QStringLiteral("UlFiber"),
				QStringLiteral("UlRunning"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsRunning"));

	statemachine->submitEvent(QStringLiteral("stopLiveSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronized"));

	statemachine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Inactive"));
}

void StatemachineTest::testLogout()
{
	QSignalSpy idleSpy{statemachine, &EngineStateMachine::reachedStableState};

	statemachine->stop();
	statemachine->start();
	TEST_STATES(QStringLiteral("Inactive"));

	statemachine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));

	statemachine->submitEvent(QStringLiteral("signedIn"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("DelTables"));

	statemachine->submitEvent(QStringLiteral("loggedOut"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));

	statemachine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Inactive"));
}

void StatemachineTest::testDeleteAcc()
{
	QSignalSpy idleSpy{statemachine, &EngineStateMachine::reachedStableState};

	statemachine->stop();
	statemachine->start();
	TEST_STATES(QStringLiteral("Inactive"));

	statemachine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));

	statemachine->submitEvent(QStringLiteral("signedIn"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("DelTables"));

	statemachine->submitEvent(QStringLiteral("delDone"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcWaiting"));

	statemachine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("DlReady"));

	statemachine->submitEvent(QStringLiteral("triggerUpload"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Uploading"));

	statemachine->submitEvent(QStringLiteral("syncReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronized"));

	statemachine->submitEvent(QStringLiteral("deleteAcc"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("DeletingAcc"));

	statemachine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Inactive"));
}

void StatemachineTest::testError()
{
	QSignalSpy idleSpy{statemachine, &EngineStateMachine::reachedStableState};

	statemachine->stop();
	statemachine->start();
	TEST_STATES(QStringLiteral("Inactive"));

	statemachine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));

	statemachine->submitEvent(QStringLiteral("signedIn"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("DelTables"));

	statemachine->submitEvent(QStringLiteral("delDone"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcWaiting"));

	statemachine->submitEvent(QStringLiteral("error"));
	TEST_STATES(QStringLiteral("Error"));

	statemachine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Inactive"));
}

QTEST_MAIN(StatemachineTest)

#include "tst_statemachine.moc"
