#include "widget.h"
#include <QApplication>

#include <setup.h>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	//setup datasync
	QtDataSync::Setup().create();

	Widget w;
	w.show();

	return a.exec();
}
