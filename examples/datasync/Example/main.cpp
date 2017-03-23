#include "mainwidget.h"
#include <QApplication>
#include <QLoggingCategory>
#include <cachingdatastore.h>
#include <QtJsonSerializer/QJsonSerializer>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QLoggingCategory::setFilterRules("qtdatasync.DefaultSetup.debug=true");
	QJsonSerializer::registerListConverters<SampleData*>();

	MainWidget w;
	a.setProperty("__mw", QVariant::fromValue(&w));
	w.show();

	return a.exec();
}
