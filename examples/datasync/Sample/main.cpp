#include "widget.h"
#include <QApplication>
#include <QLoggingCategory>
#include <QtDataSync/syncmanager.h>
#include <QtDataSync/setup.h>
#include <QtDataSync/keystore.h>
#include "sampledata.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	qRegisterMetaType<SampleData>();

	QLoggingCategory::setFilterRules(QStringLiteral("qtdatasync.*.debug=true"));
	qSetMessagePattern(QStringLiteral("%{if-debug}[\033[32mDebug\033[0m]    %{endif}"
									  "%{if-info}[\033[36mInfo\033[0m]     %{endif}"
									  "%{if-warning}[\033[33mWarning\033[0m]  %{endif}"
									  "%{if-critical}[\033[31mCritical\033[0m] %{endif}"
									  "%{if-fatal}[\033[35mFatal\033[0m]    %{endif}"
									  "%{if-category}\033[34m%{category}:\033[0m %{endif}"
									  "%{message}"));

	qDebug() << "Available keystores:" << QtDataSync::KeyStore::listProviders();

	//setup datasync
	QtDataSync::Setup()
			.setRemoteConfiguration(QUrl(QStringLiteral("ws://localhost:4242")))
			.create();

	Widget w;
	w.show();

	return a.exec();
}
