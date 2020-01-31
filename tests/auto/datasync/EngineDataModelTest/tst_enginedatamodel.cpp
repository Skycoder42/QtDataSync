#include <QtTest>
#include <QtDataSync>
#include "testlib.h"

#include "enginestatemachine_p.h"
#include "enginedatamodel_p.h"
#include "fakeauth.h"
using namespace QtDataSync;
using namespace std::chrono_literals;

Q_DECLARE_METATYPE(EnginePrivate::ErrorInfo)

#define VERIFY_STATE(machine, state) QCOMPARE(machine->activeStateNames(true), QStringList{QStringLiteral(state)})
#define TRY_VERIFY_STATE(machine, state) QTRY_COMPARE(machine->activeStateNames(true), QStringList{QStringLiteral(state)})
#define VERIFY_SPY(sigSpy, errorSpy) QVERIFY2(sigSpy.wait(), qUtf8Printable(QStringLiteral("Error: ") + errorSpy.value(0).value(0).toString()))

class EngineDataModelTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testSyncFlow();
	void testLogoutFlow();
	void testDeleteAcc();
	void testExternalError();

	void testAuthCancel();

private:
	using EngineState = Engine::EngineState;

	FirebaseAuthenticator *_auth = nullptr;
	EngineStateMachine *_machine = nullptr;
	EngineDataModel *_model = nullptr;

	static inline EngineState getState(QSignalSpy &spy) {
		return spy.takeFirst()[0].value<EngineState>();
	}
};

bool operator==(const EnginePrivate::ErrorInfo &lhs, const EnginePrivate::ErrorInfo &rhs) {
	return lhs.type == rhs.type &&
		   lhs.data == rhs.data;
}

void EngineDataModelTest::initTestCase()
{
	qRegisterMetaType<EngineState>("Engine::EngineState");
	qRegisterMetaType<EnginePrivate::ErrorInfo>("EnginePrivate::ErrorInfo");

	try {
		__private::SetupPrivate::FirebaseConfig config {
			QStringLiteral(FIREBASE_PROJECT_ID),
			QStringLiteral(FIREBASE_API_KEY),
			1min,
			100,
			false
		};

		auto settings = TestLib::createSettings(this);
		auto nam = new QNetworkAccessManager{this};
		// authenticator
		_auth = TestLib::createAuth(config.apiKey, this, nam, settings);
		QVERIFY(_auth);

		// create rmc
		auto rmc = new RemoteConnector{
			config,
			settings,
			nam,
			std::nullopt,
			this
		};

		// create machine/model
		_machine = new EngineStateMachine{this};
		_model = new EngineDataModel{_machine};
		_machine->setDataModel(_model);
		QVERIFY(_machine->init());
		_model->setupModel(_auth, rmc);
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

void EngineDataModelTest::cleanupTestCase()
{
	if (_machine)
		delete _machine;
	if (_auth) {
		TestLib::rmAcc(_auth);
		delete _auth;
	}
}

void EngineDataModelTest::testSyncFlow()
{
	QSignalSpy startSpy{_model, &EngineDataModel::startTableSync};
	QVERIFY(startSpy.isValid());
	QSignalSpy stopSpy{_model, &EngineDataModel::stopTableSync};
	QVERIFY(stopSpy.isValid());
	QSignalSpy stateSpy{_model, &EngineDataModel::stateChanged};
	QVERIFY(stateSpy.isValid());
	QSignalSpy errorSpy{_model, &EngineDataModel::errorOccured};
	QVERIFY(errorSpy.isValid());

	QCOMPARE(_model->state(), EngineState::Inactive);
	QCOMPARE(_model->isSyncActive(), false);

	_machine->start();
	QVERIFY(_machine->isRunning());
	TRY_VERIFY_STATE(_machine, "Inactive");
	QCOMPARE(_model->state(), EngineState::Inactive);
	QCOMPARE(_model->isSyncActive(), false);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::Inactive);

	_model->start();
	QTRY_COMPARE(startSpy.size(), 1);
	QCOMPARE(_model->state(), EngineState::TableSync);
	QCOMPARE(_model->isSyncActive(), true);
	QCOMPARE(stateSpy.size(), 2);
	QCOMPARE(getState(stateSpy), EngineState::SigningIn);
	QCOMPARE(getState(stateSpy), EngineState::TableSync);
	VERIFY_STATE(_machine, "TableSync");

	// stop the engine
	_model->stop();
	QTRY_COMPARE(stopSpy.size(), 1);
	QCOMPARE(_model->state(), EngineState::Stopping);
	QCOMPARE(_model->isSyncActive(), false);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::Stopping);
	VERIFY_STATE(_machine, "Stopping");

	_model->allTablesStopped();
	QTRY_COMPARE(_model->state(), EngineState::Inactive);
	QCOMPARE(_model->isSyncActive(), false);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::Inactive);
	VERIFY_STATE(_machine, "Inactive");

	QVERIFY(errorSpy.isEmpty());
}

