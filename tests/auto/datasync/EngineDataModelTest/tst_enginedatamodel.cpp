#include <QtTest>
#include <QtDataSync>
#include "testlib.h"

#include "enginestatemachine_p.h"
#include "enginedatamodel_p.h"
using namespace QtDataSync;
using namespace std::chrono_literals;

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

private:
	using EngineState = Engine::EngineState;

	FirebaseAuthenticator *_auth = nullptr;
	EngineStateMachine *_machine = nullptr;
	EngineDataModel *_model = nullptr;
};

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

		auto nam = new QNetworkAccessManager{this};
		// authenticator
		_auth = TestLib::createAuth(config.apiKey, this, nam);
		QVERIFY(_auth);

		// create rmc
		auto rmc = new RemoteConnector{
			config,
			nullptr,
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

	// TODO use stateSpy

	_machine->start();
	QVERIFY(_machine->isRunning());
	TRY_VERIFY_STATE(_machine, "Inactive");
	QCOMPARE(_model->state(), EngineState::Inactive);
	QCOMPARE(_model->isSyncActive(), false);

	// wait until logged in
	_model->start();
	QTRY_COMPARE(startSpy.size(), 1);
	QCOMPARE(_model->state(), EngineState::TableSync);
	VERIFY_STATE(_machine, "TableSync");

	// stop the engine
	_model->stop();
	QTRY_COMPARE(stopSpy.size(), 1);
	QCOMPARE(_model->state(), EngineState::Stopping);
	VERIFY_STATE(_machine, "Stopping");

	_model->allTablesStopped();
	QTRY_COMPARE(_model->state(), EngineState::Inactive);
	VERIFY_STATE(_machine, "Inactive");
}

QTEST_MAIN(EngineDataModelTest)

#include "tst_enginedatamodel.moc"
