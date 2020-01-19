#include <QtTest>
#include <QtDataSync>

#include "enginestatemachine_p.h"
#include "enginedatamodel_p.h"
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

class EngineMachineTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testSyncFlow();
	void testLogOut();
	void testDelAcc();
	void testErrors();

private:
	EngineStateMachine *_machine = nullptr;
	QPointer<EngineDataModel> _model;
};

void EngineMachineTest::initTestCase()
{
	_machine = new EngineStateMachine{this};
	_model = new EngineDataModel{_machine};
	_machine->setDataModel(_model);
	QVERIFY(_machine->init());
}

void EngineMachineTest::cleanupTestCase()
{
	if (_machine->isRunning())
		_machine->stop();
	_machine->deleteLater();
}

void EngineMachineTest::testSyncFlow()
{
	QSignalSpy idleSpy{_machine, &EngineStateMachine::reachedStableState};
	QVERIFY(idleSpy.isValid());

	_machine->stop();
	_machine->start();
	QVERIFY(_machine->isRunning());
	TEST_STATES(QStringLiteral("Inactive"));
	VERIFY_EMPTY(_model);

	// start sync
	_machine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));
	COMPARE_CALLED(_model, signIn, 1);
	VERIFY_EMPTY(_model);

	// stop during sign in
	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Inactive"));
	VERIFY_EMPTY(_model);

	// start again
	_machine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));
	COMPARE_CALLED(_model, signIn, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("signedIn"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("TableSync"));
	COMPARE_CALLED(_model, startTableSync, 1);
	VERIFY_EMPTY(_model);

	// stop
	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Stopping"));
	COMPARE_CALLED(_model, stopTableSync, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("stopped"));
	TEST_STATES(QStringLiteral("Inactive"));
	VERIFY_EMPTY(_model);
}

void EngineMachineTest::testLogOut()
{
	QSignalSpy idleSpy{_machine, &EngineStateMachine::reachedStableState};
	QVERIFY(idleSpy.isValid());

	_machine->stop();
	_machine->start();
	QVERIFY(_machine->isRunning());
	TEST_STATES(QStringLiteral("Inactive"));
	VERIFY_EMPTY(_model);

	// start sync
	_machine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));
	COMPARE_CALLED(_model, signIn, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("signedIn"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("TableSync"));
	COMPARE_CALLED(_model, startTableSync, 1);
	VERIFY_EMPTY(_model);

	// logout
	_model->_stopEv = EngineDataModel::StopEvent::LogOut;
	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Stopping"));
	COMPARE_CALLED(_model, stopTableSync, 1);
	VERIFY_EMPTY(_model);
	QCOMPARE(_model->_stopEv, EngineDataModel::StopEvent::LogOut);

	_machine->submitEvent(QStringLiteral("stopped"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));
	COMPARE_CALLED(_model, signIn, 1);
	VERIFY_EMPTY(_model);
	QCOMPARE(_model->_stopEv, EngineDataModel::StopEvent::LogOut);

	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Inactive"));
	VERIFY_EMPTY(_model);
	QCOMPARE(_model->_stopEv, EngineDataModel::StopEvent::Stop);
}

