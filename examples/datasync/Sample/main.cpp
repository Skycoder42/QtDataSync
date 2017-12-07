#include "widget.h"
#include <QApplication>
#include <QLoggingCategory>
#include <QtDataSync/setup.h>
#include <QtDataSync/keystore.h>
#include "sampledata.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	qRegisterMetaType<SampleData>();

	QLoggingCategory::setFilterRules(QStringLiteral("qtdatasync.*.debug=true"));

	qDebug() << "Available keystores:" << QtDataSync::KeyStore::listProviders();

	//setup datasync
	QtDataSync::Setup()
			.setRemoteConfiguration(QUrl(QStringLiteral("ws://localhost:4242")))
			.create();

	Widget w;
	w.show();

	return a.exec();
}
