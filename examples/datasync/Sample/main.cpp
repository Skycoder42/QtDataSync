#include "widget.h"
#include <QApplication>
#include <QLoggingCategory>
#include <QInputDialog>
#include <QSettings>
#include <QMessageBox>
#include <QtDataSync/syncmanager.h>
#include <QtDataSync/setup.h>
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

	qDebug() << "Available keystores:" << QtDataSync::Setup::keystoreProviders();
	qDebug() << "Active keystores:" << QtDataSync::Setup::availableKeystores();

	QString name;
	if(QCoreApplication::arguments().size() >= 2)
		name = QCoreApplication::arguments().at(1);
	else {
		QSettings settings;
		auto known = settings.value(QStringLiteral("clients")).toStringList();
		bool ok = false;
		name = QInputDialog::getItem(nullptr,
									 QCoreApplication::translate("GLOBAL", "Client select"),
									 QCoreApplication::translate("GLOBAL", "Select a client to work with (you can enter a new name):"),
									 known, 0, true, &ok);
		if(name.isEmpty() || !ok)
			return EXIT_FAILURE;
		if(!known.contains(name)) {
			known.append(name);
			settings.setValue(QStringLiteral("clients"), known);
		}
	}

	//setup datasync
	QtDataSync::Setup setup;
	setup.setLocalDir(QStringLiteral("./%1").arg(name))
			.setRemoteConfiguration(QUrl(QStringLiteral("ws://localhost:4242")))
			.setRemoteObjectHost(QStringLiteral("local:qtdssample_%1").arg(name)) //use local to enable passive setup
			.setSignatureScheme(QtDataSync::Setup::ECDSA_ECP_SHA3_512);
	try {
		setup.create(name);
	} catch(QtDataSync::SetupLockedException &e) {
		qDebug() << e.what();
		if(QMessageBox::question(nullptr,
								 QCoreApplication::translate("GLOBAL", "Setup locked"),
								 QCoreApplication::translate("GLOBAL", "Setup already locked. Start as passive setup?")))
			setup.createPassive(name);
		else
			return EXIT_FAILURE;
	}

	Widget w(name);
	w.show();

	return a.exec();
}
