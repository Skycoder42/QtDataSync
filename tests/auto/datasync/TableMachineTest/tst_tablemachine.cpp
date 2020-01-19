#include <QtTest>
#include <QtDataSync>

#include "tablestatemachine_p.h"
#include "tabledatamodel_p.h"
using namespace QtDataSync;

#define TEST_STATES(...) do { \
	QVERIFY(idleSpy.wait()); \
	QStringList states {__VA_ARGS__}; \
	states.sort(); \
	auto sNames = _machine->activeStateNames(false); \
	sNames.sort(); \
	if (!QTest::qCompare(sNames, states, "_machine->activeStateNames(false)", #__VA_ARGS__, __FILE__, __LINE__))\
		return;\
} while (false)

class TableMachineTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testPassivSyncFlow();
	void testLiveSyncFlow();
	void testSyncModeTransitions();
	void testDelTableFlows();
	void testErrors();

private:
	TableStateMachine *_machine = nullptr;
	QPointer<TableDataModel> _model;
};

void TableMachineTest::initTestCase()
{
	_machine = new TableStateMachine{this};
	_model = new TableDataModel{_machine};
	_machine->setDataModel(_model);
	QVERIFY(_machine->init());
}

void TableMachineTest::cleanupTestCase()
{
	if (_machine->isRunning())
		_machine->stop();
	_machine->deleteLater();
}

