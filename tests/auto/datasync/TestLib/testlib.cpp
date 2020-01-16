#include "testlib.h"
#include <QtTest>
#include <QtDataSync/authenticator.h>
#define private public
#include <QtDataSync/setup_impl.h>
#undef private
#include <QtDataSync/private/firebaseauthenticator_p.h>
using namespace QtDataSync;
using namespace QtDataSync::__private;

namespace {

class DummyExtender : public __private::SetupExtensionPrivate
{
public:
	QObject *createInstance(QObject *) override {
		return nullptr;
	}
};

}

std::optional<SetupPrivate> TestLib::loadSetup(const QString &path)
{
	std::optional<SetupPrivate> setup;
	[&](){
		try {
			QFile configFile{path};
			QVERIFY(configFile.exists());
			QVERIFY2(configFile.open(QIODevice::ReadOnly | QIODevice::Text), qUtf8Printable(configFile.errorString()));
			setup = __private::SetupPrivate{};
			setup->_authExt = new DummyExtender{};
			setup->_transExt = new DummyExtender{};
			setup->readWebConfig(&configFile);
		} catch (std::exception &e) {
			setup.reset();
			QFAIL(e.what());
		}
	}();
	return setup;
}

void TestLib::rmAcc(FirebaseAuthenticator *auth)
{
	QSignalSpy accDelSpy{auth, &FirebaseAuthenticator::accountDeleted};
	auth->deleteUser();
	if (!accDelSpy.isEmpty() || accDelSpy.wait())
		qDebug() << "deleteAcc" << accDelSpy[0][0].toBool();
	else
		qDebug() << "deleteAcc did not fire";
}
