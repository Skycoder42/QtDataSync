#include "mainwidget.h"
#include <QApplication>
#include <QtJsonSerializer/QJsonSerializer>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QJsonSerializer::registerListConverters<SampleData*>();

	MainWidget w;
	w.show();

	return a.exec();
}