void TableMachineTest::testPassivSyncFlow()
{
	QSignalSpy idleSpy{_machine, &TableStateMachine::reachedStableState};
	QVERIFY(idleSpy.isValid());

	_machine->stop();
	_machine->start();
	QVERIFY(_machine->isRunning());
	TEST_STATES(QStringLiteral("Stopped"),
				QStringLiteral("Inactive"));
	COMPARE_CALLED(_model, emitStop, 1);
	VERIFY_EMPTY(_model);

	// start sync
	_machine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("Init"));
	COMPARE_CALLED(_model, initSync, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("startPassiveSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"));
	COMPARE_CALLED(_model, downloadChanges, 1);
	COMPARE_CALLED(_model, processDownload, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlDone"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"));
	VERIFY_EMPTY(_model);

	// trigger sync during download
	_machine->submitEvent(QStringLiteral("triggerSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"));
	COMPARE_CALLED(_model, downloadChanges, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("procReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcWaiting"));
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("dataReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"));
	COMPARE_CALLED(_model, processDownload, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("procContinue"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"));
	COMPARE_CALLED(_model, processDownload, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("procReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcWaiting"));
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Uploading"));
	COMPARE_CALLED(_model, cancelPassiveSync, 1);
	COMPARE_CALLED(_model, uploadData, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("ulContinue"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Uploading"));
	COMPARE_CALLED(_model, uploadData, 1);
	VERIFY_EMPTY(_model);

	// trigger sync during upload
	_machine->submitEvent(QStringLiteral("triggerSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"));
	COMPARE_CALLED(_model, downloadChanges, 1);
	COMPARE_CALLED(_model, processDownload, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlDone"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"));
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("procReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Uploading"));
	COMPARE_CALLED(_model, cancelPassiveSync, 1);
	COMPARE_CALLED(_model, uploadData, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("syncReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronized"));
	VERIFY_EMPTY(_model);

	// trigger upload
	_machine->submitEvent(QStringLiteral("triggerUpload"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Uploading"));
	COMPARE_CALLED(_model, uploadData, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("syncReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronized"));
	VERIFY_EMPTY(_model);

	// trigger sync
	_machine->submitEvent(QStringLiteral("triggerSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"));
	COMPARE_CALLED(_model, downloadChanges, 1);
	COMPARE_CALLED(_model, processDownload, 1);
	VERIFY_EMPTY(_model);

	// theoretical transition, should not happen in real live, but is possible
	_machine->submitEvent(QStringLiteral("syncReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronized"));
	COMPARE_CALLED(_model, cancelPassiveSync, 1);
	VERIFY_EMPTY(_model);

	// stop
	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Stopped"),
				QStringLiteral("Inactive"));
	COMPARE_CALLED(_model, cancelAll, 1);
	COMPARE_CALLED(_model, emitStop, 1);
	VERIFY_EMPTY(_model);
}

void TableMachineTest::testLiveSyncFlow()
{
	QSignalSpy idleSpy{_machine, &TableStateMachine::reachedStableState};
	QVERIFY(idleSpy.isValid());

	_machine->stop();
	_machine->start();
	QVERIFY(_machine->isRunning());
	TEST_STATES(QStringLiteral("Stopped"),
				QStringLiteral("Inactive"));
	COMPARE_CALLED(_model, emitStop, 1);
	VERIFY_EMPTY(_model);

	// start sync
	_machine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("Init"));
	COMPARE_CALLED(_model, initSync, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("startLiveSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsStarting"));
	COMPARE_CALLED(_model, initLiveSync, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsProcessInit"));
	COMPARE_CALLED(_model, processDownload, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("procContinue"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsProcessInit"));
	COMPARE_CALLED(_model, processDownload, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("procReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsActive"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsRunning"),
				QStringLiteral("UlFiber"),
				QStringLiteral("UlRunning"));
	COMPARE_CALLED(_model, processDownload, 1);
	COMPARE_CALLED(_model, uploadData, 1);
	VERIFY_EMPTY(_model);

	// test various dl/ul flows
	_machine->submitEvent(QStringLiteral("procReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsActive"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsWaiting"),
				QStringLiteral("UlFiber"),
				QStringLiteral("UlRunning"));
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("ulContinue"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsActive"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsWaiting"),
				QStringLiteral("UlFiber"),
				QStringLiteral("UlRunning"));
	COMPARE_CALLED(_model, uploadData, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("syncReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsActive"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsWaiting"),
				QStringLiteral("UlFiber"),
				QStringLiteral("UlWaiting"));
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("dataReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsActive"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsRunning"),
				QStringLiteral("UlFiber"),
				QStringLiteral("UlWaiting"));
	COMPARE_CALLED(_model, processDownload, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("procContinue"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsActive"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsRunning"),
				QStringLiteral("UlFiber"),
				QStringLiteral("UlWaiting"));
	COMPARE_CALLED(_model, processDownload, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("triggerUpload"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsActive"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsRunning"),
				QStringLiteral("UlFiber"),
				QStringLiteral("UlRunning"));
	COMPARE_CALLED(_model, uploadData, 1);
	VERIFY_EMPTY(_model);

	// force sync
	_machine->submitEvent(QStringLiteral("forceSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsStarting"));
	COMPARE_CALLED(_model, cancelLiveSync, 1);
	COMPARE_CALLED(_model, initLiveSync, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsProcessInit"));
	COMPARE_CALLED(_model, processDownload, 1);
	VERIFY_EMPTY(_model);

	// ls error
	_machine->submitEvent(QStringLiteral("lsError"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsError"));
	COMPARE_CALLED(_model, cancelLiveSync, 1);
	COMPARE_CALLED(_model, scheduleLsRestart, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("continueLiveSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsStarting"));
	COMPARE_CALLED(_model, clearLsRestart, 1);
	COMPARE_CALLED(_model, initLiveSync, 1);
	VERIFY_EMPTY(_model);

	// stop
	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Stopped"),
				QStringLiteral("Inactive"));
	COMPARE_CALLED(_model, cancelLiveSync, 1);
	COMPARE_CALLED(_model, cancelAll, 1);
	COMPARE_CALLED(_model, emitStop, 1);
	VERIFY_EMPTY(_model);
}

void TableMachineTest::testSyncModeTransitions()
{
	QSignalSpy idleSpy{_machine, &TableStateMachine::reachedStableState};
	QVERIFY(idleSpy.isValid());

	_machine->stop();
	_machine->start();
	QVERIFY(_machine->isRunning());
	TEST_STATES(QStringLiteral("Stopped"),
				QStringLiteral("Inactive"));
	COMPARE_CALLED(_model, emitStop, 1);
	VERIFY_EMPTY(_model);

	// start in passive sync mode
	_machine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("Init"));
	COMPARE_CALLED(_model, initSync, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("startPassiveSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"));
	COMPARE_CALLED(_model, downloadChanges, 1);
	COMPARE_CALLED(_model, processDownload, 1);
	VERIFY_EMPTY(_model);

	// switch to live sync
	_machine->submitEvent(QStringLiteral("startLiveSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsStarting"));
	COMPARE_CALLED(_model, cancelPassiveSync, 1);
	COMPARE_CALLED(_model, initLiveSync, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsProcessInit"));
	COMPARE_CALLED(_model, processDownload, 1);
	VERIFY_EMPTY(_model);

	// switch back to passive sync
	_machine->submitEvent(QStringLiteral("startPassiveSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"));
	COMPARE_CALLED(_model, cancelLiveSync, 1);
	COMPARE_CALLED(_model, downloadChanges, 1);
	COMPARE_CALLED(_model, processDownload, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlDone"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"));
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("procReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Uploading"));
	COMPARE_CALLED(_model, cancelPassiveSync, 1);
	COMPARE_CALLED(_model, uploadData, 1);
	VERIFY_EMPTY(_model);

	// switch to live sync
	_machine->submitEvent(QStringLiteral("startLiveSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsStarting"));
	COMPARE_CALLED(_model, initLiveSync, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsProcessInit"));
	COMPARE_CALLED(_model, processDownload, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("procReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsActive"),
				QStringLiteral("LsFiber"),
				QStringLiteral("LsRunning"),
				QStringLiteral("UlFiber"),
				QStringLiteral("UlRunning"));
	COMPARE_CALLED(_model, processDownload, 1);
	COMPARE_CALLED(_model, uploadData, 1);
	VERIFY_EMPTY(_model);

	// passive sync again, while "uploading"
	_machine->submitEvent(QStringLiteral("startPassiveSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Uploading"));
	COMPARE_CALLED(_model, cancelLiveSync, 1);
	COMPARE_CALLED(_model, uploadData, 1);
	VERIFY_EMPTY(_model);

	// stop
	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Stopped"),
				QStringLiteral("Inactive"));
	COMPARE_CALLED(_model, cancelAll, 1);
	COMPARE_CALLED(_model, emitStop, 1);
	VERIFY_EMPTY(_model);
}

void TableMachineTest::testDelTableFlows()
{
	QSignalSpy idleSpy{_machine, &TableStateMachine::reachedStableState};
	QVERIFY(idleSpy.isValid());

	_machine->stop();
	_machine->start();
	QVERIFY(_machine->isRunning());
	TEST_STATES(QStringLiteral("Stopped"),
				QStringLiteral("Inactive"));
	COMPARE_CALLED(_model, emitStop, 1);
	VERIFY_EMPTY(_model);

	// delete the table
	_model->_delTable = true;
	_machine->submitEvent(QStringLiteral("delTable"));
	TEST_STATES(QStringLiteral("Stopped"),
				QStringLiteral("Inactive"));
	VERIFY_EMPTY(_model);
	QCOMPARE(_model->_delTable, true);

	_machine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("DelTable"));
	COMPARE_CALLED(_model, delTable, 1);
	VERIFY_EMPTY(_model);
	QCOMPARE(_model->_delTable, true);

	_machine->submitEvent(QStringLiteral("delTableDone"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("Init"));
	COMPARE_CALLED(_model, cancelPassiveSync, 1);
	COMPARE_CALLED(_model, initSync, 1);
	VERIFY_EMPTY(_model);
	QCOMPARE(_model->_delTable, false);

	// delete while "running"
	_model->_delTable = true;
	_machine->submitEvent(QStringLiteral("delTable"));
	TEST_STATES(QStringLiteral("DelTable"));
	COMPARE_CALLED(_model, cancelAll, 1);
	COMPARE_CALLED(_model, delTable, 1);
	VERIFY_EMPTY(_model);
	QCOMPARE(_model->_delTable, true);

	// stop
	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Stopped"),
				QStringLiteral("Inactive"));
	COMPARE_CALLED(_model, cancelPassiveSync, 1);
	COMPARE_CALLED(_model, emitStop, 1);
	VERIFY_EMPTY(_model);
	QCOMPARE(_model->_delTable, false);
}

void TableMachineTest::testErrors()
{
	QSignalSpy idleSpy{_machine, &TableStateMachine::reachedStableState};
	QVERIFY(idleSpy.isValid());

	_machine->stop();
	_machine->start();
	QVERIFY(_machine->isRunning());
	TEST_STATES(QStringLiteral("Stopped"),
				QStringLiteral("Inactive"));
	COMPARE_CALLED(_model, emitStop, 1);
	VERIFY_EMPTY(_model);

	// start passive sync
	_machine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("Init"));
	COMPARE_CALLED(_model, initSync, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("startPassiveSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("PassiveSync"),
				QStringLiteral("Synchronizing"),
				QStringLiteral("Downloading"),
				QStringLiteral("DlFiber"),
				QStringLiteral("DlRunning"),
				QStringLiteral("ProcFiber"),
				QStringLiteral("ProcRunning"));
	COMPARE_CALLED(_model, downloadChanges, 1);
	COMPARE_CALLED(_model, processDownload, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("error"));
	TEST_STATES(QStringLiteral("Stopped"),
				QStringLiteral("Error"));
	COMPARE_CALLED(_model, cancelPassiveSync, 1);
	COMPARE_CALLED(_model, cancelAll, 1);
	COMPARE_CALLED(_model, emitStop, 1);
	COMPARE_CALLED(_model, emitError, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("error"));
	TEST_STATES(QStringLiteral("Stopped"),
				QStringLiteral("Error"));
	COMPARE_CALLED(_model, emitError, 1);
	VERIFY_EMPTY(_model);

	// start live sync (from within error)
	_machine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("Init"));
	COMPARE_CALLED(_model, initSync, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("startLiveSync"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsStarting"));
	COMPARE_CALLED(_model, initLiveSync, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("dlReady"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("LiveSync"),
				QStringLiteral("LsProcessInit"));
	COMPARE_CALLED(_model, processDownload, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("error"));
	TEST_STATES(QStringLiteral("Stopped"),
				QStringLiteral("Error"));
	COMPARE_CALLED(_model, cancelLiveSync, 1);
	COMPARE_CALLED(_model, cancelAll, 1);
	COMPARE_CALLED(_model, emitStop, 1);
	COMPARE_CALLED(_model, emitError, 1);
	VERIFY_EMPTY(_model);

	// error in delete table
	_model->_delTable = true;
	_machine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("DelTable"));
	COMPARE_CALLED(_model, delTable, 1);
	VERIFY_EMPTY(_model);
	QCOMPARE(_model->_delTable, true);

	_machine->submitEvent(QStringLiteral("error"));
	TEST_STATES(QStringLiteral("Stopped"),
				QStringLiteral("Error"));
	COMPARE_CALLED(_model, cancelPassiveSync, 1);
	COMPARE_CALLED(_model, emitStop, 1);
	COMPARE_CALLED(_model, emitError, 1);
	VERIFY_EMPTY(_model);
	QCOMPARE(_model->_delTable, false);
}

QTEST_MAIN(TableMachineTest)

#include "tst_tablemachine.moc"
