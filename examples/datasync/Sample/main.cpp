#include "widget.h"
#include <QApplication>
#include <QLoggingCategory>
#include <QtDataSync/setup.h>
#include "sampledata.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	qRegisterMetaType<SampleData>();

	QLoggingCategory::setFilterRules(QStringLiteral("qtdatasync.*.debug=true"));

	//setup datasync
	QtDataSync::Setup().create();

	Widget w;
	w.show();

	return a.exec();
}