void EngineMachineTest::testDelAcc()
{
	QSignalSpy idleSpy{_machine, &EngineStateMachine::reachedStableState};
	QVERIFY(idleSpy.isValid());

	_machine->stop();
	_machine->start();
	QVERIFY(_machine->isRunning());
	TEST_STATES(QStringLiteral("Inactive"));
	VERIFY_EMPTY(_model);

	// start sync
	_machine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));
	COMPARE_CALLED(_model, signIn, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("signedIn"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("TableSync"));
	COMPARE_CALLED(_model, startTableSync, 1);
	VERIFY_EMPTY(_model);

	// delete account
	_model->_stopEv = EngineDataModel::StopEvent::DeleteAcc;
	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Stopping"));
	COMPARE_CALLED(_model, stopTableSync, 1);
	VERIFY_EMPTY(_model);
	QCOMPARE(_model->_stopEv, EngineDataModel::StopEvent::DeleteAcc);

	_machine->submitEvent(QStringLiteral("stopped"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("DeletingAcc"));
	COMPARE_CALLED(_model, removeUser, 1);
	VERIFY_EMPTY(_model);
	QCOMPARE(_model->_stopEv, EngineDataModel::StopEvent::DeleteAcc);

	_machine->submitEvent(QStringLiteral("delAccDone"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));
	COMPARE_CALLED(_model, cancelRmUser, 1);
	COMPARE_CALLED(_model, signIn, 1);
	VERIFY_EMPTY(_model);
	QCOMPARE(_model->_stopEv, EngineDataModel::StopEvent::DeleteAcc);

	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Inactive"));
	VERIFY_EMPTY(_model);
	QCOMPARE(_model->_stopEv, EngineDataModel::StopEvent::Stop);
}

void EngineMachineTest::testErrors()
{
	QSignalSpy idleSpy{_machine, &EngineStateMachine::reachedStableState};
	QVERIFY(idleSpy.isValid());

	_machine->stop();
	_machine->start();
	QVERIFY(_machine->isRunning());
	TEST_STATES(QStringLiteral("Inactive"));
	VERIFY_EMPTY(_model);

	// start sync
	_machine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));
	COMPARE_CALLED(_model, signIn, 1);
	VERIFY_EMPTY(_model);

	// error during sign in
	_model->_hasError = true;
	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Error"));
	COMPARE_CALLED(_model, emitError, 1);
	VERIFY_EMPTY(_model);
	_model->_hasError = false;

	// error while in error
	_model->_hasError = true;
	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Error"));
	COMPARE_CALLED(_model, emitError, 1);
	VERIFY_EMPTY(_model);
	_model->_hasError = false;

	// error while running
	_machine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));
	COMPARE_CALLED(_model, signIn, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("signedIn"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("TableSync"));
	COMPARE_CALLED(_model, startTableSync, 1);
	VERIFY_EMPTY(_model);

	_model->_hasError = true;
	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Stopping"));
	COMPARE_CALLED(_model, stopTableSync, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("stopped"));
	TEST_STATES(QStringLiteral("Error"));
	COMPARE_CALLED(_model, emitError, 1);
	VERIFY_EMPTY(_model);
	_model->_hasError = false;

	// error while running with logout
	_machine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));
	COMPARE_CALLED(_model, signIn, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("signedIn"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("TableSync"));
	COMPARE_CALLED(_model, startTableSync, 1);
	VERIFY_EMPTY(_model);

	_model->_hasError = true;
	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Stopping"));
	COMPARE_CALLED(_model, stopTableSync, 1);
	VERIFY_EMPTY(_model);

	_model->_stopEv = EngineDataModel::StopEvent::LogOut;
	_machine->submitEvent(QStringLiteral("stopped"));
	TEST_STATES(QStringLiteral("Error"));
	COMPARE_CALLED(_model, emitError, 1);
	VERIFY_EMPTY(_model);
	_model->_hasError = false;
	QCOMPARE(_model->_stopEv, EngineDataModel::StopEvent::Stop);

	// error while running with delete acc
	_machine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));
	COMPARE_CALLED(_model, signIn, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("signedIn"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("TableSync"));
	COMPARE_CALLED(_model, startTableSync, 1);
	VERIFY_EMPTY(_model);

	_model->_hasError = true;
	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Stopping"));
	COMPARE_CALLED(_model, stopTableSync, 1);
	VERIFY_EMPTY(_model);

	_model->_stopEv = EngineDataModel::StopEvent::DeleteAcc;
	_machine->submitEvent(QStringLiteral("stopped"));
	TEST_STATES(QStringLiteral("Error"));
	COMPARE_CALLED(_model, emitError, 1);
	VERIFY_EMPTY(_model);
	_model->_hasError = false;
	QCOMPARE(_model->_stopEv, EngineDataModel::StopEvent::Stop);

	// error while delete acc
	_machine->submitEvent(QStringLiteral("start"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SigningIn"));
	COMPARE_CALLED(_model, signIn, 1);
	VERIFY_EMPTY(_model);

	_machine->submitEvent(QStringLiteral("signedIn"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("TableSync"));
	COMPARE_CALLED(_model, startTableSync, 1);
	VERIFY_EMPTY(_model);

	_model->_stopEv = EngineDataModel::StopEvent::DeleteAcc;
	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("SignedIn"),
				QStringLiteral("Stopping"));
	COMPARE_CALLED(_model, stopTableSync, 1);
	VERIFY_EMPTY(_model);
	QCOMPARE(_model->_stopEv, EngineDataModel::StopEvent::DeleteAcc);

	_machine->submitEvent(QStringLiteral("stopped"));
	TEST_STATES(QStringLiteral("Active"),
				QStringLiteral("DeletingAcc"));
	COMPARE_CALLED(_model, removeUser, 1);
	VERIFY_EMPTY(_model);
	QCOMPARE(_model->_stopEv, EngineDataModel::StopEvent::DeleteAcc);

	_model->_hasError = true;
	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Error"));
	COMPARE_CALLED(_model, cancelRmUser, 1);
	COMPARE_CALLED(_model, emitError, 1);
	VERIFY_EMPTY(_model);
	_model->_hasError = false;

	// stop completely
	_machine->submitEvent(QStringLiteral("stop"));
	TEST_STATES(QStringLiteral("Inactive"));
	VERIFY_EMPTY(_model);
}

QTEST_MAIN(EngineMachineTest)

#include "tst_enginemachine.moc"
