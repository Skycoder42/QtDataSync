#include <QtQuickTest/quicktest.h>
#include <QtDataSync/setup.h>

static void initImportPath()
{
	qputenv("QML2_IMPORT_PATH", "/home/sky/Programming/QtLibraries/build-qtdatasync-Desktop_Qt_5_10_0_GCC_64bit-Debug/qml/");//TODO make dynamic

	QtDataSync::Setup().create();
}
Q_COREAPP_STARTUP_FUNCTION(initImportPath)

QUICK_TEST_MAIN(qmldatasync)