void EngineDataModelTest::testLogoutFlow()
{
	QSignalSpy startSpy{_model, &EngineDataModel::startTableSync};
	QVERIFY(startSpy.isValid());
	QSignalSpy stopSpy{_model, &EngineDataModel::stopTableSync};
	QVERIFY(stopSpy.isValid());
	QSignalSpy stateSpy{_model, &EngineDataModel::stateChanged};
	QVERIFY(stateSpy.isValid());
	QSignalSpy errorSpy{_model, &EngineDataModel::errorOccured};
	QVERIFY(errorSpy.isValid());

	QCOMPARE(_model->state(), EngineState::Inactive);
	QCOMPARE(_model->isSyncActive(), false);

	// start
	_model->start();
	QTRY_COMPARE(startSpy.size(), 1);
	QCOMPARE(_model->state(), EngineState::TableSync);
	QCOMPARE(_model->isSyncActive(), true);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::TableSync);
	VERIFY_STATE(_machine, "TableSync");

	// logout
	_model->logOut();
	QTRY_COMPARE(stopSpy.size(), 1);
	QCOMPARE(_model->state(), EngineState::Stopping);
	QCOMPARE(_model->isSyncActive(), false);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::Stopping);
	VERIFY_STATE(_machine, "Stopping");

	// triggers a new sign in
	_model->allTablesStopped();
	QTRY_COMPARE(_model->state(), EngineState::TableSync);
	QCOMPARE(_model->isSyncActive(), true);
	QCOMPARE(stateSpy.size(), 2);
	QCOMPARE(getState(stateSpy), EngineState::SigningIn);
	QCOMPARE(getState(stateSpy), EngineState::TableSync);
	VERIFY_STATE(_machine, "TableSync");

	// stop
	_model->stop();
	QTRY_COMPARE(stopSpy.size(), 2);
	QCOMPARE(_model->state(), EngineState::Stopping);
	QCOMPARE(_model->isSyncActive(), false);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::Stopping);
	VERIFY_STATE(_machine, "Stopping");

	_model->allTablesStopped();
	QTRY_COMPARE(_model->state(), EngineState::Inactive);
	QCOMPARE(_model->isSyncActive(), false);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::Inactive);
	VERIFY_STATE(_machine, "Inactive");

	QVERIFY(errorSpy.isEmpty());
}

void EngineDataModelTest::testDeleteAcc()
{
	QSignalSpy startSpy{_model, &EngineDataModel::startTableSync};
	QVERIFY(startSpy.isValid());
	QSignalSpy stopSpy{_model, &EngineDataModel::stopTableSync};
	QVERIFY(stopSpy.isValid());
	QSignalSpy stateSpy{_model, &EngineDataModel::stateChanged};
	QVERIFY(stateSpy.isValid());
	QSignalSpy errorSpy{_model, &EngineDataModel::errorOccured};
	QVERIFY(errorSpy.isValid());

	QCOMPARE(_model->state(), EngineState::Inactive);
	QCOMPARE(_model->isSyncActive(), false);

	// start
	_model->start();
	QTRY_COMPARE(startSpy.size(), 1);
	QCOMPARE(_model->state(), EngineState::TableSync);
	QCOMPARE(_model->isSyncActive(), true);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::TableSync);
	VERIFY_STATE(_machine, "TableSync");

	// delete acc
	_model->deleteAccount();
	QTRY_COMPARE(stopSpy.size(), 1);
	QCOMPARE(_model->state(), EngineState::Stopping);
	QCOMPARE(_model->isSyncActive(), false);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::Stopping);
	VERIFY_STATE(_machine, "Stopping");

	_model->allTablesStopped();
	QTRY_COMPARE(_model->state(), EngineState::TableSync);
	QCOMPARE(_model->isSyncActive(), true);
	QCOMPARE(stateSpy.size(), 3);
	QCOMPARE(getState(stateSpy), EngineState::DeletingAcc);
	QCOMPARE(getState(stateSpy), EngineState::SigningIn);
	QCOMPARE(getState(stateSpy), EngineState::TableSync);
	VERIFY_STATE(_machine, "TableSync");

	// stop
	_model->stop();
	QTRY_COMPARE(stopSpy.size(), 2);
	QCOMPARE(_model->state(), EngineState::Stopping);
	QCOMPARE(_model->isSyncActive(), false);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::Stopping);
	VERIFY_STATE(_machine, "Stopping");

	_model->allTablesStopped();
	QTRY_COMPARE(_model->state(), EngineState::Inactive);
	QCOMPARE(_model->isSyncActive(), false);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::Inactive);
	VERIFY_STATE(_machine, "Inactive");

	QVERIFY(errorSpy.isEmpty());
}

