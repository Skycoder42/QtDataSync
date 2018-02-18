#include <QtCore>
#include <QtQuickTest/quicktest.h>
#include <QtDataSync/setup.h>

using namespace QtDataSync;

static QTemporaryDir tDir;

static void initImportPath()
{
#ifdef Q_OS_LINUX
	if(!qgetenv("LD_PRELOAD").contains("Qt5DataSync"))
		qWarning() << "No LD_PRELOAD set - this may fail on systems with multiple version of the modules";
#endif
	qputenv("PLUGIN_KEYSTORES_PATH", KEYSTORE_PATH);
	qputenv("QML2_IMPORT_PATH", QML_PATH);

#ifdef VERBOSE_TESTS
	QLoggingCategory::setFilterRules(QStringLiteral("qtdatasync.*.debug=true"));
#endif

	tDir.setAutoRemove(false);
	qInfo() << "storage path:" << tDir.path();
	Setup().setLocalDir(tDir.path())
			.setKeyStoreProvider(QStringLiteral("plain"))
			.setSignatureScheme(Setup::RSA_PSS_SHA3_512)
			.setSignatureKeyParam(2048)
			.setEncryptionScheme(Setup::RSA_OAEP_SHA3_512)
			.setEncryptionKeyParam(2048)
			.create();
}
Q_COREAPP_STARTUP_FUNCTION(initImportPath)

QUICK_TEST_MAIN(qmldatasync)
