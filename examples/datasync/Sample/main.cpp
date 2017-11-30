#include "widget.h"
#include <QApplication>
#include <QtDataSync/setup.h>
#include "sampledata.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	qRegisterMetaType<SampleData>();

	//setup datasync
	QtDataSync::Setup().create();

	Widget w;
	a.setProperty("__mw", QVariant::fromValue(&w));
	w.show();

	return a.exec();
}