void EngineDataModelTest::testExternalError()
{
	QSignalSpy startSpy{_model, &EngineDataModel::startTableSync};
	QVERIFY(startSpy.isValid());
	QSignalSpy stopSpy{_model, &EngineDataModel::stopTableSync};
	QVERIFY(stopSpy.isValid());
	QSignalSpy stateSpy{_model, &EngineDataModel::stateChanged};
	QVERIFY(stateSpy.isValid());
	QSignalSpy errorSpy{_model, &EngineDataModel::errorOccured};
	QVERIFY(errorSpy.isValid());

	QCOMPARE(_model->state(), EngineState::Inactive);
	QCOMPARE(_model->isSyncActive(), false);

	// start
	_model->start();
	QTRY_COMPARE(startSpy.size(), 1);
	QCOMPARE(_model->state(), EngineState::TableSync);
	QCOMPARE(_model->isSyncActive(), true);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::TableSync);
	VERIFY_STATE(_machine, "TableSync");

	// send error
	EnginePrivate::ErrorInfo testError;
	testError.type = Engine::ErrorType::Database;
	testError.data = 42;
	_model->cancel(testError);
	QTRY_COMPARE(stopSpy.size(), 1);
	QCOMPARE(_model->state(), EngineState::Stopping);
	QCOMPARE(_model->isSyncActive(), false);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::Stopping);
	VERIFY_STATE(_machine, "Stopping");

	_model->allTablesStopped();
	QTRY_COMPARE(_model->state(), EngineState::Error);
	QCOMPARE(_model->isSyncActive(), false);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::Error);
	QCOMPARE(errorSpy.size(), 1);
	QCOMPARE(errorSpy.takeFirst()[0].value<EnginePrivate::ErrorInfo>(), testError);
	VERIFY_STATE(_machine, "Error");

	// error in error
	testError.data = 66;
	_model->cancel(testError);
	QTRY_COMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::Error);
	QCOMPARE(_model->state(), EngineState::Error);
	QCOMPARE(_model->isSyncActive(), false);
	QCOMPARE(errorSpy.size(), 1);
	QCOMPARE(errorSpy.takeFirst()[0].value<EnginePrivate::ErrorInfo>(), testError);
	VERIFY_STATE(_machine, "Error");

	// start again
	_model->start();
	QTRY_COMPARE(startSpy.size(), 2);
	QCOMPARE(_model->state(), EngineState::TableSync);
	QCOMPARE(_model->isSyncActive(), true);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::TableSync);
	VERIFY_STATE(_machine, "TableSync");

	// send another error
	testError.data = 13;
	_model->cancel(testError);
	QTRY_COMPARE(stopSpy.size(), 2);
	QCOMPARE(_model->state(), EngineState::Stopping);
	QCOMPARE(_model->isSyncActive(), false);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::Stopping);
	VERIFY_STATE(_machine, "Stopping");

	_model->allTablesStopped();
	QTRY_COMPARE(_model->state(), EngineState::Error);
	QCOMPARE(_model->isSyncActive(), false);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::Error);
	QCOMPARE(errorSpy.size(), 1);
	QCOMPARE(errorSpy.takeFirst()[0].value<EnginePrivate::ErrorInfo>(), testError);
	VERIFY_STATE(_machine, "Error");

	// stop
	_model->stop();
	QTRY_COMPARE(_model->state(), EngineState::Inactive);
	QCOMPARE(_model->isSyncActive(), false);
	QCOMPARE(stateSpy.size(), 1);
	QCOMPARE(getState(stateSpy), EngineState::Inactive);
	VERIFY_STATE(_machine, "Inactive");

	QVERIFY(errorSpy.isEmpty());
}

void EngineDataModelTest::testAuthCancel()
{
	try {
		__private::SetupPrivate::FirebaseConfig config;

		auto settings = TestLib::createSettings(this);
		auto nam = new QNetworkAccessManager{this};
		// authenticator
		auto auth = new FirebaseAuthenticator {
			new FakeAuth{this},
			{},
			settings,
			nam,
			std::nullopt,
			this
		};

		// create rmc
		auto rmc = new RemoteConnector{
			config,
			settings,
			nam,
			std::nullopt,
			this
		};

		// create machine/model
		auto machine = new EngineStateMachine{this};
		auto model = new EngineDataModel{machine};
		machine->setDataModel(model);
		QVERIFY(machine->init());
		model->setupModel(auth, rmc);

		// start machine and wait for sign up
		machine->start();
		model->start();
		QTRY_COMPARE(model->state(), EngineState::SigningIn);
		QCOMPARE(model->isSyncActive(), false);
		VERIFY_STATE(machine, "SigningIn");

		// send stop
		model->stop();
		QTRY_COMPARE(model->state(), EngineState::Inactive);
		QCOMPARE(model->isSyncActive(), false);
		VERIFY_STATE(machine, "Inactive");
	} catch (std::exception &e) {
		QFAIL(e.what());
	}
}

QTEST_MAIN(EngineDataModelTest)

#include "tst_enginedatamodel.moc"
